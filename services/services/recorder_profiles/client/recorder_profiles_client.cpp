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

#include "recorder_profiles_client.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderProfilesClient"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<RecorderProfilesClient> RecorderProfilesClient::Create(
    const sptr<IStandardRecorderProfilesService> &ipcProxy)
{
    CHECK_AND_RETURN_RET_LOG(ipcProxy != nullptr, nullptr, "ipcProxy is nullptr..");

    std::shared_ptr<RecorderProfilesClient> recorderProfiles = std::make_shared<RecorderProfilesClient>(ipcProxy);
    CHECK_AND_RETURN_RET_LOG(recorderProfiles != nullptr, nullptr, "failed to new RecorderProfilesClient..");

    return recorderProfiles;
}

RecorderProfilesClient::RecorderProfilesClient(const sptr<IStandardRecorderProfilesService> &ipcProxy)
    : recorderProfilesProxy_(ipcProxy)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

RecorderProfilesClient::~RecorderProfilesClient()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (recorderProfilesProxy_ != nullptr) {
        (void)recorderProfilesProxy_->DestroyStub();
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void RecorderProfilesClient::MediaServerDied()
{
    std::lock_guard<std::mutex> lock(mutex_);
    recorderProfilesProxy_ = nullptr;
}

bool RecorderProfilesClient::IsAudioRecoderConfigSupported(const RecorderProfilesData &profile)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProfilesProxy_ != nullptr, false, "recorder_profiles service does not exist.");
    return recorderProfilesProxy_->IsAudioRecoderConfigSupported(profile);
}

bool RecorderProfilesClient::HasVideoRecorderProfile(int32_t sourceId, int32_t qualityLevel)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProfilesProxy_ != nullptr, false, "recorder_profiles service does not exist.");
    return recorderProfilesProxy_->HasVideoRecorderProfile(sourceId, qualityLevel);
}

RecorderProfilesData RecorderProfilesClient::GetVideoRecorderProfileInfo(int32_t sourceId, int32_t qualityLevel)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProfilesProxy_ != nullptr, RecorderProfilesData(),
        "recorder_profiles service does not exist.");
    return recorderProfilesProxy_->GetVideoRecorderProfileInfo(sourceId, qualityLevel);
}

std::vector<RecorderProfilesData> RecorderProfilesClient::GetAudioRecorderCapsInfo()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProfilesProxy_ != nullptr, std::vector<RecorderProfilesData>(),
        "recorder_profiles service does not exist.");
    return recorderProfilesProxy_->GetAudioRecorderCapsInfo();
}

std::vector<RecorderProfilesData> RecorderProfilesClient::GetVideoRecorderCapsInfo()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProfilesProxy_ != nullptr, std::vector<RecorderProfilesData>(),
        "recorder_profiles service does not exist.");
    return recorderProfilesProxy_->GetVideoRecorderCapsInfo();
}
}  // namespace Media
}  // namespace OHOS
