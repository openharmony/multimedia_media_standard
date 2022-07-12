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

#include "avmuxer_service_stub.h"
#include "media_server_manager.h"
#include "media_errors.h"
#include "media_log.h"
#include "avsharedmemory_ipc.h"
#include "media_parcel.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMuxerServiceStub"};
}

namespace OHOS {
namespace Media {
sptr<AVMuxerServiceStub> AVMuxerServiceStub::Create()
{
    sptr<AVMuxerServiceStub> avmuxerStub = new(std::nothrow) AVMuxerServiceStub();
    CHECK_AND_RETURN_RET_LOG(avmuxerStub != nullptr, nullptr, "Failed to create avmuxer service stub");

    int32_t ret = avmuxerStub->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "Failed to init AVMuxerServiceStub");
    return avmuxerStub;
}

AVMuxerServiceStub::AVMuxerServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVMuxerServiceStub::~AVMuxerServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVMuxerServiceStub::Init()
{
    avmuxerServer_ = AVMuxerServer::Create();
    CHECK_AND_RETURN_RET_LOG(avmuxerServer_ != nullptr, MSERR_NO_MEMORY, "Failed to create muxer server");

    avmuxerFuncs_[GET_MUXER_FORMAT_LIST] = &AVMuxerServiceStub::GetAVMuxerFormatList;
    avmuxerFuncs_[SET_OUTPUT] = &AVMuxerServiceStub::SetOutput;
    avmuxerFuncs_[SET_LOCATION] = &AVMuxerServiceStub::SetLocation;
    avmuxerFuncs_[SET_ORIENTATION_HINT] = &AVMuxerServiceStub::SetRotation;
    avmuxerFuncs_[ADD_TRACK] = &AVMuxerServiceStub::AddTrack;
    avmuxerFuncs_[START] = &AVMuxerServiceStub::Start;
    avmuxerFuncs_[WRITE_TRACK_SAMPLE] = &AVMuxerServiceStub::WriteTrackSample;
    avmuxerFuncs_[STOP] = &AVMuxerServiceStub::Stop;
    avmuxerFuncs_[RELEASE] = &AVMuxerServiceStub::Release;
    avmuxerFuncs_[DESTROY] = &AVMuxerServiceStub::DestroyStub;
    return MSERR_OK;
}

int32_t AVMuxerServiceStub::DestroyStub()
{
    avmuxerServer_ = nullptr;
    MediaServerManager::GetInstance().DestroyStubObject(MediaServerManager::AVMUXER, AsObject());
    return MSERR_OK;
}

int AVMuxerServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    MEDIA_LOGI("Stub: OnRemoteRequest of code: %{public}u is received", code);

    auto remoteDescriptor = data.ReadInterfaceToken();
    if (AVMuxerServiceStub::GetDescriptor() != remoteDescriptor) {
        MEDIA_LOGE("Invalid descriptor");
        return MSERR_INVALID_OPERATION;
    }

    auto itFunc = avmuxerFuncs_.find(code);
    if (itFunc != avmuxerFuncs_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            int32_t ret = (this->*memberFunc)(data, reply);
            CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call memberFunc");
            return MSERR_OK;
        }
    }
    MEDIA_LOGW("Failed to find corresponding function");
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

std::vector<std::string> AVMuxerServiceStub::GetAVMuxerFormatList()
{
    CHECK_AND_RETURN_RET_LOG(avmuxerServer_ != nullptr, std::vector<std::string>(), "AVMuxer Service does not exist");
    return avmuxerServer_->GetAVMuxerFormatList();
}

int32_t AVMuxerServiceStub::SetOutput(int32_t fd, const std::string &format)
{
    CHECK_AND_RETURN_RET_LOG(avmuxerServer_ != nullptr, MSERR_NO_MEMORY, "AVMuxer Service does not exist");
    return avmuxerServer_->SetOutput(fd, format);
}

int32_t AVMuxerServiceStub::SetLocation(float latitude, float longitude)
{
    CHECK_AND_RETURN_RET_LOG(avmuxerServer_ != nullptr, MSERR_NO_MEMORY, "AVMuxer Service does not exist");
    return avmuxerServer_->SetLocation(latitude, longitude);
}

int32_t AVMuxerServiceStub::SetRotation(int32_t rotation)
{
    CHECK_AND_RETURN_RET_LOG(avmuxerServer_ != nullptr, MSERR_NO_MEMORY, "AVMuxer Service does not exist");
    return avmuxerServer_->SetRotation(rotation);
}

int32_t AVMuxerServiceStub::AddTrack(const MediaDescription &trackDesc, int32_t &trackId)
{
    CHECK_AND_RETURN_RET_LOG(avmuxerServer_ != nullptr, MSERR_NO_MEMORY, "AVMuxer Service does not exist");
    return avmuxerServer_->AddTrack(trackDesc, trackId);
}

int32_t AVMuxerServiceStub::Start()
{
    CHECK_AND_RETURN_RET_LOG(avmuxerServer_ != nullptr, MSERR_NO_MEMORY, "AVMuxer Service does not exist");
    return avmuxerServer_->Start();
}

int32_t AVMuxerServiceStub::WriteTrackSample(std::shared_ptr<AVSharedMemory> sampleData,
    const TrackSampleInfo &sampleInfo)
{
    CHECK_AND_RETURN_RET_LOG(sampleData != nullptr, MSERR_INVALID_VAL, "sampleData is nullptr");
    CHECK_AND_RETURN_RET_LOG(avmuxerServer_ != nullptr, MSERR_NO_MEMORY, "AVMuxer Service does not exist");
    return avmuxerServer_->WriteTrackSample(sampleData, sampleInfo);
}

int32_t AVMuxerServiceStub::Stop()
{
    CHECK_AND_RETURN_RET_LOG(avmuxerServer_ != nullptr, MSERR_NO_MEMORY, "AVMuxer Service does not exist");
    return avmuxerServer_->Stop();
}

void AVMuxerServiceStub::Release()
{
    CHECK_AND_RETURN_LOG(avmuxerServer_ != nullptr, "AVMuxer Service does not exist");
    avmuxerServer_->Release();
}

int32_t AVMuxerServiceStub::GetAVMuxerFormatList(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    CHECK_AND_RETURN_RET(reply.WriteStringVector(GetAVMuxerFormatList()), MSERR_UNKNOWN);
    return MSERR_OK;
}

int32_t AVMuxerServiceStub::SetOutput(MessageParcel &data, MessageParcel &reply)
{
    int32_t fd = data.ReadFileDescriptor();
    std::string format = data.ReadString();
    CHECK_AND_RETURN_RET(reply.WriteInt32(SetOutput(fd, format)), MSERR_UNKNOWN);
    return MSERR_OK;
}

int32_t AVMuxerServiceStub::SetLocation(MessageParcel &data, MessageParcel &reply)
{
    float latitude = data.ReadFloat();
    float longitude = data.ReadFloat();
    CHECK_AND_RETURN_RET(reply.WriteInt32(SetLocation(latitude, longitude)), MSERR_UNKNOWN);
    return MSERR_OK;
}

int32_t AVMuxerServiceStub::SetRotation(MessageParcel &data, MessageParcel &reply)
{
    int32_t rotation = data.ReadInt32();
    CHECK_AND_RETURN_RET(reply.WriteInt32(SetRotation(rotation)), MSERR_UNKNOWN);
    return MSERR_OK;
}

int32_t AVMuxerServiceStub::AddTrack(MessageParcel &data, MessageParcel &reply)
{
    MediaDescription trackDesc;
    (void)MediaParcel::Unmarshalling(data, trackDesc);
    int32_t trackId;
    int32_t ret = AddTrack(trackDesc, trackId);
    CHECK_AND_RETURN_RET(reply.WriteInt32(trackId), MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(reply.WriteInt32(ret), MSERR_UNKNOWN);
    return MSERR_OK;
}

int32_t AVMuxerServiceStub::Start(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    CHECK_AND_RETURN_RET(reply.WriteInt32(Start()), MSERR_UNKNOWN);
    return MSERR_OK;
}

int32_t AVMuxerServiceStub::WriteTrackSample(MessageParcel &data, MessageParcel &reply)
{
    std::shared_ptr<AVSharedMemory> sampleData = ReadAVSharedMemoryFromParcel(data);
    CHECK_AND_RETURN_RET(sampleData != nullptr, MSERR_UNKNOWN);
    TrackSampleInfo sampleInfo = {data.ReadInt32(), data.ReadInt64(),
        data.ReadInt32(), static_cast<AVCodecBufferFlag>(data.ReadInt32())};
    CHECK_AND_RETURN_RET(reply.WriteInt32(WriteTrackSample(sampleData, sampleInfo)), MSERR_UNKNOWN);
    return MSERR_OK;
}

int32_t AVMuxerServiceStub::Stop(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    CHECK_AND_RETURN_RET(reply.WriteInt32(Stop()), MSERR_UNKNOWN);
    return MSERR_OK;
}

int32_t AVMuxerServiceStub::Release(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    (void)reply;
    Release();
    return MSERR_OK;
}

int32_t AVMuxerServiceStub::DestroyStub(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    CHECK_AND_RETURN_RET(reply.WriteInt32(DestroyStub()), MSERR_UNKNOWN);
    return MSERR_OK;
}
}  // namespace Media
}  // namespace OHOS