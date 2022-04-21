/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "avmuxer_server.h"
#include <unistd.h>
#include <fcntl.h>
#include "media_errors.h"
#include "media_log.h"
#include "engine_factory_repo.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMuxerServer"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<IAVMuxerService> AVMuxerServer::Create()
{
    std::shared_ptr<AVMuxerServer> avmuxerServer = std::make_shared<AVMuxerServer>();
    CHECK_AND_RETURN_RET_LOG(avmuxerServer != nullptr, nullptr, "AVMuxer Service does not exist");
    int32_t ret = avmuxerServer->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "Failed to init avmuxer server");
    return avmuxerServer;
}

AVMuxerServer::AVMuxerServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVMuxerServer::~AVMuxerServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    std::lock_guard<std::mutex> lock(mutex_);
    avmuxerEngine_ = nullptr;
}

int32_t AVMuxerServer::Init()
{
    auto engineFactory = EngineFactoryRepo::Instance().GetEngineFactory(IEngineFactory::Scene::SCENE_AVMUXER);
    CHECK_AND_RETURN_RET_LOG(engineFactory != nullptr, MSERR_CREATE_AVMUXER_ENGINE_FAILED,
        "Failed to get engine factory");
    avmuxerEngine_ = engineFactory->CreateAVMuxerEngine();
    CHECK_AND_RETURN_RET_LOG(avmuxerEngine_ != nullptr, MSERR_NO_MEMORY, "Failed to create avmuxer engine");
    int32_t ret = avmuxerEngine_->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to init avmuxer engine");

    return MSERR_OK;
}

std::vector<std::string> AVMuxerServer::GetAVMuxerFormatList()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avmuxerEngine_ != nullptr, std::vector<std::string>(),
        "AVMuxer engine does not exist");
    std::vector<std::string> formatList = avmuxerEngine_->GetAVMuxerFormatList();
    return formatList;
}

int32_t AVMuxerServer::SetOutput(int32_t fd, const std::string &format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (curState_ != AVMUXER_IDEL && curState_ != AVMUXER_OUTPUT_SET) {
        MEDIA_LOGE("Failed to call SetOutput, currentState is %{public}d", curState_);
        return MSERR_INVALID_OPERATION;
    }

    CHECK_AND_RETURN_RET_LOG(fd >= 0, MSERR_INVALID_VAL, "failed to get file descriptor");
    int32_t flags = fcntl(fd, F_GETFL);
    CHECK_AND_RETURN_RET_LOG(flags != -1, MSERR_INVALID_VAL, "failed to get file status flags");
    CHECK_AND_RETURN_RET_LOG((static_cast<uint32_t>(flags) & O_WRONLY) == O_WRONLY,
        MSERR_INVALID_VAL, "Failed to check fd")
    CHECK_AND_RETURN_RET_LOG(avmuxerEngine_ != nullptr, MSERR_INVALID_OPERATION, "AVMuxer engine does not exist");
    int32_t ret = avmuxerEngine_->SetOutput(fd, format);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call SetOutput");
    curState_ = AVMUXER_OUTPUT_SET;
    return MSERR_OK;
}

int32_t AVMuxerServer::SetLocation(float latitude, float longitude)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (curState_ != AVMUXER_OUTPUT_SET && curState_ != AVMUXER_PARAMETER_SET) {
        MEDIA_LOGE("Failed to call SetLocation, currentState is %{public}d", curState_);
        return MSERR_INVALID_OPERATION;
    }
    CHECK_AND_RETURN_RET_LOG(avmuxerEngine_ != nullptr, MSERR_INVALID_OPERATION, "AVMuxer engine does not exist");
    int32_t ret = avmuxerEngine_->SetLocation(latitude, longitude);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call SetLocation");
    curState_ = AVMUXER_PARAMETER_SET;
    return MSERR_OK;
}

int32_t AVMuxerServer::SetRotation(int32_t rotation)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (curState_ != AVMUXER_OUTPUT_SET && curState_ != AVMUXER_PARAMETER_SET) {
        MEDIA_LOGE("Failed to call SetRotation, currentState is %{public}d", curState_);
        return MSERR_INVALID_OPERATION;
    }
    CHECK_AND_RETURN_RET_LOG(avmuxerEngine_ != nullptr, MSERR_INVALID_OPERATION, "AVMuxer engine does not exist");
    int32_t ret = avmuxerEngine_->SetRotation(rotation);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call SetRotation");
    curState_ = AVMUXER_PARAMETER_SET;
    return MSERR_OK;
}

int32_t AVMuxerServer::AddTrack(const MediaDescription &trackDesc, int32_t &trackId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (curState_ != AVMUXER_OUTPUT_SET && curState_ != AVMUXER_PARAMETER_SET) {
        MEDIA_LOGE("Failed to call AddTrack, currentState is %{public}d", curState_);
        return MSERR_INVALID_OPERATION;
    }
    CHECK_AND_RETURN_RET_LOG(avmuxerEngine_ != nullptr, MSERR_INVALID_OPERATION, "AVMuxer engine does not exist");
    int32_t ret = avmuxerEngine_->AddTrack(trackDesc, trackId);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call AddTrack");
    curState_ = AVMUXER_PARAMETER_SET;
    trackNum_ += 1;
    return MSERR_OK;
}

int32_t AVMuxerServer::Start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (curState_ != AVMUXER_PARAMETER_SET || trackNum_ == 0) {
        MEDIA_LOGE("Failed to call Start, currentState is %{public}d", curState_);
        return MSERR_INVALID_OPERATION;
    }
    CHECK_AND_RETURN_RET_LOG(avmuxerEngine_ != nullptr, MSERR_INVALID_OPERATION, "AVMuxer engine does not exist");
    int32_t ret = avmuxerEngine_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call Start");
    curState_ = AVMUXER_STARTED;
    return MSERR_OK;
}

int32_t AVMuxerServer::WriteTrackSample(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &info)
{
    CHECK_AND_RETURN_RET_LOG(sampleData != nullptr, MSERR_INVALID_VAL, "sampleData is nullptr");
    std::lock_guard<std::mutex> lock(mutex_);
    if (curState_ != AVMUXER_STARTED && curState_ != AVMUXER_SAMPLE_WRITING) {
        MEDIA_LOGE("Failed to call Start, currentState is %{public}d", curState_);
        return MSERR_INVALID_OPERATION;
    }
    CHECK_AND_RETURN_RET_LOG(avmuxerEngine_ != nullptr, MSERR_INVALID_OPERATION, "AVMuxer engine does not exist");
    int32_t ret = avmuxerEngine_->WriteTrackSample(sampleData, info);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call WriteTrackSample");
    curState_ = AVMUXER_SAMPLE_WRITING;
    return MSERR_OK;
}

int32_t AVMuxerServer::Stop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avmuxerEngine_ != nullptr, MSERR_INVALID_OPERATION, "AVMuxer engine does not exist");
    int32_t ret = avmuxerEngine_->Stop();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call Stop");
    curState_ = AVMUXER_IDEL;
    return MSERR_OK;
}

void AVMuxerServer::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (curState_ != AVMUXER_IDEL) {
        mutex_.unlock();
        Stop();
        mutex_.lock();
    }
    avmuxerEngine_ = nullptr;
}
}  // namespace Media
}  // namespace OHOS