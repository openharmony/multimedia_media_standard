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

#include "avmuxer_client.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMuxerClient"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<AVMuxerClient> AVMuxerClient::Create(const sptr<IStandardAVMuxerService> &ipcProxy)
{
    std::shared_ptr<AVMuxerClient> avmuxerClient = std::make_shared<AVMuxerClient>(ipcProxy);
    CHECK_AND_RETURN_RET_LOG(avmuxerClient != nullptr, nullptr, "Failed to create avmuxer client");
    return avmuxerClient;
}

AVMuxerClient::AVMuxerClient(const sptr<IStandardAVMuxerService> &ipcProxy)
    : avmuxerProxy_(ipcProxy)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVMuxerClient::~AVMuxerClient()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (avmuxerProxy_ != nullptr) {
        (void)avmuxerProxy_->DestroyStub();
        avmuxerProxy_ = nullptr;
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void AVMuxerClient::MediaServerDied()
{
    std::lock_guard<std::mutex> lock(mutex_);
    avmuxerProxy_ = nullptr;
}

std::vector<std::string> AVMuxerClient::GetAVMuxerFormatList()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avmuxerProxy_ != nullptr, std::vector<std::string>(), "AVMuxer Service does not exist");
    return avmuxerProxy_->GetAVMuxerFormatList();
}

int32_t AVMuxerClient::SetOutput(int32_t fd, const std::string &format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avmuxerProxy_ != nullptr, MSERR_NO_MEMORY, "AVMuxer Service does not exist");
    return avmuxerProxy_->SetOutput(fd, format);
}

int32_t AVMuxerClient::SetLocation(float latitude, float longitude)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avmuxerProxy_ != nullptr, MSERR_NO_MEMORY, "AVMuxer Service does not exist");
    return avmuxerProxy_->SetLocation(latitude, longitude);
}

int32_t AVMuxerClient::SetRotation(int32_t rotation)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avmuxerProxy_ != nullptr, MSERR_NO_MEMORY, "AVMuxer Service does not exist");
    return avmuxerProxy_->SetRotation(rotation);
}

int32_t AVMuxerClient::AddTrack(const MediaDescription &trackDesc, int32_t &trackId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avmuxerProxy_ != nullptr, MSERR_NO_MEMORY, "AVMuxer Service does not exist");
    return avmuxerProxy_->AddTrack(trackDesc, trackId);
}

int32_t AVMuxerClient::Start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avmuxerProxy_ != nullptr, MSERR_NO_MEMORY, "AVMuxer Service does not exist");
    return avmuxerProxy_->Start();
}

int32_t AVMuxerClient::WriteTrackSample(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(sampleData != nullptr, MSERR_INVALID_VAL, "sampleData is nullptr");
    CHECK_AND_RETURN_RET_LOG(avmuxerProxy_ != nullptr, MSERR_NO_MEMORY, "AVMuxer Service does not exist");
    return avmuxerProxy_->WriteTrackSample(sampleData, sampleInfo);
}

int32_t AVMuxerClient::Stop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avmuxerProxy_ != nullptr, MSERR_NO_MEMORY, "AVMuxer Service does not exist");
    return avmuxerProxy_->Stop();
}

void AVMuxerClient::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(avmuxerProxy_ != nullptr, "AVMuxer Service does not exist");
    avmuxerProxy_->Release();
}
}  // namespace Media
}  // namespace OHOS