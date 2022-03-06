/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "gst_msg_processor.h"
#include <unordered_map>
#include "media_errors.h"
#include "media_log.h"
#include "scope_guard.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "GstMsgProc"};
}

namespace OHOS {
namespace Media {
GstMsgProcessor::GstMsgProcessor(
    GstBus &gstBus,
    const InnerMsgNotifier &notifier,
    const std::shared_ptr<IGstMsgConverter> &converter)
    : notifier_(notifier), guardTask_("msg_loop_guard"), msgConverter_(converter)
{
    gstBus_ = GST_BUS_CAST(gst_object_ref(&gstBus));
    MEDIA_LOGD("enter ctor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
}

GstMsgProcessor::~GstMsgProcessor()
{
    MEDIA_LOGD("enter dtor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));

    Reset();

    if (gstBus_ != nullptr) {
        gst_object_unref(gstBus_);
        gstBus_ = nullptr;
    }
}

int32_t GstMsgProcessor::Init()
{
    MEDIA_LOGD("Init enter");

    if (notifier_ == nullptr) {
        MEDIA_LOGE("message notifier is nullptr");
        return MSERR_INVALID_VAL;
    }

    ON_SCOPE_EXIT(0) { Reset(); };

    if (msgConverter_ == nullptr) {
        msgConverter_ = std::make_shared<GstMsgConverterDefault>();
    }

    int32_t ret = guardTask_.Start();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    auto mainLoopEnter = std::make_shared<TaskHandler<int32_t>>([this]() { return DoInit(); });
    ret = guardTask_.EnqueueTask(mainLoopEnter);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    auto result = mainLoopEnter->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_UNKNOWN, "msg processor init failed");
    CHECK_AND_RETURN_RET_LOG(result.Value() == MSERR_OK, result.Value(), "msg processor init failed");

    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this] { return !needWaiting_; }); // wait main loop run done

    CANCEL_SCOPE_EXIT_GUARD(0);
    MEDIA_LOGD("Init exit");
    return MSERR_OK;
}

int32_t GstMsgProcessor::DoInit()
{
    ON_SCOPE_EXIT(0) { DoReset(); };

    context_ = g_main_context_new();
    CHECK_AND_RETURN_RET(context_ != nullptr, MSERR_NO_MEMORY);

    mainLoop_ = g_main_loop_new(context_, false);
    CHECK_AND_RETURN_RET(mainLoop_ != nullptr, MSERR_NO_MEMORY);
    g_main_context_push_thread_default(context_);

    GSource *source = g_idle_source_new();
    g_source_set_callback(source, (GSourceFunc)MainLoopRunDone, this, nullptr);
    guint ret = g_source_attach(source, context_);
    CHECK_AND_RETURN_RET_LOG(ret > 0, MSERR_INVALID_OPERATION, "add idle source failed");
    g_source_unref(source);

    busSource_ = gst_bus_create_watch(gstBus_);
    CHECK_AND_RETURN_RET_LOG(busSource_ != nullptr, MSERR_NO_MEMORY, "add bus source failed");
    g_source_set_callback(busSource_, (GSourceFunc)&GstMsgProcessor::BusCallback, this, nullptr);
    ret = g_source_attach(busSource_, context_);
    CHECK_AND_RETURN_RET_LOG(ret > 0, MSERR_INVALID_OPERATION, "add bus source failed");

    auto mainLoopRun = std::make_shared<TaskHandler<void>>([this] {
        MEDIA_LOGI("start msg main loop...");
        g_main_loop_run(mainLoop_);
        MEDIA_LOGI("stop msg main loop...");
        DoReset();
    });

    int32_t ret1 = guardTask_.EnqueueTask(mainLoopRun);
    CHECK_AND_RETURN_RET(ret1 == MSERR_OK, ret1);

    CANCEL_SCOPE_EXIT_GUARD(0);
    return MSERR_OK;
}

gboolean GstMsgProcessor::MainLoopRunDone(GstMsgProcessor *thiz)
{
    if (thiz == nullptr) {
        return G_SOURCE_REMOVE;
    }

    std::unique_lock<std::mutex> lock(thiz->mutex_);
    thiz->needWaiting_ = false;
    thiz->cond_.notify_one();

    return G_SOURCE_REMOVE;
}

void GstMsgProcessor::AddMsgFilter(const std::string &filter)
{
    std::unique_lock<std::mutex> lock(mutex_);
    for (auto &elem :  filters_)  {
        if (elem == filter) {
            return;
        }
    }

    MEDIA_LOGI("add msg filter: %{public}s", filter.c_str());
    filters_.push_back(filter);
}

void GstMsgProcessor::FlushBegin()
{
    gst_bus_set_flushing(gstBus_, TRUE);
}

void GstMsgProcessor::FlushEnd()
{
    gst_bus_set_flushing(gstBus_, FALSE);
}

void GstMsgProcessor::DoReset()
{
    if (busSource_ != nullptr) {
        g_source_destroy(busSource_);
        g_source_unref(busSource_);
        busSource_ = nullptr;
    }

    if (mainLoop_ != nullptr) {
        g_main_loop_unref(mainLoop_);
        mainLoop_ = nullptr;
    }

    if (context_ != nullptr) {
        g_main_context_pop_thread_default(context_);
        g_main_context_unref(context_);
        context_ = nullptr;
    }
}

void GstMsgProcessor::Reset() noexcept
{
    if (mainLoop_ != nullptr) {
        if (g_main_loop_is_running(mainLoop_)) {
            g_main_loop_quit(mainLoop_);
        }
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        needWaiting_ = false;
    }

    cond_.notify_all();
    (void)guardTask_.Stop();
    msgConverter_ = nullptr;
}

gboolean GstMsgProcessor::BusCallback(const GstBus *bus, GstMessage *msg, GstMsgProcessor *thiz)
{
    (void)bus;
    if (thiz == nullptr) {
        MEDIA_LOGE("processor is nullptr");
        return FALSE;
    }
    CHECK_AND_RETURN_RET(msg != nullptr, FALSE);
    thiz->ProcessGstMessage(*msg);
    return TRUE;
}

void GstMsgProcessor::ProcessGstMessage(GstMessage &msg)
{
    InnerMessage innerMsg {};
    innerMsg.type = InnerMsgType::INNER_MSG_UNKNOWN;

    int32_t ret = msgConverter_->ConvertToInnerMsg(msg, innerMsg);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("converter gst msg %{public}s failed, this msg is from %{public}s",
                   GST_MESSAGE_TYPE_NAME(&msg), GST_MESSAGE_SRC_NAME(&msg));
        return;
    }

    if (innerMsg.type == InnerMsgType::INNER_MSG_UNKNOWN) {
        return; // ignore.
    }

    if (innerMsg.type != InnerMsgType::INNER_MSG_ERROR) {
        gchar *srcName = GST_OBJECT_NAME(GST_MESSAGE_SRC(&msg));
        if (srcName == nullptr) {
            return;
        }
        std::unique_lock<std::mutex> lock(mutex_);
        for (auto &filter : filters_) {
            if (filter.compare(srcName) == 0) {
                notifier_(innerMsg);
                return;
            }
        }
    } else {
        notifier_(innerMsg);
    }
}
} // namespace Media
} // namespace OHOS
