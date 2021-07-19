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

#include "recorder_pipeline.h"
#include <gst/gst.h>
#include "errors.h"
#include "media_log.h"
#include "scope_guard.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderPipeline"};
}

namespace OHOS {
namespace Media {
gboolean RecorderPipeline::BusCallbackWrapper(GstBus *bus, const GstMessage *msg, gpointer data)
{
    (void)bus;

    RecorderPipeline *pipeline = (RecorderPipeline *)data;
    if (pipeline == nullptr) {
        MEDIA_LOGE("pipeline is nullptr !");
        return FALSE;
    }

    CHECK_AND_RETURN_RET(msg != nullptr, FALSE);

    int32_t ret = pipeline->BusCallback(*msg);
    CHECK_AND_RETURN_RET(ret == ERR_OK, FALSE);

    return TRUE;
}

RecorderPipeline::RecorderPipeline(std::shared_ptr<RecorderPipelineDesc> desc)
    : desc_(desc), mainLoopGuard_("RecorderPipeline")
{
    MEDIA_LOGD("enter, ctor");
}

RecorderPipeline::~RecorderPipeline()
{
    MEDIA_LOGD("enter, dtor");
    Reset();
}

void RecorderPipeline::SetNotifier(RecorderNotifier notifier)
{
    notifier_ = notifier;
    auto callback = std::bind(&RecorderPipeline::NotifyCallback, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    for (auto &elem : desc_->allElems) {
        elem->SetNotifier(callback);
    }
}

int32_t RecorderPipeline::Init()
{
    ON_SCOPE_EXIT(0) { ClearResource(); };

    if (desc_ == nullptr) {
        MEDIA_LOGE("pipeline desc is nullptr");
        return ERR_INVALID_OPERATION;
    }

    gstPipeline_ = (GstPipeline *)gst_pipeline_new("recorder-pipeline");
    if (gstPipeline_ == nullptr) {
        MEDIA_LOGE("Create gst pipeline failed !");
        return ERR_NO_MEMORY;
    }

    mainLoop_ = g_main_loop_new(nullptr, FALSE);
    CHECK_AND_RETURN_RET(mainLoop_ != nullptr, ERR_NO_MEMORY);

    GstBus *bus = gst_pipeline_get_bus(gstPipeline_);
    CHECK_AND_RETURN_RET(bus != nullptr, ERR_INVALID_OPERATION);

    busWatchId_ = gst_bus_add_watch(bus, (GstBusFunc)&RecorderPipeline::BusCallbackWrapper, this);
    CHECK_AND_RETURN_RET(busWatchId_ != 0, ERR_INVALID_OPERATION);
    gst_object_unref(bus);

    int32_t ret = mainLoopGuard_.Start();
    CHECK_AND_RETURN_RET(ret == ERR_OK, ERR_INVALID_OPERATION);

    auto mainLoopGuardTask = std::make_shared<TaskHandler>([this] {
        g_main_loop_run(mainLoop_);
        return ERR_OK; // need to optimize the taskhandler to support no-return or any return value type
    }, ERR_OK);

    ret = mainLoopGuard_.EnqueueTask(mainLoopGuardTask);
    CHECK_AND_RETURN_RET(ret == ERR_OK, ERR_INVALID_OPERATION);

    CANCEL_SCOPE_EXIT_GUARD(0);

    return ERR_OK;
}

int32_t RecorderPipeline::Prepare()
{
    MEDIA_LOGD("enter Prepare");

    int32_t ret = DoElemAction(&RecorderElement::Prepare);
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    ret = SyncWaitChangeState(GST_STATE_PAUSED);
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    return ERR_OK;
}

int32_t RecorderPipeline::Start()
{
    MEDIA_LOGD("enter Start");

    int32_t ret = DoElemAction(&RecorderElement::Start);
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    ret = SyncWaitChangeState(GST_STATE_PLAYING);
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    isStarted_ = true;
    return ERR_OK;
}

int32_t RecorderPipeline::Pause()
{
    MEDIA_LOGD("enter Pause");

    int32_t ret = DoElemAction(&RecorderElement::Pause);
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    return SyncWaitChangeState(GST_STATE_PAUSED);
}

int32_t RecorderPipeline::Resume()
{
    MEDIA_LOGD("enter Resume");

    int32_t ret = DoElemAction(&RecorderElement::Resume);
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    return SyncWaitChangeState(GST_STATE_PLAYING);
}

int32_t RecorderPipeline::Stop(bool isDrainAll)
{
    if (currState_ == GST_STATE_NULL) {
        return ERR_OK;
    }

    MEDIA_LOGI("enter Stop, isDrainAll = %{public}d", isDrainAll);
    DrainBuffer(isDrainAll);

    (void) DoElemAction(&RecorderElement::Stop, false);

    int32_t ret = SyncWaitChangeState(GST_STATE_NULL);
    if (ret != ERR_OK) {
        MEDIA_LOGW("Stop failed !");
        return ret;
    }

    isStarted_ = false;
    return ERR_OK;
}

int32_t RecorderPipeline::SetParameter(int32_t sourceId, const RecorderParam &recParam)
{
    int32_t ret = ERR_OK;
    for (auto &elem : desc_->allElems) {
        if (elem->GetSourceId() == sourceId)  {
            ret = elem->SetParameter(recParam);
            CHECK_AND_RETURN_RET(ret == ERR_OK, ret);
        }
    }

    return ret;
}

int32_t RecorderPipeline::GetParameter(int32_t sourceId, RecorderParam &recParam)
{
    if (desc_->srcElems.find(sourceId) == desc_->srcElems.end()) {
        MEDIA_LOGE("invalid sourceId %{public}d", sourceId);
        return ERR_INVALID_VALUE;
    }
    return desc_->srcElems[sourceId]->GetParameter(recParam);
}

int32_t RecorderPipeline::BusCallback(const GstMessage &msg)
{
    switch (GST_MESSAGE_TYPE(&msg)) {
        case GST_MESSAGE_ASYNC_DONE: {
            {
                std::unique_lock<std::mutex> lock(gstPipeMutex_);
                asyncDone_ = true;
                MEDIA_LOGI("Accept message GST_MESSAGE_ASYNC_DONE");
            }
            gstPipeCond_.notify_one();
            break;
        }
        case GST_MESSAGE_EOS: {
            {
                std::unique_lock<std::mutex> lock(gstPipeMutex_);
                eosDone_ = true;
                MEDIA_LOGI("Accept message GST_MESSAGE_EOS");
            }
            gstPipeCond_.notify_one();
            break;
        }
        case GST_MESSAGE_ERROR: {
            gchar  *debug;
            GError *error;
            gst_message_parse_error(const_cast<GstMessage *>(&msg), &error, &debug);
            MEDIA_LOGE("Gstreamer emit error, error msg: %{public}s, debug msg: %{public}s", error->message, debug);
            g_error_free (error);
            g_free (debug);
            notifier_(RECORDER_NOTIFY_ERROR, IRecorderEngineObs::ErrorType::ERROR_UNKNOWN, 0);
            break;
        }
        default:
            MEDIA_LOGD("Get message %{public}s, ignore", gst_message_type_get_name(GST_MESSAGE_TYPE(&msg)));
            break;
    }

    return ERR_OK;
}

int32_t RecorderPipeline::SyncWaitChangeState(GstState targetState)
{
    MEDIA_LOGI("change state to %{public}d", targetState);

    GstStateChangeReturn stateRet = gst_element_set_state((GstElement *)gstPipeline_, targetState);
    CHECK_AND_RETURN_RET(stateRet != GST_STATE_CHANGE_FAILURE,  ERR_INVALID_OPERATION);

    if (stateRet != GST_STATE_CHANGE_ASYNC) {
        MEDIA_LOGI("finish change gstpipeline state to %{public}d.", targetState);
        currState_ = targetState;
        return ERR_OK;
    }

    std::unique_lock<std::mutex> lock(gstPipeMutex_);
    MEDIA_LOGI("begin sync wait gstpipeline state change to %{public}d..........", targetState);
    gstPipeCond_.wait(lock, [this] { return asyncDone_; });
    asyncDone_ = false;
    currState_ = targetState;

    MEDIA_LOGI("finish change gstpipeline state to %{public}d..........", targetState);
    return ERR_OK;
}

void RecorderPipeline::DrainBuffer(bool isDrainAll)
{
    if (currState_ == GST_STATE_PAUSED) {
        if (isStarted_) {
            SyncWaitChangeState(GST_STATE_PLAYING);
        } else {
            return;
        }
    }

    if (isDrainAll) {
        (void) PostAndSyncWaitEOS();
        return;
    }

    if (desc_->muxerSinkBin != nullptr) {
        bool needWaitEos = desc_->muxerSinkBin->DrainAll();
        if (needWaitEos) {
            SyncWaitEOS();
        }
    }
}

int32_t RecorderPipeline::PostAndSyncWaitEOS()
{
    GstEvent *eos = gst_event_new_eos();
    if (eos == nullptr) {
        MEDIA_LOGE("Create EOS event failed");
        return ERR_INVALID_OPERATION;
    }

    gboolean success = gst_element_send_event((GstElement *)gstPipeline_, eos);
    if (!success) {
        MEDIA_LOGE("Send EOS event failed");
        return ERR_INVALID_OPERATION;
    }

    SyncWaitEOS();
    return ERR_OK;
}

void RecorderPipeline::SyncWaitEOS()
{
    MEDIA_LOGI("Wait EOS finished........................");
    std::unique_lock<std::mutex> lock(gstPipeMutex_);
    gstPipeCond_.wait(lock, [this] { return eosDone_; });
    eosDone_ = false;
    MEDIA_LOGI("EOS finished........................");
}

int32_t RecorderPipeline::Reset()
{
    Stop(false);
    (void)DoElemAction(&RecorderElement::Reset, false);
    ClearResource();
    return ERR_OK;
}

int32_t RecorderPipeline::DoElemAction(const ElemAction &action, bool needAllSucc)
{
    if (desc_ == nullptr)  {
        return ERR_INVALID_OPERATION;
    }

    bool allSucc = true;
    for (auto &elem : desc_->allElems) {
        int32_t ret = action(*elem);
        if (ret == ERR_OK) {
            continue;
        }
        allSucc = false;
        // if one element execute action fail, exit immediately.
        if (needAllSucc) {
            MEDIA_LOGE("element %{public}s execute action failed", elem->GetName().c_str());
            return ret;
        }
    }

    return allSucc ? ERR_OK : ERR_INVALID_OPERATION;
}

void RecorderPipeline::ClearResource()
{
    if (busWatchId_ != 0) {
        g_source_remove(busWatchId_);
        busWatchId_ = 0;
    }

    if (mainLoop_ != nullptr) {
        if (g_main_loop_is_running(mainLoop_)) {
            g_main_loop_quit(mainLoop_);
        }
        g_main_loop_unref(mainLoop_);
        mainLoop_ = nullptr;
    }

    if (gstPipeline_ != nullptr) {
        gst_object_unref(gstPipeline_);
        gstPipeline_ = nullptr;
    }

    mainLoopGuard_.Stop();
    desc_ = nullptr;
}

void RecorderPipeline::NotifyCallback(int32_t type, int32_t code, int32_t detail)
{
    (void)type;
    (void)code;
    (void)detail;
}
}
}