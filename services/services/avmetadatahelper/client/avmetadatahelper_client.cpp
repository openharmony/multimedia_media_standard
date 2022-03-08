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

#include "avmetadatahelper_client.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMetadataHelperClient"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<AVMetadataHelperClient> AVMetadataHelperClient::Create(
    const sptr<IStandardAVMetadataHelperService> &ipcProxy)
{
    std::shared_ptr<AVMetadataHelperClient> AVMetadataHelper = std::make_shared<AVMetadataHelperClient>(ipcProxy);
    return AVMetadataHelper;
}

AVMetadataHelperClient::AVMetadataHelperClient(const sptr<IStandardAVMetadataHelperService> &ipcProxy)
    : avMetadataHelperProxy_(ipcProxy)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVMetadataHelperClient::~AVMetadataHelperClient()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (avMetadataHelperProxy_ != nullptr) {
        (void)avMetadataHelperProxy_->DestroyStub();
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void AVMetadataHelperClient::MediaServerDied()
{
    std::lock_guard<std::mutex> lock(mutex_);
    avMetadataHelperProxy_ = nullptr;
}

int32_t AVMetadataHelperClient::SetSource(const std::string &uri, int32_t usage)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperProxy_ != nullptr, MSERR_NO_MEMORY,
        "avmetadatahelper service does not exist.");
    return avMetadataHelperProxy_->SetSource(uri, usage);
}

int32_t AVMetadataHelperClient::SetSource(int32_t fd, int64_t offset, int64_t size, int32_t usage)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperProxy_ != nullptr, MSERR_NO_MEMORY,
        "avmetadatahelper service does not exist.");
    return avMetadataHelperProxy_->SetSource(fd, offset, size, usage);
}

std::string AVMetadataHelperClient::ResolveMetadata(int32_t key)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperProxy_ != nullptr, "", "avmetadatahelper service does not exist.");
    return avMetadataHelperProxy_->ResolveMetadata(key);
}

std::unordered_map<int32_t, std::string> AVMetadataHelperClient::ResolveMetadata()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperProxy_ != nullptr, {}, "avmetadatahelper service does not exist.");
    return avMetadataHelperProxy_->ResolveMetadataMap();
}

std::shared_ptr<AVSharedMemory> AVMetadataHelperClient::FetchArtPicture()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperProxy_ != nullptr, {}, "avmetadatahelper service does not exist.");
    return avMetadataHelperProxy_->FetchArtPicture();
}

std::shared_ptr<AVSharedMemory> AVMetadataHelperClient::FetchFrameAtTime(int64_t timeUs, int32_t option,
    const OutputConfiguration &param)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperProxy_ != nullptr, nullptr, "avmetadatahelper service does not exist.");
    return avMetadataHelperProxy_->FetchFrameAtTime(timeUs, option, param);
}

void AVMetadataHelperClient::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(avMetadataHelperProxy_ != nullptr, "avmetadatahelper service does not exist.");
    avMetadataHelperProxy_->Release();
}
} // namespace Media
} // namespace OHOS