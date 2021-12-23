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

#include "avcodec_service_stub.h"
#include <unistd.h>
#include "avcodec_listener_proxy.h"
#include "avsharedmemory_ipc.h"
#include "media_errors.h"
#include "media_log.h"
#include "media_parcel.h"
#include "media_server_manager.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecServiceStub"};
}

namespace OHOS {
namespace Media {
sptr<AVCodecServiceStub> AVCodecServiceStub::Create()
{
    sptr<AVCodecServiceStub> codecStub = new(std::nothrow) AVCodecServiceStub();
    CHECK_AND_RETURN_RET_LOG(codecStub != nullptr, nullptr, "failed to new AVCodecServiceStub");

    int32_t ret = codecStub->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to codec stub init");
    return codecStub;
}

AVCodecServiceStub::AVCodecServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecServiceStub::~AVCodecServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVCodecServiceStub::Init()
{
    codecServer_ = AVCodecServer::Create();
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, MSERR_NO_MEMORY, "failed to create AVCodecServer");

    recFuncs_[SET_LISTENER_OBJ] = &AVCodecServiceStub::SetListenerObject;
    recFuncs_[INIT_PARAMETER] = &AVCodecServiceStub::InitParameter;
    recFuncs_[CONFIGURE] = &AVCodecServiceStub::Configure;
    recFuncs_[PREPARE] = &AVCodecServiceStub::Prepare;
    recFuncs_[START] = &AVCodecServiceStub::Start;
    recFuncs_[STOP] = &AVCodecServiceStub::Stop;
    recFuncs_[FLUSH] = &AVCodecServiceStub::Flush;
    recFuncs_[RESET] = &AVCodecServiceStub::Reset;
    recFuncs_[RELEASE] = &AVCodecServiceStub::Release;
    recFuncs_[CREATE_INPUT_SURFACE] = &AVCodecServiceStub::CreateInputSurface;
    recFuncs_[SET_OUTPUT_SURFACE] = &AVCodecServiceStub::SetOutputSurface;
    recFuncs_[GET_INPUT_BUFFER] = &AVCodecServiceStub::GetInputBuffer;
    recFuncs_[QUEUE_INPUT_BUFFER] = &AVCodecServiceStub::QueueInputBuffer;
    recFuncs_[GET_OUTPUT_BUFFER] = &AVCodecServiceStub::GetOutputBuffer;
    recFuncs_[RELEASE_OUTPUT_BUFFER] = &AVCodecServiceStub::ReleaseOutputBuffer;
    recFuncs_[GET_OUTPUT_FORMAT] = &AVCodecServiceStub::GetOutputFormat;
    recFuncs_[SET_PARAMETER] = &AVCodecServiceStub::SetParameter;
    recFuncs_[DESTROY] = &AVCodecServiceStub::DestroyStub;
    return MSERR_OK;
}

int32_t AVCodecServiceStub::DestroyStub()
{
    codecServer_ = nullptr;

    MediaServerManager::GetInstance().DestroyStubObject(MediaServerManager::RECORDER, AsObject());
    return MSERR_OK;
}

int AVCodecServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    MEDIA_LOGI("Stub: OnRemoteRequest of code: %{public}d is received", code);

    auto itFunc = recFuncs_.find(code);
    if (itFunc != recFuncs_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            int32_t ret = (this->*memberFunc)(data, reply);
            if (ret != MSERR_OK) {
                MEDIA_LOGE("calling memberFunc is failed.");
            }
            return MSERR_OK;
        }
    }
    MEDIA_LOGW("AVCodecServiceStub: no member func supporting, applying default process");

    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int32_t AVCodecServiceStub::SetListenerObject(const sptr<IRemoteObject> &object)
{
    CHECK_AND_RETURN_RET_LOG(object != nullptr, MSERR_NO_MEMORY, "set listener object is nullptr");

    sptr<IStandardAVCodecListener> listener = iface_cast<IStandardAVCodecListener>(object);
    CHECK_AND_RETURN_RET_LOG(listener != nullptr, MSERR_NO_MEMORY, "failed to convert IStandardAVCodecListener");

    std::shared_ptr<AVCodecCallback> callback = std::make_shared<AVCodecListenerCallback>(listener);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_NO_MEMORY, "failed to new AVCodecListenerCallback");

    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, MSERR_NO_MEMORY, "avcodec server is nullptr");
    (void)codecServer_->SetCallback(callback);
    return MSERR_OK;
}

int32_t AVCodecServiceStub::InitParameter(AVCodecType type, bool isMimeType, const std::string &name)
{
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, MSERR_NO_MEMORY, "avcodec server is nullptr");
    return codecServer_->InitParameter(type, isMimeType, name);
}

int32_t AVCodecServiceStub::Configure(const Format &format)
{
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, MSERR_NO_MEMORY, "avcodec server is nullptr");
    return codecServer_->Configure(format);
}

int32_t AVCodecServiceStub::Prepare()
{
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, MSERR_NO_MEMORY, "avcodec server is nullptr");
    return codecServer_->Prepare();
}

int32_t AVCodecServiceStub::Start()
{
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, MSERR_NO_MEMORY, "avcodec server is nullptr");
    return codecServer_->Start();
}

int32_t AVCodecServiceStub::Stop()
{
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, MSERR_NO_MEMORY, "avcodec server is nullptr");
    return codecServer_->Stop();
}

int32_t AVCodecServiceStub::Flush()
{
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, MSERR_NO_MEMORY, "avcodec server is nullptr");
    return codecServer_->Flush();
}

int32_t AVCodecServiceStub::Reset()
{
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, MSERR_NO_MEMORY, "avcodec server is nullptr");
    return codecServer_->Reset();
}

int32_t AVCodecServiceStub::Release()
{
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, MSERR_NO_MEMORY, "avcodec server is nullptr");
    return codecServer_->Release();
}

sptr<OHOS::Surface> AVCodecServiceStub::CreateInputSurface()
{
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, nullptr, "avcodec server is nullptr");
    return codecServer_->CreateInputSurface();
}

int32_t AVCodecServiceStub::SetOutputSurface(sptr<OHOS::Surface> surface)
{
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, MSERR_NO_MEMORY, "avcodec server is nullptr");
    return codecServer_->SetOutputSurface(surface);
}

std::shared_ptr<AVSharedMemory> AVCodecServiceStub::GetInputBuffer(uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, nullptr, "avcodec server is nullptr");
    return codecServer_->GetInputBuffer(index);
}

int32_t AVCodecServiceStub::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, MSERR_NO_MEMORY, "avcodec server is nullptr");
    return codecServer_->QueueInputBuffer(index, info, flag);
}

std::shared_ptr<AVSharedMemory> AVCodecServiceStub::GetOutputBuffer(uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, nullptr, "avcodec server is nullptr");
    return codecServer_->GetOutputBuffer(index);
}

int32_t AVCodecServiceStub::GetOutputFormat(Format &format)
{
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, MSERR_NO_MEMORY, "avcodec server is nullptr");
    return codecServer_->GetOutputFormat(format);
}

int32_t AVCodecServiceStub::ReleaseOutputBuffer(uint32_t index, bool render)
{
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, MSERR_NO_MEMORY, "avcodec server is nullptr");
    return codecServer_->ReleaseOutputBuffer(index, render);
}

int32_t AVCodecServiceStub::SetParameter(const Format &format)
{
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, MSERR_NO_MEMORY, "avcodec server is nullptr");
    return codecServer_->SetParameter(format);
}

int32_t AVCodecServiceStub::SetListenerObject(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    reply.WriteInt32(SetListenerObject(object));
    return MSERR_OK;
}

int32_t AVCodecServiceStub::InitParameter(MessageParcel &data, MessageParcel &reply)
{
    AVCodecType type = static_cast<AVCodecType>(data.ReadInt32());
    bool isMimeType = data.ReadBool();
    std::string name = data.ReadString();
    reply.WriteInt32(InitParameter(type, isMimeType, name));
    return MSERR_OK;
}

int32_t AVCodecServiceStub::Configure(MessageParcel &data, MessageParcel &reply)
{
    Format format;
    (void)MediaParcel::Unmarshalling(data, format);
    reply.WriteInt32(Configure(format));
    return MSERR_OK;
}

int32_t AVCodecServiceStub::Prepare(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Prepare());
    return MSERR_OK;
}

int32_t AVCodecServiceStub::Start(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Start());
    return MSERR_OK;
}

int32_t AVCodecServiceStub::Stop(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Stop());
    return MSERR_OK;
}

int32_t AVCodecServiceStub::Flush(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Flush());
    return MSERR_OK;
}

int32_t AVCodecServiceStub::Reset(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Reset());
    return MSERR_OK;
}

int32_t AVCodecServiceStub::Release(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Release());
    return MSERR_OK;
}

int32_t AVCodecServiceStub::CreateInputSurface(MessageParcel &data, MessageParcel &reply)
{
    sptr<OHOS::Surface> surface = CreateInputSurface();
    if (surface != nullptr && surface->GetProducer() != nullptr) {
        sptr<IRemoteObject> object = surface->GetProducer()->AsObject();
        (void)reply.WriteRemoteObject(object);
    }
    return MSERR_OK;
}

int32_t AVCodecServiceStub::SetOutputSurface(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, MSERR_NO_MEMORY, "object is nullptr");

    sptr<IBufferProducer> producer = iface_cast<IBufferProducer>(object);
    CHECK_AND_RETURN_RET_LOG(producer != nullptr, MSERR_NO_MEMORY, "failed to convert object to producer");

    sptr<OHOS::Surface> surface = OHOS::Surface::CreateSurfaceAsProducer(producer);
    CHECK_AND_RETURN_RET_LOG(surface != nullptr, MSERR_NO_MEMORY, "failed to create surface");

    std::string format = data.ReadString();
    MEDIA_LOGI("surfaceFormat is %{public}s!", format.c_str());
    const std::string surfaceFormat = "SURFACE_FORMAT";
    (void)surface->SetUserData(surfaceFormat, format);
    reply.WriteInt32(SetOutputSurface(surface));
    return MSERR_OK;
}

int32_t AVCodecServiceStub::GetInputBuffer(MessageParcel &data, MessageParcel &reply)
{
    uint32_t index = data.ReadUint32();
    auto buffer = GetInputBuffer(index);
    if (buffer != nullptr) {
        (void)WriteAVSharedMemoryToParcel(buffer, reply);
    }
    return MSERR_OK;
}

int32_t AVCodecServiceStub::QueueInputBuffer(MessageParcel &data, MessageParcel &reply)
{
    uint32_t index = data.ReadUint32();
    AVCodecBufferInfo info;
    info.presentationTimeUs = data.ReadInt64();
    info.size = data.ReadInt32();
    info.offset = data.ReadInt32();
    AVCodecBufferFlag flag = static_cast<AVCodecBufferFlag>(data.ReadInt32());
    reply.WriteInt32(QueueInputBuffer(index, info, flag));
    return MSERR_OK;
}

int32_t AVCodecServiceStub::GetOutputBuffer(MessageParcel &data, MessageParcel &reply)
{
    uint32_t index = data.ReadUint32();
    auto buffer = GetOutputBuffer(index);
    if (buffer != nullptr) {
        (void)WriteAVSharedMemoryToParcel(buffer, reply);
    }
    return MSERR_OK;
}

int32_t AVCodecServiceStub::GetOutputFormat(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    Format format;
    (void)GetOutputFormat(format);
    (void)MediaParcel::Marshalling(reply, format);
    return MSERR_OK;
}

int32_t AVCodecServiceStub::ReleaseOutputBuffer(MessageParcel &data, MessageParcel &reply)
{
    uint32_t index = data.ReadUint32();
    bool render = data.ReadBool();
    reply.WriteInt32(ReleaseOutputBuffer(index, render));
    return MSERR_OK;
}

int32_t AVCodecServiceStub::SetParameter(MessageParcel &data, MessageParcel &reply)
{
    Format format;
    (void)MediaParcel::Unmarshalling(data, format);
    reply.WriteInt32(SetParameter(format));
    return MSERR_OK;
}

int32_t AVCodecServiceStub::DestroyStub(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(DestroyStub());
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
