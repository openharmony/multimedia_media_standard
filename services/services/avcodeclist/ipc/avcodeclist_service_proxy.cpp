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

#include "avcodeclist_service_proxy.h"
#include "avsharedmemory_ipc.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecListServiceProxy"};
}

namespace OHOS {
namespace Media {
AVCodecListServiceProxy::AVCodecListServiceProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardAVCodecListService>(impl)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecListServiceProxy::~AVCodecListServiceProxy()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

std::string AVCodecListServiceProxy::FindVideoDecoder(const Format &format)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(AVCodecListServiceProxy::GetDescriptor())) {
        MEDIA_LOGE("Failed to write descriptor");
        return "";
    }

    (void)MediaParcel::Marshalling(data, format);
    int32_t ret = Remote()->SendRequest(FIND_VIDEO_DECODER, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, "", "FindVideoDecoder failed");
    return reply.ReadString();
}

std::string AVCodecListServiceProxy::FindVideoEncoder(const Format &format)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(AVCodecListServiceProxy::GetDescriptor())) {
        MEDIA_LOGE("Failed to write descriptor");
        return "";
    }

    (void)MediaParcel::Marshalling(data, format);
    int32_t ret = Remote()->SendRequest(FIND_VIDEO_ENCODER, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, "", "FindVideoEncoder failed");
    return reply.ReadString();
}

std::string AVCodecListServiceProxy::FindAudioDecoder(const Format &format)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(AVCodecListServiceProxy::GetDescriptor())) {
        MEDIA_LOGE("Failed to write descriptor");
        return "";
    }

    (void)MediaParcel::Marshalling(data, format);
    int32_t ret = Remote()->SendRequest(FIND_AUDIO_DECODER, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, "", "FindAudioDecoder failed");

    return reply.ReadString();
}

std::string AVCodecListServiceProxy::FindAudioEncoder(const Format &format)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(AVCodecListServiceProxy::GetDescriptor())) {
        MEDIA_LOGE("Failed to write descriptor");
        return "";
    }

    (void)MediaParcel::Marshalling(data, format);
    int32_t ret = Remote()->SendRequest(FIND_AUDIO_ENCODER, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, "", "FindAudioEncoder failed");

    return reply.ReadString();
}

std::vector<CapabilityData> AVCodecListServiceProxy::GetCodecCapabilityInfos()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    Format profileFormat;
    std::vector<CapabilityData> capabilityDataArray;

    if (!data.WriteInterfaceToken(AVCodecListServiceProxy::GetDescriptor())) {
        MEDIA_LOGE("Failed to write descriptor");
        return capabilityDataArray;
    }

    int32_t ret = Remote()->SendRequest(GET_CAPABILITY_INFOS, data, reply, option);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("GetCodecCapabilityInfos failed, error: %{public}d", ret);
    }
    (void)AVCodecListParcel::Unmarshalling(reply, capabilityDataArray); // MessageParcel to std::vector<CapabilityData>

    return capabilityDataArray;
}

int32_t AVCodecListServiceProxy::DestroyStub()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(AVCodecListServiceProxy::GetDescriptor())) {
        MEDIA_LOGE("Failed to write descriptor");
        return MSERR_UNKNOWN;
    }

    int32_t ret = Remote()->SendRequest(DESTROY, data, reply, option);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("destroy failed, error: %{public}d", ret);
    }
    return reply.ReadInt32();
}
} // namespace Media
} // namespace OHOS
