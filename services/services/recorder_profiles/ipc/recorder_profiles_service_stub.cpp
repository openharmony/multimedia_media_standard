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

#include "recorder_profiles_service_stub.h"
#include <unistd.h>
#include "avsharedmemory_ipc.h"
#include "media_errors.h"
#include "media_log.h"
#include "media_server_manager.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderProfilesServiceStub"};
}

namespace OHOS {
namespace Media {
sptr<RecorderProfilesServiceStub> RecorderProfilesServiceStub::Create()
{
    sptr<RecorderProfilesServiceStub> meidaProfileStub = new(std::nothrow) RecorderProfilesServiceStub();
    CHECK_AND_RETURN_RET_LOG(meidaProfileStub != nullptr, nullptr, "failed to new RecorderProfilesServiceStub");
    int32_t ret = meidaProfileStub->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to mediaprofile stub init");
    return meidaProfileStub;
}

RecorderProfilesServiceStub::RecorderProfilesServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

RecorderProfilesServiceStub::~RecorderProfilesServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t RecorderProfilesServiceStub::Init()
{
    mediaProfileServer_ = RecorderProfilesServer::Create();
    CHECK_AND_RETURN_RET_LOG(
        mediaProfileServer_ != nullptr, MSERR_NO_MEMORY, "failed to create RecorderProfilesServer");
    mediaProfileFuncs_[static_cast<int>(RecorderProfilesServiceMsg::RECORDER_PROFILES_IS_AUDIO_RECORDER_SUPPORT)]
        = &RecorderProfilesServiceStub::IsAudioRecoderConfigSupported;
    mediaProfileFuncs_[static_cast<int>(RecorderProfilesServiceMsg::RECORDER_PROFILES_HAS_VIDEO_RECORD_PROFILE)]
        = &RecorderProfilesServiceStub::HasVideoRecorderProfile;
    mediaProfileFuncs_[static_cast<int>(RecorderProfilesServiceMsg::RECORDER_PROFILES_GET_AUDIO_RECORDER_CAPS)]
        = &RecorderProfilesServiceStub::GetAudioRecorderCapsInfo;
    mediaProfileFuncs_[static_cast<int>(RecorderProfilesServiceMsg::RECORDER_PROFILES_GET_VIDEO_RECORDER_CAPS)]
        = &RecorderProfilesServiceStub::GetVideoRecorderCapsInfo;
    mediaProfileFuncs_[static_cast<int>(RecorderProfilesServiceMsg::RECORDER_PROFILES_GET_VIDEO_RECORDER_PROFILE)]
        = &RecorderProfilesServiceStub::GetVideoRecorderProfileInfo;
    mediaProfileFuncs_[static_cast<int>(RecorderProfilesServiceMsg::RECORDER_PROFILES_DESTROY)]
        = &RecorderProfilesServiceStub::DestroyStub;
    return MSERR_OK;
}

int32_t RecorderProfilesServiceStub::DestroyStub()
{
    mediaProfileServer_ = nullptr;
    MediaServerManager::GetInstance().DestroyStubObject(MediaServerManager::RECORDERPROFILES, AsObject());
    return MSERR_OK;
}

int RecorderProfilesServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    MEDIA_LOGI("Stub: OnRemoteRequest of code: %{public}u is received", code);

    auto remoteDescriptor = data.ReadInterfaceToken();
    if (RecorderProfilesServiceStub::GetDescriptor() != remoteDescriptor) {
        MEDIA_LOGE("Invalid descriptor");
        return MSERR_INVALID_OPERATION;
    }

    auto itFunc = mediaProfileFuncs_.find(code);
    if (itFunc != mediaProfileFuncs_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            int32_t ret = (this->*memberFunc)(data, reply);
            if (ret != MSERR_OK) {
                MEDIA_LOGE("calling memberFunc is failed.");
            }
            return MSERR_OK;
        }
    }
    MEDIA_LOGW("RecorderProfilesServiceStub: no member func supporting, applying default process");

    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

bool RecorderProfilesServiceStub::IsAudioRecoderConfigSupported(const RecorderProfilesData &profile)
{
    CHECK_AND_RETURN_RET_LOG(mediaProfileServer_ != nullptr, false, "recorder_profiles server is nullptr");
    return mediaProfileServer_->IsAudioRecoderConfigSupported(profile);
}

bool RecorderProfilesServiceStub::HasVideoRecorderProfile(int32_t sourceId, int32_t qualityLevel)
{
    CHECK_AND_RETURN_RET_LOG(mediaProfileServer_ != nullptr, false, "recorder_profiles server is nullptr");
    return mediaProfileServer_->HasVideoRecorderProfile(sourceId, qualityLevel);
}

RecorderProfilesData RecorderProfilesServiceStub::GetVideoRecorderProfileInfo(int32_t sourceId, int32_t qualityLevel)
{
    RecorderProfilesData data;
    CHECK_AND_RETURN_RET_LOG(mediaProfileServer_ != nullptr, data, "recorder_profiles server is nullptr");
    return mediaProfileServer_->GetVideoRecorderProfileInfo(sourceId, qualityLevel);
}

std::vector<RecorderProfilesData> RecorderProfilesServiceStub::GetAudioRecorderCapsInfo()
{
    CHECK_AND_RETURN_RET_LOG(
        mediaProfileServer_ != nullptr, std::vector<RecorderProfilesData>(), "recorder_profiles server is nullptr");
    return mediaProfileServer_->GetAudioRecorderCapsInfo();
}

std::vector<RecorderProfilesData> RecorderProfilesServiceStub::GetVideoRecorderCapsInfo()
{
    CHECK_AND_RETURN_RET_LOG(
        mediaProfileServer_ != nullptr, std::vector<RecorderProfilesData>(), "recorder_profiles server is nullptr");
    return mediaProfileServer_->GetVideoRecorderCapsInfo();
}

int32_t RecorderProfilesServiceStub::IsAudioRecoderConfigSupported(MessageParcel &data, MessageParcel &reply)
{
    RecorderProfilesData profile;
    (void)RecorderProfilesParcel::Unmarshalling(data, profile);
    reply.WriteBool(IsAudioRecoderConfigSupported(profile));
    return MSERR_OK;
}

int32_t RecorderProfilesServiceStub::HasVideoRecorderProfile(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t qualityLevel = data.ReadInt32();
    reply.WriteBool(HasVideoRecorderProfile(sourceId, qualityLevel));
    return MSERR_OK;
}

int32_t RecorderProfilesServiceStub::GetVideoRecorderProfileInfo(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t qualityLevel = data.ReadInt32();
    RecorderProfilesData capabilityData = GetVideoRecorderProfileInfo(sourceId, qualityLevel);
    (void)RecorderProfilesParcel::Marshalling(reply, capabilityData);
    return MSERR_OK;
}

int32_t RecorderProfilesServiceStub::GetAudioRecorderCapsInfo(MessageParcel &data, MessageParcel &reply)
{
    std::string configFile = data.ReadString();
    std::vector<RecorderProfilesData> capabilityDataArray = GetAudioRecorderCapsInfo();
    (void)RecorderProfilesParcel::Marshalling(reply, capabilityDataArray);
    return MSERR_OK;
}

int32_t RecorderProfilesServiceStub::GetVideoRecorderCapsInfo(MessageParcel &data, MessageParcel &reply)
{
    std::string configFile = data.ReadString();
    std::vector<RecorderProfilesData> capabilityDataArray = GetVideoRecorderCapsInfo();
    (void)RecorderProfilesParcel::Marshalling(reply, capabilityDataArray);
    return MSERR_OK;
}

int32_t RecorderProfilesServiceStub::DestroyStub(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(DestroyStub());
    return MSERR_OK;
}
}  // namespace Media
}  // namespace OHOS
