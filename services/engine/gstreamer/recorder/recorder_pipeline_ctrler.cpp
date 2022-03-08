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

#include "recorder_pipeline_ctrler.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderPipelineCtrler"};
}

namespace OHOS {
namespace Media {
RecorderPipelineCtrler::RecorderPipelineCtrler()
{
    MEDIA_LOGD("enter ctor");
}

RecorderPipelineCtrler::~RecorderPipelineCtrler()
{
    MEDIA_LOGD("enter dtor");

    if (cmdQ_ != nullptr) {
        (void)Stop(false);
        (void)cmdQ_->Stop();
    }

    if (msgQ_ != nullptr) {
        (void)msgQ_->Stop();
    }
}

void RecorderPipelineCtrler::SetObs(const std::weak_ptr<IRecorderEngineObs> &obs)
{
    obs_ = obs;
}

void RecorderPipelineCtrler::SetPipeline(std::shared_ptr<RecorderPipeline> pipeline)
{
    if (pipeline == nullptr) {
        MEDIA_LOGE("Null pipeline !");
        return;
    }

    if (pipeline_ != nullptr) {
        MEDIA_LOGE("Already set pipeline, ignore !");
        return;
    }

    pipeline_ = pipeline;

    auto notifier = std::bind(&RecorderPipelineCtrler::Notify, this, std::placeholders::_1);
    pipeline_->SetNotifier(notifier);
}

int32_t RecorderPipelineCtrler::Init()
{
    cmdQ_ = std::make_unique<TaskQueue>("rec-pipe-ctrler-cmd");
    int32_t ret = cmdQ_->Start();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    msgQ_ = std::make_unique<TaskQueue>("rec-pipe-ctrler-msg");
    ret = msgQ_->Start();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    return MSERR_OK;
}

int32_t RecorderPipelineCtrler::Prepare()
{
    MEDIA_LOGD("enter");

    if (pipeline_ == nullptr) {
        MEDIA_LOGE("Null pipeline !");
        return MSERR_INVALID_OPERATION;
    }

    auto prepareTask = std::make_shared<TaskHandler<int32_t>>([this] { return pipeline_->Prepare(); });
    int ret = cmdQ_->EnqueueTask(prepareTask);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    auto result = prepareTask->GetResult();
    CHECK_AND_RETURN_RET(!result.HasResult() || (result.Value() == MSERR_OK), result.Value());

    return MSERR_OK;
}

int32_t RecorderPipelineCtrler::Start()
{
    MEDIA_LOGD("enter");

    if (pipeline_ == nullptr) {
        MEDIA_LOGE("Null pipeline !");
        return MSERR_INVALID_OPERATION;
    }

    auto startTask = std::make_shared<TaskHandler<int32_t>>([this] { return pipeline_->Start(); });
    int ret = cmdQ_->EnqueueTask(startTask);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    auto result = startTask->GetResult();
    CHECK_AND_RETURN_RET(!result.HasResult() || (result.Value() == MSERR_OK), result.Value());

    return MSERR_OK;
}

int32_t RecorderPipelineCtrler::Pause()
{
    MEDIA_LOGD("enter");

    if (pipeline_ == nullptr) {
        MEDIA_LOGE("Null pipeline !");
        return MSERR_INVALID_OPERATION;
    }

    auto pauseTask = std::make_shared<TaskHandler<int32_t>>([this] { return pipeline_->Pause(); });
    int ret = cmdQ_->EnqueueTask(pauseTask);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    auto result = pauseTask->GetResult();
    CHECK_AND_RETURN_RET(!result.HasResult() || (result.Value() == MSERR_OK), result.Value());

    return MSERR_OK;
}

int32_t RecorderPipelineCtrler::Resume()
{
    MEDIA_LOGD("enter");

    if (pipeline_ == nullptr) {
        MEDIA_LOGE("Null pipeline !");
        return MSERR_INVALID_OPERATION;
    }

    auto resumeTask = std::make_shared<TaskHandler<int32_t>>([this] { return pipeline_->Resume(); });
    int ret = cmdQ_->EnqueueTask(resumeTask);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    auto result = resumeTask->GetResult();
    CHECK_AND_RETURN_RET(!result.HasResult() || (result.Value() == MSERR_OK), result.Value());

    return MSERR_OK;
}

int32_t RecorderPipelineCtrler::Stop(bool isDrainAll)
{
    if (pipeline_ == nullptr) {
        return MSERR_OK;
    }

    MEDIA_LOGD("enter");

    auto stopTask = std::make_shared<TaskHandler<int32_t>>(
        [this, isDrainAll] { return pipeline_->Stop(isDrainAll); }
    );
    int ret = cmdQ_->EnqueueTask(stopTask, true);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    auto result = stopTask->GetResult();
    CHECK_AND_RETURN_RET(result.HasResult(), MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(result.Value() == MSERR_OK, result.Value());

    return MSERR_OK;
}

int32_t RecorderPipelineCtrler::Reset()
{
    MEDIA_LOGD("enter");
    pipeline_ = nullptr;
    return MSERR_OK;
}

void RecorderPipelineCtrler::Notify(const RecorderMessage &msg)
{
    auto notifyTask = std::make_shared<TaskHandler<void>>([this, msg] {
        std::shared_ptr<IRecorderEngineObs> obs = obs_.lock();
        CHECK_AND_RETURN_LOG(obs != nullptr, "obs is nullptr");
        MEDIA_LOGI("Receive message, type: %{public}d, code: %{public}d, detail: %{public}d",
                   msg.type, msg.code, msg.detail);
        if (msg.type == RecorderMessageType::REC_MSG_INFO) {
            obs->OnInfo(static_cast<IRecorderEngineObs::InfoType>(msg.code), msg.detail);
        } else if (msg.type == RecorderMessageType::REC_MSG_ERROR) {
            obs->OnError(static_cast<IRecorderEngineObs::ErrorType>(msg.code), msg.detail);
        }
    });

    int ret = msgQ_->EnqueueTask(notifyTask);
    CHECK_AND_RETURN_LOG(ret == MSERR_OK, "Enqueue message failed !");
}
} // namespace Media
} // namespace OHOS
