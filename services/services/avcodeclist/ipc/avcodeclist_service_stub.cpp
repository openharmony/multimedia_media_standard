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

#include "avcodeclist_service_stub.h"
#include <unistd.h>
#include "avsharedmemory_ipc.h"
#include "media_errors.h"
#include "media_log.h"
#include "media_server_manager.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecListServiceStub"};
}

namespace OHOS {
namespace Media {
sptr<AVCodecListServiceStub> AVCodecListServiceStub::Create()
{
    sptr<AVCodecListServiceStub> codecListStub = new(std::nothrow) AVCodecListServiceStub();
    CHECK_AND_RETURN_RET_LOG(codecListStub != nullptr, nullptr, "failed to new AVCodecListServiceStub");

    int32_t ret = codecListStub->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to codeclist stub init");
    return codecListStub;
}

AVCodecListServiceStub::AVCodecListServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecListServiceStub::~AVCodecListServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVCodecListServiceStub::Init()
{
    codecListServer_ = AVCodecListServer::Create();
    CHECK_AND_RETURN_RET_LOG(codecListServer_ != nullptr, MSERR_NO_MEMORY, "failed to create AVCodecListServer");
    codecListFuncs_[FIND_VIDEO_DECODER] = &AVCodecListServiceStub::FindVideoDecoder;
    codecListFuncs_[FIND_VIDEO_ENCODER] = &AVCodecListServiceStub::FindVideoEncoder;
    codecListFuncs_[FIND_AUDIO_DECODER] = &AVCodecListServiceStub::FindAudioDecoder;
    codecListFuncs_[FIND_AUDIO_ENCODER] = &AVCodecListServiceStub::FindAudioEncoder;
    codecListFuncs_[GET_CAPABILITY_INFOS] = &AVCodecListServiceStub::GetCodecCapabilityInfos;
    codecListFuncs_[DESTROY] = &AVCodecListServiceStub::DestroyStub;
    return MSERR_OK;
}

int32_t AVCodecListServiceStub::DestroyStub()
{
    codecListServer_ = nullptr;

    MediaServerManager::GetInstance().DestroyStubObject(MediaServerManager::AVCODECLIST, AsObject());
    return MSERR_OK;
}

int AVCodecListServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    MEDIA_LOGI("Stub: OnRemoteRequest of code: %{public}u is received", code);

    auto remoteDescriptor = data.ReadInterfaceToken();
    if (AVCodecListServiceStub::GetDescriptor() != remoteDescriptor) {
        MEDIA_LOGE("Invalid descriptor");
        return MSERR_INVALID_OPERATION;
    }

    auto itFunc = codecListFuncs_.find(code);
    if (itFunc != codecListFuncs_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            int32_t ret = (this->*memberFunc)(data, reply);
            if (ret != MSERR_OK) {
                MEDIA_LOGE("calling memberFunc is failed.");
            }
            return MSERR_OK;
        }
    }
    MEDIA_LOGW("AVCodecListServiceStub: no member func supporting, applying default process");

    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

std::string AVCodecListServiceStub::FindVideoDecoder(const Format &format)
{
    CHECK_AND_RETURN_RET_LOG(codecListServer_ != nullptr, "", "avcodeclist server is nullptr");
    return codecListServer_->FindVideoDecoder(format);
}

std::string AVCodecListServiceStub::FindVideoEncoder(const Format &format)
{
    CHECK_AND_RETURN_RET_LOG(codecListServer_ != nullptr, "", "avcodeclist server is nullptr");
    return codecListServer_->FindVideoEncoder(format);
}

std::string AVCodecListServiceStub::FindAudioDecoder(const Format &format)
{
    CHECK_AND_RETURN_RET_LOG(codecListServer_ != nullptr, "", "avcodeclist server is nullptr");
    return codecListServer_->FindAudioDecoder(format);
}

std::string AVCodecListServiceStub::FindAudioEncoder(const Format &format)
{
    CHECK_AND_RETURN_RET_LOG(codecListServer_ != nullptr, "", "avcodeclist server is nullptr");
    return codecListServer_->FindAudioEncoder(format);
}

std::vector<CapabilityData> AVCodecListServiceStub::GetCodecCapabilityInfos()
{
    CHECK_AND_RETURN_RET_LOG(codecListServer_ != nullptr, std::vector<CapabilityData>(),
        "avcodeclist server is nullptr");
    return codecListServer_->GetCodecCapabilityInfos();
}

int32_t AVCodecListServiceStub::FindVideoDecoder(MessageParcel &data, MessageParcel &reply)
{
    Format format;
    (void)MediaParcel::Unmarshalling(data, format);
    reply.WriteString(FindVideoDecoder(format));
    return MSERR_OK;
}

int32_t AVCodecListServiceStub::FindVideoEncoder(MessageParcel &data, MessageParcel &reply)
{
    Format format;
    (void)MediaParcel::Unmarshalling(data, format);
    reply.WriteString(FindVideoEncoder(format));
    return MSERR_OK;
}

int32_t AVCodecListServiceStub::FindAudioDecoder(MessageParcel &data, MessageParcel &reply)
{
    Format format;
    (void)MediaParcel::Unmarshalling(data, format);
    reply.WriteString(FindAudioDecoder(format));
    return MSERR_OK;
}

int32_t AVCodecListServiceStub::FindAudioEncoder(MessageParcel &data, MessageParcel &reply)
{
    Format format;
    (void)MediaParcel::Unmarshalling(data, format);
    reply.WriteString(FindAudioEncoder(format));
    return MSERR_OK;
}

int32_t AVCodecListServiceStub::GetCodecCapabilityInfos(MessageParcel &data, MessageParcel &reply)
{
    std::string configFile = data.ReadString();
    std::vector<CapabilityData> capabilityDataArray = GetCodecCapabilityInfos();
    (void)AVCodecListParcel::Marshalling(reply, capabilityDataArray); // vector<CapabilityData> to MessageParcel

    return MSERR_OK;
}

int32_t AVCodecListServiceStub::DestroyStub(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(DestroyStub());
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
