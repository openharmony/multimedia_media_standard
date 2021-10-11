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

#include "avmetadatahelper_service_stub.h"
#include "media_server_manager.h"
#include "media_log.h"
#include "media_errors.h"
#include "avsharedmemory_ipc.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMetadataHelperServiceStub"};
}

namespace OHOS {
namespace Media {
sptr<AVMetadataHelperServiceStub> AVMetadataHelperServiceStub::Create()
{
    sptr<AVMetadataHelperServiceStub> avMetadataHelperStub = new(std::nothrow) AVMetadataHelperServiceStub();
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperStub != nullptr, nullptr, "failed to new AVMetadataHelperServiceStub");

    int32_t ret = avMetadataHelperStub->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to avmetadatahlper stub init");
    return avMetadataHelperStub;
}

AVMetadataHelperServiceStub::AVMetadataHelperServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVMetadataHelperServiceStub::~AVMetadataHelperServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destory", FAKE_POINTER(this));
}

int32_t AVMetadataHelperServiceStub::Init()
{
    avMetadateHelperServer_ = AVMetadataHelperServer::Create();
    CHECK_AND_RETURN_RET_LOG(avMetadateHelperServer_ != nullptr, MSERR_NO_MEMORY,
        "failed to create AVMetadataHelper Service");

    avMetadataHelperFuncs_[SET_SOURCE] = &AVMetadataHelperServiceStub::SetSource;
    avMetadataHelperFuncs_[RESOLVE_METADATA] = &AVMetadataHelperServiceStub::ResolveMetadata;
    avMetadataHelperFuncs_[RESOLVE_METADATA_MAP] = &AVMetadataHelperServiceStub::ResolveMetadataMap;
    avMetadataHelperFuncs_[FETCH_FRAME_AT_TIME] = &AVMetadataHelperServiceStub::FetchFrameAtTime;
    avMetadataHelperFuncs_[RELEASE] = &AVMetadataHelperServiceStub::Release;
    avMetadataHelperFuncs_[DESTROY] = &AVMetadataHelperServiceStub::DestroyStub;
    return MSERR_OK;
}

int32_t AVMetadataHelperServiceStub::DestroyStub()
{
    avMetadateHelperServer_ = nullptr;
    MediaServerManager::GetInstance().DestroyStubObject(MediaServerManager::AVMETADATAHELPER, AsObject());
    return MSERR_OK;
}

int AVMetadataHelperServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    MEDIA_LOGI("Stub: OnRemoteRequest of code: %{public}u is received", code);

    auto itFunc = avMetadataHelperFuncs_.find(code);
    if (itFunc != avMetadataHelperFuncs_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            int32_t ret = (this->*memberFunc)(data, reply);
            if (ret != MSERR_OK) {
                MEDIA_LOGE("Calling memberFunc is failed.");
            }
            return MSERR_OK;
        }
    }
    MEDIA_LOGW("AVMetadataHelperServiceStub: no member func supporting, applying default process");

    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int32_t AVMetadataHelperServiceStub::SetSource(const std::string &uri, int32_t usage)
{
    CHECK_AND_RETURN_RET_LOG(avMetadateHelperServer_ != nullptr, MSERR_NO_MEMORY,
        "avmetadatahelper server is nullptr");
    return avMetadateHelperServer_->SetSource(uri, usage);
}

std::string AVMetadataHelperServiceStub::ResolveMetadata(int32_t key)
{
    CHECK_AND_RETURN_RET_LOG(avMetadateHelperServer_ != nullptr, "",
        "avmetadatahelper server is nullptr");
    return avMetadateHelperServer_->ResolveMetadata(key);
}

std::unordered_map<int32_t, std::string> AVMetadataHelperServiceStub::ResolveMetadataMap()
{
    CHECK_AND_RETURN_RET_LOG(avMetadateHelperServer_ != nullptr, {},
        "avmetadatahelper server is nullptr");
    return avMetadateHelperServer_->ResolveMetadata();
}

std::shared_ptr<AVSharedMemory> AVMetadataHelperServiceStub::FetchFrameAtTime(int64_t timeUs,
    int32_t option, OutputConfiguration param)
{
    CHECK_AND_RETURN_RET_LOG(avMetadateHelperServer_ != nullptr, nullptr,
        "avmetadatahelper server is nullptr");
    return avMetadateHelperServer_->FetchFrameAtTime(timeUs, option, param);
}

void AVMetadataHelperServiceStub::Release()
{
    CHECK_AND_RETURN_LOG(avMetadateHelperServer_ != nullptr, "avmetadatahelper server is nullptr");
    return avMetadateHelperServer_->Release();
}

int32_t AVMetadataHelperServiceStub::SetSource(MessageParcel &data, MessageParcel &reply)
{
    std::string uri = data.ReadString();
    int32_t usage = data.ReadInt32();
    reply.WriteInt32(SetSource(uri, usage));
    return MSERR_OK;
}

int32_t AVMetadataHelperServiceStub::ResolveMetadata(MessageParcel &data, MessageParcel &reply)
{
    int32_t key = data.ReadInt32();
    reply.WriteString(ResolveMetadata(key));
    return MSERR_OK;
}

int32_t AVMetadataHelperServiceStub::ResolveMetadataMap(MessageParcel &data, MessageParcel &reply)
{
    std::unordered_map<int32_t, std::string> metadata = ResolveMetadataMap();
    CHECK_AND_RETURN_RET_LOG(metadata.size() != 0, MSERR_INVALID_VAL, "No metadata");

    std::vector<int32_t> key;
    std::vector<std::string> dataStr;
    for (auto it = metadata.begin(); it != metadata.end(); it++) {
        key.push_back(it->first);
        dataStr.push_back(it->second);
    }

    reply.WriteInt32Vector(key);
    reply.WriteStringVector(dataStr);
    return MSERR_OK;
}

int32_t AVMetadataHelperServiceStub::FetchFrameAtTime(MessageParcel &data, MessageParcel &reply)
{
    int64_t timeUs = data.ReadInt64();
    int32_t option = data.ReadInt32();
    OutputConfiguration param = {data.ReadInt32(), data.ReadInt32(), static_cast<PixelFormat>(data.ReadInt32())};
    std::shared_ptr<AVSharedMemory> ashMem = FetchFrameAtTime(timeUs, option, param);

    return WriteAVSharedMemoryToParcel(ashMem, reply);
}

int32_t AVMetadataHelperServiceStub::Release(MessageParcel &data, MessageParcel &reply)
{
    Release();
    return MSERR_OK;
}

int32_t AVMetadataHelperServiceStub::DestroyStub(MessageParcel &data, MessageParcel &reply)
{
    reply.WriteInt32(DestroyStub());
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
