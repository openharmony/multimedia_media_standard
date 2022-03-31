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

#include "avmuxer_impl.h"
#include "securec.h"
#include "i_media_service.h"
#include "media_log.h"
#include "media_errors.h"
#include "avsharedmemorybase.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMuxerImpl"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<AVMuxer> AVMuxerFactory::CreateAVMuxer()
{
    std::shared_ptr<AVMuxerImpl> impl = std::make_shared<AVMuxerImpl>();
    CHECK_AND_RETURN_RET_LOG(impl != nullptr, nullptr, "Failed to create avmuxer implementation");

    int32_t ret = impl->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "Failed to init avmuxer implementation");
    return impl;
}

int32_t AVMuxerImpl::Init()
{
    avmuxerService_ = MediaServiceFactory::GetInstance().CreateAVMuxerService();
    CHECK_AND_RETURN_RET_LOG(avmuxerService_ != nullptr, MSERR_NO_MEMORY, "Failed to create avmuxer service");
    return MSERR_OK;
}

AVMuxerImpl::AVMuxerImpl()
{
    MEDIA_LOGD("AVMuxerImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVMuxerImpl::~AVMuxerImpl()
{
    if (avmuxerService_ != nullptr) {
        (void)MediaServiceFactory::GetInstance().DestroyAVMuxerService(avmuxerService_);
        avmuxerService_ = nullptr;
    }
    MEDIA_LOGD("AVMuxerImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

std::vector<std::string> AVMuxerImpl::GetAVMuxerFormatList()
{
    CHECK_AND_RETURN_RET_LOG(avmuxerService_ != nullptr, std::vector<std::string>(), "AVMuxer Service does not exist");
    return avmuxerService_->GetAVMuxerFormatList();
}

int32_t AVMuxerImpl::SetOutput(int32_t fd, const std::string &format)
{
    CHECK_AND_RETURN_RET_LOG(avmuxerService_ != nullptr, MSERR_INVALID_OPERATION, "AVMuxer Service does not exist");
    CHECK_AND_RETURN_RET_LOG(fd > 0, MSERR_INVALID_VAL, "Fd is invalid");
    CHECK_AND_RETURN_RET_LOG(!format.empty(), MSERR_INVALID_VAL, "Format is empty");

    return avmuxerService_->SetOutput(fd, format);
}

int32_t AVMuxerImpl::SetLocation(float latitude, float longitude)
{
    CHECK_AND_RETURN_RET_LOG(avmuxerService_ != nullptr, MSERR_INVALID_OPERATION, "AVMuxer Service does not exist");
    return avmuxerService_->SetLocation(latitude, longitude);
}

int32_t AVMuxerImpl::SetOrientationHint(int32_t degrees)
{
    CHECK_AND_RETURN_RET_LOG(avmuxerService_ != nullptr, MSERR_INVALID_OPERATION, "AVMuxer Service does not exist");
    return avmuxerService_->SetOrientationHint(degrees);
}

int32_t AVMuxerImpl::AddTrack(const MediaDescription &trackDesc, int32_t &trackId)
{
    CHECK_AND_RETURN_RET_LOG(avmuxerService_ != nullptr, MSERR_INVALID_OPERATION, "AVMuxer Service does not exist");
    return avmuxerService_->AddTrack(trackDesc, trackId);
}

int32_t AVMuxerImpl::Start()
{
    CHECK_AND_RETURN_RET_LOG(avmuxerService_ != nullptr, MSERR_INVALID_OPERATION, "AVMuxer Service does not exist");
    return avmuxerService_->Start();
}

int32_t AVMuxerImpl::WriteTrackSample(std::shared_ptr<AVMemory> sampleData, const TrackSampleInfo &info)
{
    CHECK_AND_RETURN_RET_LOG(avmuxerService_ != nullptr, MSERR_INVALID_OPERATION, "AVMuxer Service does not exist");
    CHECK_AND_RETURN_RET_LOG(sampleData != nullptr && sampleData->Offset() + sampleData->Size() <= sampleData->Capacity() &&
                             sampleData->Offset() + sampleData->Size() >= sampleData->Size(),
                             MSERR_INVALID_VAL, "Invalid memory");
    MEDIA_LOGD("sampleData->Capacity is: %{public}u, sampleData->Size is: %{public}u,"
               "sampleData->Data is: %{public}s, sampleData->Base is: %{public}s",
                sampleData->Capacity(), sampleData->Size(), sampleData->Data(), sampleData->Base());
    std::shared_ptr<AVSharedMemoryBase> avSharedMem = 
        std::make_shared<AVSharedMemoryBase>(sampleData->Size(), AVSharedMemory::FLAGS_READ_ONLY, "sampleData");
    int32_t ret = avSharedMem->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_NO_MEMORY, "Failed to create AVSharedMemoryBase");
    errno_t rc = memcpy_s(avSharedMem->GetBase(), avSharedMem->GetSize(), sampleData->Data(), sampleData->Size());
    CHECK_AND_RETURN_RET_LOG(rc == EOK, MSERR_UNKNOWN, "memcpy_s failed");
    return avmuxerService_->WriteTrackSample(avSharedMem, info);
}

int32_t AVMuxerImpl::Stop()
{
    CHECK_AND_RETURN_RET_LOG(avmuxerService_ != nullptr, MSERR_INVALID_OPERATION, "AVMuxer Service does not exist");
    return avmuxerService_->Stop();
}

void AVMuxerImpl::Release()
{
    CHECK_AND_RETURN_LOG(avmuxerService_ != nullptr, "AVMuxer Service does not exist");
    (void)avmuxerService_->Release();
    (void)MediaServiceFactory::GetInstance().DestroyAVMuxerService(avmuxerService_);
    avmuxerService_ = nullptr;
}
}  // namespace Media
}  // namespace OHOS