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
    (void)Stop(false);
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

    auto notifier = std::bind(&RecorderPipelineCtrler::Notify, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    pipeline_->SetNotifier(notifier);
}

int32_t RecorderPipelineCtrler::Init()
{
    cmdQ_ = std::make_unique<TaskQueue>("rec-pipe-ctrler-cmd");
    int32_t ret = cmdQ_->Start();
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    msgQ_ = std::make_unique<TaskQueue>("rec-pipe-ctrler-msg");
    ret = msgQ_->Start();
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    return ERR_OK;
}

int32_t RecorderPipelineCtrler::Prepare()
{
    MEDIA_LOGD("enter");

    if (pipeline_ == nullptr) {
        MEDIA_LOGE("Null pipeline !");
        return ERR_NO_INIT;
    }

    auto prepareTask = std::make_shared<TaskHandler>([this] { return pipeline_->Prepare(); }, ERR_OK);
    int ret = cmdQ_->EnqueueTask(prepareTask);
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    ret = prepareTask->GetResult();
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    return ERR_OK;
}

int32_t RecorderPipelineCtrler::Start()
{
    MEDIA_LOGD("enter");

    if (pipeline_ == nullptr) {
        MEDIA_LOGE("Null pipeline !");
        return ERR_NO_INIT;
    }

    auto startTask = std::make_shared<TaskHandler>([this] { return pipeline_->Start(); }, ERR_OK);
    int ret = cmdQ_->EnqueueTask(startTask);
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    ret = startTask->GetResult();
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    return ERR_OK;
}

int32_t RecorderPipelineCtrler::Pause()
{
    MEDIA_LOGD("enter");

    if (pipeline_ == nullptr) {
        MEDIA_LOGE("Null pipeline !");
        return ERR_NO_INIT;
    }

    auto pauseTask = std::make_shared<TaskHandler>([this] { return pipeline_->Pause(); }, ERR_OK);
    int ret = cmdQ_->EnqueueTask(pauseTask);
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    ret = pauseTask->GetResult();
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    return ERR_OK;
}

int32_t RecorderPipelineCtrler::Resume()
{
    MEDIA_LOGD("enter");

    if (pipeline_ == nullptr) {
        MEDIA_LOGE("Null pipeline !");
        return ERR_NO_INIT;
    }

    auto resumeTask = std::make_shared<TaskHandler>([this] { return pipeline_->Resume(); }, ERR_OK);
    int ret = cmdQ_->EnqueueTask(resumeTask);
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    ret = resumeTask->GetResult();
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    return ERR_OK;
}

int32_t RecorderPipelineCtrler::Stop(bool isDrainAll)
{
    if (pipeline_ == nullptr) {
        return ERR_OK;
    }

    MEDIA_LOGD("enter");

    auto stopTask = std::make_shared<TaskHandler>(
        [this, isDrainAll] { return pipeline_->Stop(isDrainAll); }, ERR_OK
    );
    int ret = cmdQ_->EnqueueTask(stopTask, true);
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    ret = stopTask->GetResult();
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    return ERR_OK;
}

int32_t RecorderPipelineCtrler::Reset()
{
    MEDIA_LOGD("enter");

    cmdQ_->Stop();
    msgQ_->Stop();
    pipeline_ = nullptr;

    int32_t ret = Init();
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    return ERR_OK;
}

void RecorderPipelineCtrler::Notify(int32_t type, int32_t code, int32_t detail)
{
    MEDIA_LOGI("Receive notify, type: %{public}d, code: %{public}d, detail: %{public}d", type, code, detail);
}
}
}
