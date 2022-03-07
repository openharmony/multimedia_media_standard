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

#include "recorder_message_processor.h"
#include "recorder_inner_defines.h"
#include "media_errors.h"
#include "media_log.h"
#include "scope_guard.h"
#include "i_recorder_engine.h"

namespace {
using namespace OHOS::Media;
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecMsgProc"};
static const std::unordered_map<GstMessageType, RecorderMessageFeature> FEATURE_MSG_TYPE_CVT_TABLE = {
    { GST_MESSAGE_EOS, REC_MSG_FEATURE_EOS_DONE },
    { GST_MESSAGE_ASYNC_DONE, REC_MSG_FEATURE_ASYNC_DONE },
    { GST_MESSAGE_STATE_CHANGED, REC_MSG_FEATURE_STATE_CHANGE_DONE },
};
}

namespace OHOS {
namespace Media {
RecorderMsgProcResult RecorderMsgHandler::ProcessInfoMsgDefault(GstMessage &msg, RecorderMessage &prettyMsg)
{
    (void)prettyMsg;

    GstInfoMsgParser parser(msg);
    CHECK_AND_RETURN_RET(parser.InitCheck(), RecorderMsgProcResult::REC_MSG_PROC_FAILED);
    MEDIA_LOGI("[INFO] %{public}s, %{public}s", parser.GetErr()->message, parser.GetDbg());

    return RecorderMsgProcResult::REC_MSG_PROC_IGNORE;
}

RecorderMsgProcResult RecorderMsgHandler::ProcessWarningMsgDefault(GstMessage &msg, RecorderMessage &prettyMsg)
{
    (void)prettyMsg;

    GstWarningMsgParser parser(msg);
    CHECK_AND_RETURN_RET(parser.InitCheck(), RecorderMsgProcResult::REC_MSG_PROC_FAILED);
    MEDIA_LOGW("[WARNING] %{public}s, %{public}s", parser.GetErr()->message, parser.GetDbg());

    return RecorderMsgProcResult::REC_MSG_PROC_IGNORE;
}

RecorderMsgProcResult RecorderMsgHandler::ProcessErrorMsgDefault(GstMessage &msg, RecorderMessage &prettyMsg)
{
    GstErrorMsgParser parser(msg);
    CHECK_AND_RETURN_RET(parser.InitCheck(), RecorderMsgProcResult::REC_MSG_PROC_FAILED);
    MEDIA_LOGE("[ERROR] %{public}s, %{public}s", parser.GetErr()->message, parser.GetDbg());

    prettyMsg.type = REC_MSG_ERROR;
    prettyMsg.code = IRecorderEngineObs::ErrorType::ERROR_INTERNAL;
    prettyMsg.detail = MSERR_UNKNOWN;

    return RecorderMsgProcResult::REC_MSG_PROC_OK;
}

static RecorderMsgProcResult ProcessFeatureMessage(GstMessage &msg, RecorderMessage &prettyMsg)
{
    auto featureMsgTypeIter = FEATURE_MSG_TYPE_CVT_TABLE.find(GST_MESSAGE_TYPE(&msg));
    if (featureMsgTypeIter != FEATURE_MSG_TYPE_CVT_TABLE.end()) {
        prettyMsg.type = REC_MSG_FEATURE;
        prettyMsg.code = featureMsgTypeIter->second;
        return RecorderMsgProcResult::REC_MSG_PROC_OK;
    }

    return RecorderMsgProcResult::REC_MSG_PROC_IGNORE;
}

static RecorderMsgProcResult ProcessStateChangedMessage(GstMessage &msg, RecorderMessage &prettyMsg)
{
    GstState oldState = GST_STATE_NULL;
    GstState newState = GST_STATE_NULL;
    GstState pendingState = GST_STATE_NULL;

    gst_message_parse_state_changed(&msg, &oldState, &newState, &pendingState);
    MEDIA_LOGI("%{public}s finished state change, oldState: %{public}s, newState: %{public}s",
               GST_ELEMENT_NAME(msg.src), gst_element_state_get_name(oldState),
               gst_element_state_get_name(newState));

    if (GST_IS_PIPELINE(msg.src)) {
        prettyMsg.type = REC_MSG_FEATURE;
        prettyMsg.code = REC_MSG_FEATURE_STATE_CHANGE_DONE;
        prettyMsg.detail = static_cast<int32_t>(newState);
        return RecorderMsgProcResult::REC_MSG_PROC_OK;
    }

    return RecorderMsgProcResult::REC_MSG_PROC_IGNORE;
}

using MessageProcFunc = RecorderMsgProcResult (*)(GstMessage &msg, RecorderMessage &prettyMsg);
static const std::unordered_map<GstMessageType, MessageProcFunc> MSG_PROC_FUNC_TABLE = {
    { GST_MESSAGE_INFO, &RecorderMsgHandler::ProcessInfoMsgDefault },
    { GST_MESSAGE_WARNING, &RecorderMsgHandler::ProcessWarningMsgDefault },
    { GST_MESSAGE_ERROR, &RecorderMsgHandler::ProcessErrorMsgDefault },
    { GST_MESSAGE_EOS, &ProcessFeatureMessage },
    { GST_MESSAGE_ASYNC_DONE, &ProcessFeatureMessage },
    { GST_MESSAGE_STATE_CHANGED, &ProcessStateChangedMessage },
};

gboolean RecorderMsgProcessor::BusCallback(GstBus *bus, GstMessage *msg, gpointer data)
{
    (void)bus;

    RecorderMsgProcessor *processor = reinterpret_cast<RecorderMsgProcessor *>(data);
    if (processor == nullptr) {
        MEDIA_LOGE("processor is nullptr !");
        return FALSE;
    }

    CHECK_AND_RETURN_RET(msg != nullptr, FALSE);

    processor->ProcessMessage(*msg);
    return TRUE;
}

RecorderMsgProcessor::RecorderMsgProcessor(GstBus &gstBus, const MessageResCb &resCb)
    : mainLoopGuard_("rec-pipe-guard"), msgResultCb_(resCb)
{
    gstBus_ = GST_BUS_CAST(gst_object_ref(&gstBus));
}

RecorderMsgProcessor::~RecorderMsgProcessor()
{
    (void)Reset();

    if (gstBus_ != nullptr) {
        gst_object_unref(gstBus_);
        gstBus_ = nullptr;
    }
}

int32_t RecorderMsgProcessor::Init()
{
    if (msgResultCb_ == nullptr) {
        MEDIA_LOGE("message result callback is nullptr");
        return MSERR_INVALID_VAL;
    }

    ON_SCOPE_EXIT(0) { (void)Reset(); };

    mainLoop_ = g_main_loop_new(nullptr, FALSE);
    CHECK_AND_RETURN_RET(mainLoop_ != nullptr, MSERR_NO_MEMORY);

    busWatchId_ = gst_bus_add_watch(gstBus_, (GstBusFunc)&RecorderMsgProcessor::BusCallback, this);
    CHECK_AND_RETURN_RET(busWatchId_ != 0, MSERR_INVALID_OPERATION);

    int32_t ret = mainLoopGuard_.Start();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, MSERR_INVALID_OPERATION);

    auto mainLoopGuardTask = std::make_shared<TaskHandler<void>>([this] { g_main_loop_run(mainLoop_); });

    ret = mainLoopGuard_.EnqueueTask(mainLoopGuardTask);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, MSERR_INVALID_OPERATION);

    CANCEL_SCOPE_EXIT_GUARD(0);
    return MSERR_OK;
}

void RecorderMsgProcessor::AddMsgHandler(std::shared_ptr<RecorderMsgHandler> handler)
{
    if (handler == nullptr) {
        MEDIA_LOGE("handler is nullptr");
        return;
    }

    std::unique_lock<std::mutex> lock(mutex_);
    for (auto &item : msgHandlers_) {
        if (item == handler) {
            return;
        }
    }

    msgHandlers_.push_back(handler);
}

int32_t RecorderMsgProcessor::Reset()
{
    if (errorProcQ_ != nullptr) {
        (void)errorProcQ_->Stop();
        errorProcQ_ = nullptr;
    }

    if (busWatchId_ != 0) {
        (void)g_source_remove(busWatchId_);
        busWatchId_ = 0;
    }

    if (mainLoop_ != nullptr) {
        if (g_main_loop_is_running(mainLoop_)) {
            g_main_loop_quit(mainLoop_);
        }
        g_main_loop_unref(mainLoop_);
        mainLoop_ = nullptr;
    }

    (void)mainLoopGuard_.Stop();
    return MSERR_OK;
}

void RecorderMsgProcessor::ProcessMessage(GstMessage &msg)
{
    RecorderMessage prettyMsg {};
    RecorderMsgProcResult rst = RecorderMsgProcResult::REC_MSG_PROC_IGNORE;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        for (auto &msgHandler : msgHandlers_) {
            rst = msgHandler->OnMessageReceived(msg, prettyMsg);
            if (rst != RecorderMsgProcResult::REC_MSG_PROC_IGNORE) {
                break;
            }
        }
    }

    if  (rst == RecorderMsgProcResult::REC_MSG_PROC_IGNORE) {
        rst = ProcessMessageFinal(msg, prettyMsg);
    }

    if (rst == RecorderMsgProcResult::REC_MSG_PROC_FAILED) {
        NotifyInternalError(prettyMsg);
        return;
    }

    if (rst == RecorderMsgProcResult::REC_MSG_PROC_IGNORE) {
        return;
    }

    ReportMsgProcResult(prettyMsg);
}

RecorderMsgProcResult RecorderMsgProcessor::ProcessExtendMessage(GstMessage &msg, RecorderMessage &prettyMsg) const
{
    (void)msg;
    (void)prettyMsg;
    // Check whether the message format is extented format. If yes, translate it here and return OK
    // ohos extented format: ohos.ext type=*, code=*   the type value must exceed than 0x10000
    return RecorderMsgProcResult::REC_MSG_PROC_IGNORE;
}

RecorderMsgProcResult RecorderMsgProcessor::ProcessMessageFinal(GstMessage &msg, RecorderMessage &prettyMsg)
{
    if (!(GST_IS_PIPELINE(msg.src) || (GST_MESSAGE_TYPE(&msg) == GST_MESSAGE_ERROR))) {
        auto tblIter = MSG_PROC_FUNC_TABLE.find(GST_MESSAGE_TYPE(&msg));
        if (tblIter == MSG_PROC_FUNC_TABLE.end()) {
            return RecorderMsgProcResult::REC_MSG_PROC_IGNORE;
        }
        tblIter->second(msg, prettyMsg); // only print to log
        return RecorderMsgProcResult::REC_MSG_PROC_IGNORE;
    }

    prettyMsg.sourceId = INVALID_SOURCE_ID;

    RecorderMsgProcResult ret = ProcessExtendMessage(msg, prettyMsg);
    if (ret != RecorderMsgProcResult::REC_MSG_PROC_IGNORE) {
        return ret;
    }

    auto tblIter = MSG_PROC_FUNC_TABLE.find(GST_MESSAGE_TYPE(&msg));
    if (tblIter == MSG_PROC_FUNC_TABLE.end()) {
        return RecorderMsgProcResult::REC_MSG_PROC_IGNORE;
    }

    return tblIter->second(msg, prettyMsg);
}

void RecorderMsgProcessor::NotifyInternalError(RecorderMessage &msg)
{
    // sourceid keep unchanged.
    msg.type = RecorderMessageType::REC_MSG_ERROR;
    msg.code = IRecorderEngineObs::ErrorType::ERROR_INTERNAL;
    msg.detail = MSERR_UNKNOWN;

    ReportMsgProcResult(msg);
}

void RecorderMsgProcessor::ReportMsgProcResult(const RecorderMessage &msg)
{
    if (msg.type != REC_MSG_ERROR) {
        return msgResultCb_(msg);
    }

    if (errorProcQ_ == nullptr) {
        errorProcQ_ = std::make_unique<TaskQueue>("rec-err-proc");
        int32_t ret = errorProcQ_->Start();
        CHECK_AND_RETURN_LOG(ret == MSERR_OK, "unable to async process error msg !");
    }

    auto errorProc = std::make_shared<TaskHandler<void>>([this, msg] { msgResultCb_(msg); });

    int32_t ret = errorProcQ_->EnqueueTask(errorProc);
    CHECK_AND_RETURN_LOG(ret == MSERR_OK, "unable to async process error msg !");
}
} // namespace Media
} // namespace OHOS