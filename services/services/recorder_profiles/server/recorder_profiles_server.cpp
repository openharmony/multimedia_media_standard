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

#include "recorder_profiles_server.h"
#include "media_log.h"
#include "media_errors.h"
#include "engine_factory_repo.h"
#include "recorder_profiles_ability_singleton.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderProfilesServer"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<IRecorderProfilesService> RecorderProfilesServer::Create()
{
    std::shared_ptr<RecorderProfilesServer> server = std::make_shared<RecorderProfilesServer>();
    return server;
}

RecorderProfilesServer::RecorderProfilesServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

RecorderProfilesServer::~RecorderProfilesServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

bool RecorderProfilesServer::IsAudioRecoderConfigSupported(const RecorderProfilesData &profile)
{
    std::lock_guard<std::mutex> lock(mutex_);
    bool ret = false;
    std::vector<RecorderProfilesData> capabilityDataArray = GetCapabilityInfos();
    for (auto iter = capabilityDataArray.begin(); iter != capabilityDataArray.end(); ++iter) {
        RecorderProfilesData compareProfileData = (*iter);
        if (compareProfileData.mediaProfileType == RECORDER_TYPE_PROFILE) {
            if (CompareProfile(compareProfileData, profile)) {
                ret = true;
                break;
            }
        }
    }
    return ret;
}

bool RecorderProfilesServer::HasVideoRecorderProfile(int32_t sourceId, int32_t qualityLevel)
{
    std::lock_guard<std::mutex> lock(mutex_);
    bool ret = false;
    std::vector<RecorderProfilesData> capabilityDataArray = GetCapabilityInfos();
    for (auto iter = capabilityDataArray.begin(); iter != capabilityDataArray.end(); ++iter) {
        if (iter->mediaProfileType == RECORDER_TYPE_PROFILE) {
            if ((sourceId == iter->sourceId) && (qualityLevel == iter->recorderProfile.qualityLevel)) {
                ret = true;
                break;
            }
        }
    }
    return ret;
}

RecorderProfilesData RecorderProfilesServer::GetVideoRecorderProfileInfo(int32_t sourceId, int32_t qualityLevel)
{
    std::lock_guard<std::mutex> lock(mutex_);
    RecorderProfilesData capabilityData;
    std::vector<RecorderProfilesData> capabilityDataArray = GetCapabilityInfos();
    for (auto iter = capabilityDataArray.begin(); iter != capabilityDataArray.end(); ++iter) {
        if (iter->mediaProfileType == RECORDER_TYPE_PROFILE) {
            if ((sourceId == iter->sourceId) && (qualityLevel == iter->recorderProfile.qualityLevel)) {
                capabilityData = (*iter);
                break;
            }
        }
    }
    return capabilityData;
}

std::vector<RecorderProfilesData> RecorderProfilesServer::GetAudioRecorderCapsInfo()
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RecorderProfilesData> capabilityDataArray = GetCapabilityInfos();
    for (auto iter = capabilityDataArray.begin(); iter != capabilityDataArray.end();) {
        if (iter->mediaProfileType == RECORDER_TYPE_AUDIO_CAPS) {
            ++iter;
        } else {
            iter = capabilityDataArray.erase(iter);
        }
    }
    return capabilityDataArray;
}

std::vector<RecorderProfilesData> RecorderProfilesServer::GetVideoRecorderCapsInfo()
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RecorderProfilesData> capabilityDataArray = GetCapabilityInfos();
    for (auto iter = capabilityDataArray.begin(); iter != capabilityDataArray.end();) {
        if (iter->mediaProfileType == RECORDER_TYPE_VIDEO_CAPS) {
            ++iter;
        } else {
            iter = capabilityDataArray.erase(iter);
        }
    }
    return capabilityDataArray;
}

std::vector<RecorderProfilesData> RecorderProfilesServer::GetCapabilityInfos()
{
    RecorderProfilesAbilitySingleton& mediaProfileAbilityInstance = RecorderProfilesAbilitySingleton::GetInstance();
    return mediaProfileAbilityInstance.GetCapabilityDataArray();
}

bool RecorderProfilesServer::CompareProfile(
    const RecorderProfilesData &compareProfile, const RecorderProfilesData &profile)
{
    bool ret = false;
    if ((profile.recorderProfile.containerFormatType == compareProfile.recorderProfile.containerFormatType) &&
        (profile.recorderProfile.audioCodec == compareProfile.recorderProfile.audioCodec) &&
        (profile.recorderProfile.audioBitrate == compareProfile.recorderProfile.audioBitrate) &&
        (profile.recorderProfile.audioSampleRate == compareProfile.recorderProfile.audioSampleRate) &&
        (profile.recorderProfile.audioChannels == compareProfile.recorderProfile.audioChannels)) {
        ret = true;
    }
    return ret;
}
}  // namespace Media
}  // namespace OHOS
