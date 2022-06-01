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

#include "recorder_profiles_service_proxy.h"
#include "avsharedmemory_ipc.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderProfilesServiceProxy"};
}

namespace OHOS {
namespace Media {
RecorderProfilesServiceProxy::RecorderProfilesServiceProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardRecorderProfilesService>(impl)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

RecorderProfilesServiceProxy::~RecorderProfilesServiceProxy()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

bool RecorderProfilesServiceProxy::IsAudioRecoderConfigSupported(const RecorderProfilesData &profile)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(RecorderProfilesServiceProxy::GetDescriptor())) {
        MEDIA_LOGE("Failed to write descriptor");
        return false;
    }

    (void)RecorderProfilesParcel::Marshalling(data, profile);
    uint32_t code = static_cast<uint32_t>(RecorderProfilesServiceMsg::RECORDER_PROFILES_IS_AUDIO_RECORDER_SUPPORT);
    int32_t ret = Remote()->SendRequest(code, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, false, "IsAudioRecoderConfigSupported failed");
    return reply.ReadBool();
}

bool RecorderProfilesServiceProxy::HasVideoRecorderProfile(int32_t sourceId, int32_t qualityLevel)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(RecorderProfilesServiceProxy::GetDescriptor())) {
        MEDIA_LOGE("Failed to write descriptor");
        return false;
    }

    data.WriteInt32(sourceId);
    data.WriteInt32(qualityLevel);
    uint32_t code = static_cast<uint32_t>(RecorderProfilesServiceMsg::RECORDER_PROFILES_HAS_VIDEO_RECORD_PROFILE);
    int32_t ret  = Remote()->SendRequest(code, data, reply, option);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("HasVideoRecorderProfile failed, error: %{public}d", ret);
        return false;
    }
    return reply.ReadBool();
}

RecorderProfilesData RecorderProfilesServiceProxy::GetVideoRecorderProfileInfo(int32_t sourceId, int32_t qualityLevel)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    RecorderProfilesData capabilityData;

    if (!data.WriteInterfaceToken(RecorderProfilesServiceProxy::GetDescriptor())) {
        MEDIA_LOGE("Failed to write descriptor");
        return capabilityData;
    }

    data.WriteInt32(sourceId);
    data.WriteInt32(qualityLevel);

    uint32_t code = static_cast<uint32_t>(RecorderProfilesServiceMsg::RECORDER_PROFILES_GET_VIDEO_RECORDER_PROFILE);
    int32_t ret = Remote()->SendRequest(code, data, reply, option);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("GetVideoRecorderProfileInfo failed, error: %{public}d", ret);
    }
    (void)RecorderProfilesParcel::Unmarshalling(reply, capabilityData);
    return capabilityData;
}

std::vector<RecorderProfilesData> RecorderProfilesServiceProxy::GetAudioRecorderCapsInfo()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    std::vector<RecorderProfilesData> capabilityDataArray;

    if (!data.WriteInterfaceToken(RecorderProfilesServiceProxy::GetDescriptor())) {
        MEDIA_LOGE("Failed to write descriptor");
        return capabilityDataArray;
    }

    uint32_t code = static_cast<uint32_t>(RecorderProfilesServiceMsg::RECORDER_PROFILES_GET_AUDIO_RECORDER_CAPS);
    int32_t ret = Remote()->SendRequest(code, data, reply, option);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("GetAudioRecorderCapsInfo failed, error: %{public}d", ret);
    }
    (void)RecorderProfilesParcel::Unmarshalling(reply, capabilityDataArray);
    return capabilityDataArray;
}

std::vector<RecorderProfilesData> RecorderProfilesServiceProxy::GetVideoRecorderCapsInfo()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    std::vector<RecorderProfilesData> capabilityDataArray;

    if (!data.WriteInterfaceToken(RecorderProfilesServiceProxy::GetDescriptor())) {
        MEDIA_LOGE("Failed to write descriptor");
        return capabilityDataArray;
    }

    uint32_t code = static_cast<uint32_t>(RecorderProfilesServiceMsg ::RECORDER_PROFILES_GET_VIDEO_RECORDER_CAPS);
    int32_t ret = Remote()->SendRequest(code, data, reply, option);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("GetVideoRecorderCapsInfo failed, error: %{public}d", ret);
    }
    (void)RecorderProfilesParcel::Unmarshalling(reply, capabilityDataArray);
    return capabilityDataArray;
}

int32_t RecorderProfilesServiceProxy::DestroyStub()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(RecorderProfilesServiceProxy::GetDescriptor())) {
        MEDIA_LOGE("Failed to write descriptor");
        return MSERR_UNKNOWN;
    }

    uint32_t code = static_cast<uint32_t>(RecorderProfilesServiceMsg::RECORDER_PROFILES_DESTROY);
    int32_t ret = Remote()->SendRequest(code, data, reply, option);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("destroy failed, error: %{public}d", ret);
    }
    return reply.ReadInt32();
}
}  // namespace Media
}  // namespace OHOS
