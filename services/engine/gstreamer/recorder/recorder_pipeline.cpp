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
#include "string_ex.h"
#include "media_errors.h"
#include "media_log.h"
#include "i_recorder_engine.h"
#include "recorder_private_param.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderPipeline"};
}

namespace OHOS {
namespace Media {
RecorderPipeline::RecorderPipeline(std::shared_ptr<RecorderPipelineDesc> desc)
    : desc_(desc)
{
    MEDIA_LOGD("enter, ctor");
}

RecorderPipeline::~RecorderPipeline()
{
    MEDIA_LOGD("enter, dtor");
    Reset();
}

void RecorderPipeline::SetNotifier(RecorderMsgNotifier notifier)
{
    std::unique_lock<std::mutex> lock(gstPipeMutex_);
    notifier_ = notifier;
}

int32_t RecorderPipeline::Init()
{
    if (desc_ == nullptr) {
        MEDIA_LOGE("pipeline desc is nullptr");
        return ERR_INVALID_OPERATION;
    }

    gstPipeline_ = reinterpret_cast<GstPipeline *>(gst_pipeline_new("recorder-pipeline"));
    if (gstPipeline_ == nullptr) {
        MEDIA_LOGE("Create gst pipeline failed !");
        return ERR_NO_MEMORY;
    }

    GstBus *bus = gst_pipeline_get_bus(gstPipeline_);
    CHECK_AND_RETURN_RET(bus != nullptr, ERR_INVALID_OPERATION);

    auto msgResCb = std::bind(&RecorderPipeline::OnNotifyMsgProcResult, this, std::placeholders::_1);
    msgProcessor_ = std::make_unique<RecorderMsgProcessor>(*bus, msgResCb);
    gst_object_unref(bus);
    bus = nullptr;

    int32_t ret = msgProcessor_->Init();
    if (ret != MSERR_OK) {
        MEDIA_LOGE("Init RecorderMsgProcessor Failed !,  ret = %{publiuc}d", ret);
        ClearResource();
        return ret;
    }

    for (auto &elem : desc_->allElems) {
        msgProcessor_->AddMsgHandler(elem);
    }

    return ERR_OK;
}

int32_t RecorderPipeline::Prepare()
{
    MEDIA_LOGD("enter Prepare");

    CHECK_AND_RETURN_RET(!errorState_.load(), MSERR_INVALID_STATE);

    int32_t ret = DoElemAction(&RecorderElement::Prepare);
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    ret = SyncWaitChangeState(GST_STATE_PAUSED);
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    return ERR_OK;
}

int32_t RecorderPipeline::Start()
{
    MEDIA_LOGD("enter Start");

    CHECK_AND_RETURN_RET(!errorState_.load(), MSERR_INVALID_STATE);

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

    CHECK_AND_RETURN_RET(!errorState_.load(), MSERR_INVALID_STATE);

    int32_t ret = DoElemAction(&RecorderElement::Pause);
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    return SyncWaitChangeState(GST_STATE_PAUSED);
}

int32_t RecorderPipeline::Resume()
{
    MEDIA_LOGD("enter Resume");

    CHECK_AND_RETURN_RET(!errorState_.load(), MSERR_INVALID_STATE);

    int32_t ret = DoElemAction(&RecorderElement::Resume);
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    return SyncWaitChangeState(GST_STATE_PLAYING);
}

int32_t RecorderPipeline::Stop(bool isDrainAll)
{
    if (errorState_.load()) {
        return MSERR_INVALID_STATE;
    }

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
    CHECK_AND_RETURN_RET(!errorState_.load(), MSERR_INVALID_STATE);

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
    CHECK_AND_RETURN_RET(!errorState_.load(), MSERR_INVALID_STATE);

    if (desc_->srcElems.find(sourceId) == desc_->srcElems.end()) {
        MEDIA_LOGE("invalid sourceId %{public}d", sourceId);
        return ERR_INVALID_VALUE;
    }
    return desc_->srcElems[sourceId]->GetParameter(recParam);
}

int32_t RecorderPipeline::SyncWaitChangeState(GstState targetState)
{
    MEDIA_LOGI("change state to %{public}d", targetState);

    GstStateChangeReturn stateRet = gst_element_set_state((GstElement *)gstPipeline_, targetState);
    CHECK_AND_RETURN_RET(stateRet != GST_STATE_CHANGE_FAILURE,  MSERR_INVALID_STATE);

    if (stateRet != GST_STATE_CHANGE_ASYNC) {
        MEDIA_LOGI("finish change gstpipeline state to %{public}d.", targetState);
        currState_ = targetState;
        return MSERR_OK;
    }

    MEDIA_LOGI("begin sync wait gstpipeline state change to %{public}d..........", targetState);
    std::unique_lock<std::mutex> lock(gstPipeMutex_);
    gstPipeCond_.wait(lock, [this, targetState] { return currState_ == targetState || errorState_.load(); });
    if (errorState_.load()) {
        MEDIA_LOGE("error happended, change state to %{public}d failed !", targetState);
        return MSERR_INVALID_STATE;
    }
    MEDIA_LOGI("finish change gstpipeline state to %{public}d..........", targetState);
    return MSERR_OK;
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
        } // no need wait eos does not mean a error
    }
}

int32_t RecorderPipeline::PostAndSyncWaitEOS()
{
    if (currState_ != GST_STATE_PLAYING) {
        MEDIA_LOGE("curr state is not GST_STATE_PLAYING, ignore !");
        return MSERR_OK;
    }

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

bool RecorderPipeline::SyncWaitEOS()
{
    MEDIA_LOGI("Wait EOS finished........................");
    std::unique_lock<std::mutex> lock(gstPipeMutex_);
    gstPipeCond_.wait(lock, [this] { return eosDone_ || errorState_.load(); });
    if (!eosDone_) {
        MEDIA_LOGE("error happended, wait eos done failed !");
        return false;
    }
    eosDone_ = false;
    MEDIA_LOGI("EOS finished........................");
    return true;
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
    if (msgProcessor_ != nullptr) {
        (void)msgProcessor_->Reset();
        msgProcessor_ = nullptr;
    }

    if (gstPipeline_ != nullptr) {
        gst_object_unref(gstPipeline_);
        gstPipeline_ = nullptr;
    }

    desc_ = nullptr;
}

void RecorderPipeline::Dump()
{
    MEDIA_LOGI("==========================Dump Recorder Parameters Begin=========================");
    for (auto &elem : desc_->allElems) {
        elem->Dump();
    }
    MEDIA_LOGI("==========================Dump Recorder Parameters End===========================");
}

void RecorderPipeline::OnNotifyMsgProcResult(const RecorderMessage &msg)
{
    if (msg.type == RecorderMessageType::REC_MSG_INFO) {
        return ProcessInfoMessage(msg);
    }

    if (msg.type == RecorderMessageType::REC_MSG_ERROR) {
        return ProcessErrorMessage(msg);
    }

    if (msg.type == RecorderMessageType::REC_MSG_FEATURE) {
        return ProcessFeatureMessage(msg);
    }
}

void RecorderPipeline::ProcessInfoMessage(const RecorderMessage &msg)
{
    NotifyMessage(msg);
}

void RecorderPipeline::ProcessErrorMessage(const RecorderMessage &msg)
{
    // ignore the error msg
    if (errorState_.load() || (errorSources_.count(msg.sourceId) != 0)) {
        return;
    }

    if (CheckStopForError(msg)) {
        StopForError(msg);
        return;
    }

    int ret = BypassOneSource(msg.sourceId);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("bypass source[0x%{public}x] failed, stop recording.", msg.sourceId);
        StopForError(msg);
        return;
    }
    NotifyMessage(msg);
}

void RecorderPipeline::ProcessFeatureMessage(const RecorderMessage &msg)
{
    switch (msg.code) {
        case REC_MSG_FEATURE_ASYNC_DONE: {
            {
                std::unique_lock<std::mutex> lock(gstPipeMutex_);
                asyncDone_ = true;
                MEDIA_LOGI("Accept message GST_MESSAGE_ASYNC_DONE");
            }
            gstPipeCond_.notify_one();
            break;
        }
        case REC_MSG_FEATURE_EOS_DONE: {
            {
                std::unique_lock<std::mutex> lock(gstPipeMutex_);
                eosDone_ = true;
                MEDIA_LOGI("Accept message GST_MESSAGE_EOS");
            }
            gstPipeCond_.notify_one();
            break;
        }
        case REC_MSG_FEATURE_STATE_CHANGE_DONE: {
            {
                std::unique_lock<std::mutex> lock(gstPipeMutex_);
                currState_ = static_cast<GstState>(msg.detail);
                MEDIA_LOGI("Accept message REC_MSG_FEATURE_STATE_CHANGE_DONE, currState = %{public}d", currState_);
            }
            gstPipeCond_.notify_one();
            break;
        }
        default:
            MEDIA_LOGW("unknown feature message: %{public}d", msg.code);
            break;
    }
}

void RecorderPipeline::NotifyMessage(const RecorderMessage &msg)
{
    std::unique_lock<std::mutex> lock(gstPipeMutex_);
    if (notifier_ != nullptr) {
        notifier_(msg);
    }
}

bool RecorderPipeline::CheckStopForError(const RecorderMessage &msg)
{
    // Not meaningful sourceId, means the error is related to all sources, and the recording must be stopped.
    if (msg.sourceId == INVALID_SOURCE_ID || msg.sourceId == DUMMY_SOURCE_ID) {
        return true;
    }

    errorSources_.emplace(msg.sourceId);
    if (errorSources_.size() == desc_->srcElems.size()) {
        return true;
    }

    return false;
}

void RecorderPipeline::StopForError(const RecorderMessage &msg)
{
    MEDIA_LOGE("Fatal error happended, stop recording. Error code: %{public}d, detail: %{public}d",
               msg.code, msg.detail);

    errorState_.store(true);
    DrainBuffer(false);
    (void) DoElemAction(&RecorderElement::Stop, false);
    (void) SyncWaitChangeState(GST_STATE_NULL);

    isStarted_ = false;
    gstPipeCond_.notify_all();

    NotifyMessage(msg);
}

int32_t RecorderPipeline::BypassOneSource(int32_t sourceId)
{
    MEDIA_LOGE("recorder source[0x%{public}x] has error happended, bypass it", sourceId);

    auto srcElemIter = desc_->srcElems.find(sourceId);
    if (srcElemIter == desc_->srcElems.end() || srcElemIter->second == nullptr) {
        MEDIA_LOGE("The sourceId 0x%{public}x is unrecognizable, ignored !", sourceId);
        std::string srcIdStr;
        for (auto &srcElemItem : desc_->srcElems) {
            srcIdStr += DexToHexString(srcElemItem.first) + " ";
        }
        MEDIA_LOGE("Valid source id: %{public}s", srcIdStr.c_str());
        return MSERR_INVALID_VAL;
    }

    bool ret = srcElemIter->second->DrainAll();
    CHECK_AND_RETURN_RET(ret, MSERR_INVALID_OPERATION);

    return MSERR_OK;
}
}
}
