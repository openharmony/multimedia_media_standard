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

#include "recorder_service_stub.h"
#include <unistd.h>
#include "recorder_listener_proxy.h"
#include "media_server_manager.h"
#include "media_log.h"
#include "errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderServiceStub"};
}

namespace OHOS {
namespace Media {
sptr<RecorderServiceStub> RecorderServiceStub::Create()
{
    sptr<RecorderServiceStub> recorderStub = new(std::nothrow) RecorderServiceStub();
    CHECK_AND_RETURN_RET_LOG(recorderStub != nullptr, nullptr, "failed to new RecorderServiceStub");

    int32_t ret = recorderStub->Init();
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, nullptr, "failed to recorder stub init");
    return recorderStub;
}

RecorderServiceStub::RecorderServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

RecorderServiceStub::~RecorderServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t RecorderServiceStub::Init()
{
    recorderServer_ = RecorderServer::Create();
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_NO_INIT, "failed to create RecorderServer");

    recFuncs_[SET_LISTENER_OBJ] = &RecorderServiceStub::SetListenerObject;
    recFuncs_[SET_VIDEO_SOURCE] = &RecorderServiceStub::SetVideoSource;
    recFuncs_[SET_VIDEO_ENCODER] = &RecorderServiceStub::SetVideoEncoder;
    recFuncs_[SET_VIDEO_SIZE] = &RecorderServiceStub::SetVideoSize;
    recFuncs_[SET_VIDEO_FARAME_RATE] = &RecorderServiceStub::SetVideoFrameRate;
    recFuncs_[SET_VIDEO_ENCODING_BIT_RATE] = &RecorderServiceStub::SetVideoEncodingBitRate;
    recFuncs_[SET_CAPTURE_RATE] = &RecorderServiceStub::SetCaptureRate;
    recFuncs_[GET_SURFACE] = &RecorderServiceStub::GetSurface;
    recFuncs_[SET_AUDIO_SOURCE] = &RecorderServiceStub::SetAudioSource;
    recFuncs_[SET_AUDIO_ENCODER] = &RecorderServiceStub::SetAudioEncoder;
    recFuncs_[SET_AUDIO_SAMPLE_RATE] = &RecorderServiceStub::SetAudioSampleRate;
    recFuncs_[SET_AUDIO_CHANNELS] = &RecorderServiceStub::SetAudioChannels;
    recFuncs_[SET_AUDIO_ENCODING_BIT_RATE] = &RecorderServiceStub::SetAudioEncodingBitRate;
    recFuncs_[SET_DATA_SOURCE] = &RecorderServiceStub::SetDataSource;
    recFuncs_[SET_MAX_DURATION] = &RecorderServiceStub::SetMaxDuration;
    recFuncs_[SET_OUTPUT_FORMAT] = &RecorderServiceStub::SetOutputFormat;
    recFuncs_[SET_OUTPUT_PATH] = &RecorderServiceStub::SetOutputPath;
    recFuncs_[SET_OUTPUT_FILE] = &RecorderServiceStub::SetOutputFile;
    recFuncs_[SET_NEXT_OUTPUT_FILE] = &RecorderServiceStub::SetNextOutputFile;
    recFuncs_[SET_MAX_FILE_SIZE] = &RecorderServiceStub::SetMaxFileSize;
    recFuncs_[PREPARE] = &RecorderServiceStub::Prepare;
    recFuncs_[START] = &RecorderServiceStub::Start;
    recFuncs_[PAUSE] = &RecorderServiceStub::Pause;
    recFuncs_[RESUME] = &RecorderServiceStub::Resume;
    recFuncs_[STOP] = &RecorderServiceStub::Stop;
    recFuncs_[RESET] = &RecorderServiceStub::Reset;
    recFuncs_[RELEASE] = &RecorderServiceStub::Release;
    recFuncs_[SET_FILE_SPLIT_DURATION] = &RecorderServiceStub::SetFileSplitDuration;
    recFuncs_[DESTROY] = &RecorderServiceStub::DestroyStub;
    return ERR_OK;
}

int32_t RecorderServiceStub::DestroyStub()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (deathRecipient_ != nullptr) {
        deathRecipient_->SetNotifyCb(nullptr);
    }
    deathRecipient_ = nullptr;
    recorderServer_ = nullptr;

    MediaServerManager::GetInstance().DestroyStubObject(MediaServerManager::RECORDER, AsObject());
    return ERR_OK;
}

void RecorderServiceStub::ClientDied()
{
    MEDIA_LOGE("recorder client listenerStub is nullptr");
    (void)DestroyStub();
}

int RecorderServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    MEDIA_LOGI("Stub: OnRemoteRequest of code: %{public}d is received", code);

    auto itFunc = recFuncs_.find(code);
    if (itFunc != recFuncs_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            int32_t ret = (this->*memberFunc)(data, reply);
            if (ret != ERR_OK) {
                MEDIA_LOGE("calling memberFunc is failed.");
            }
            return ERR_OK;
        }
    }
    MEDIA_LOGW("RecorderServiceStub: no member func supporting, applying default process");

    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int32_t RecorderServiceStub::SetListenerObject(const sptr<IRemoteObject> &object)
{
    CHECK_AND_RETURN_RET_LOG(object != nullptr, ERR_INVALID_VALUE, "set listener object is nullptr");

    sptr<IStandardRecorderListener> listener = iface_cast<IStandardRecorderListener>(object);
    CHECK_AND_RETURN_RET_LOG(listener != nullptr, ERR_INVALID_VALUE, "failed to convert IStandardRecorderListener");

    deathRecipient_ = new(std::nothrow) MediaDeathRecipient();
    CHECK_AND_RETURN_RET_LOG(deathRecipient_ != nullptr, ERR_NO_MEMORY, "failed to new MediaDeathRecipient");

    deathRecipient_->SetNotifyCb(std::bind(&RecorderServiceStub::ClientDied, this));

    std::shared_ptr<RecorderCallback> callback = std::make_shared<RecorderListenerCallback>(listener, deathRecipient_);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, ERR_NO_MEMORY, "failed to new RecorderListenerCallback");

    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    (void)recorderServer_->SetRecorderCallback(callback);
    return ERR_OK;
}

int32_t RecorderServiceStub::SetVideoSource(VideoSourceType source, int32_t &sourceId)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    return recorderServer_->SetVideoSource(source, sourceId);
}

int32_t RecorderServiceStub::SetVideoEncoder(int32_t sourceId, VideoCodecFormat encoder)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    return recorderServer_->SetVideoEncoder(sourceId, encoder);
}

int32_t RecorderServiceStub::SetVideoSize(int32_t sourceId, int32_t width, int32_t height)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    return recorderServer_->SetVideoSize(sourceId, width, height);
}

int32_t RecorderServiceStub::SetVideoFrameRate(int32_t sourceId, int32_t frameRate)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    return recorderServer_->SetVideoFrameRate(sourceId, frameRate);
}

int32_t RecorderServiceStub::SetVideoEncodingBitRate(int32_t sourceId, int32_t rate)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    return recorderServer_->SetVideoEncodingBitRate(sourceId, rate);
}

int32_t RecorderServiceStub::SetCaptureRate(int32_t sourceId, double fps)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    return recorderServer_->SetCaptureRate(sourceId, fps);
}

sptr<OHOS::Surface> RecorderServiceStub::GetSurface(int32_t sourceId)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, nullptr, "recorder server is nullptr");
    return recorderServer_->GetSurface(sourceId);
}

int32_t RecorderServiceStub::SetAudioSource(AudioSourceType source, int32_t &sourceId)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    return recorderServer_->SetAudioSource(source, sourceId);
}

int32_t RecorderServiceStub::SetAudioEncoder(int32_t sourceId, AudioCodecFormat encoder)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    return recorderServer_->SetAudioEncoder(sourceId, encoder);
}

int32_t RecorderServiceStub::SetAudioSampleRate(int32_t sourceId, int32_t rate)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    return recorderServer_->SetAudioSampleRate(sourceId, rate);
}

int32_t RecorderServiceStub::SetAudioChannels(int32_t sourceId, int32_t num)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    return recorderServer_->SetAudioChannels(sourceId, num);
}

int32_t RecorderServiceStub::SetAudioEncodingBitRate(int32_t sourceId, int32_t bitRate)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    return recorderServer_->SetAudioEncodingBitRate(sourceId, bitRate);
}

int32_t RecorderServiceStub::SetDataSource(DataSourceType dataType, int32_t &sourceId)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    return recorderServer_->SetDataSource(dataType, sourceId);
}

int32_t RecorderServiceStub::SetMaxDuration(int32_t duration)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    return recorderServer_->SetMaxDuration(duration);
}

int32_t RecorderServiceStub::SetOutputFormat(OutputFormatType format)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    return recorderServer_->SetOutputFormat(format);
}

int32_t RecorderServiceStub::SetOutputPath(const std::string &path)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    return recorderServer_->SetOutputPath(path);
}

int32_t RecorderServiceStub::SetOutputFile(int32_t fd)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    return recorderServer_->SetOutputFile(fd);
}

int32_t RecorderServiceStub::SetNextOutputFile(int32_t fd)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    return recorderServer_->SetNextOutputFile(fd);
}

int32_t RecorderServiceStub::SetMaxFileSize(int64_t size)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    return recorderServer_->SetMaxFileSize(size);
}

int32_t RecorderServiceStub::Prepare()
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    return recorderServer_->Prepare();
}

int32_t RecorderServiceStub::Start()
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    return recorderServer_->Start();
}

int32_t RecorderServiceStub::Pause()
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    return recorderServer_->Pause();
}

int32_t RecorderServiceStub::Resume()
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    return recorderServer_->Resume();
}

int32_t RecorderServiceStub::Stop(bool block)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    return recorderServer_->Stop(block);
}

int32_t RecorderServiceStub::Reset()
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    return recorderServer_->Reset();
}

int32_t RecorderServiceStub::Release()
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    return recorderServer_->Release();
}

int32_t RecorderServiceStub::SetFileSplitDuration(FileSplitType type, int64_t timestamp, uint32_t duration)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, ERR_INVALID_OPERATION, "recorder server is nullptr");
    return recorderServer_->SetFileSplitDuration(type, timestamp, duration);
}

int32_t RecorderServiceStub::SetListenerObject(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    int32_t ret = SetListenerObject(object);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t RecorderServiceStub::SetVideoSource(MessageParcel &data, MessageParcel &reply)
{
    int32_t source = data.ReadInt32();
    VideoSourceType sourceType = static_cast<VideoSourceType>(source);
    int32_t sourceId = 0;
    int32_t ret = SetVideoSource(sourceType, sourceId);
    reply.WriteInt32(sourceId);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t RecorderServiceStub::SetVideoEncoder(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t encoder = data.ReadInt32();
    VideoCodecFormat codecFormat = static_cast<VideoCodecFormat>(encoder);
    int32_t ret = SetVideoEncoder(sourceId, codecFormat);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t RecorderServiceStub::SetVideoSize(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t width = data.ReadInt32();
    int32_t height = data.ReadInt32();
    int32_t ret = SetVideoSize(sourceId, width, height);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t RecorderServiceStub::SetVideoFrameRate(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t frameRate = data.ReadInt32();
    int32_t ret = SetVideoFrameRate(sourceId, frameRate);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t RecorderServiceStub::SetVideoEncodingBitRate(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t rate = data.ReadInt32();
    int32_t ret = SetVideoEncodingBitRate(sourceId, rate);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t RecorderServiceStub::SetCaptureRate(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    double fps = data.ReadDouble();
    int32_t ret = SetCaptureRate(sourceId, fps);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t RecorderServiceStub::GetSurface(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    sptr<OHOS::Surface> surface = GetSurface(sourceId);
    if (surface != nullptr && surface->GetProducer() != nullptr) {
        sptr<IRemoteObject> object = surface->GetProducer()->AsObject();
        (void)reply.WriteRemoteObject(object);
    }
    return ERR_OK;
}

int32_t RecorderServiceStub::SetAudioSource(MessageParcel &data, MessageParcel &reply)
{
    int32_t type = data.ReadInt32();
    int32_t sourceId = 0;
    AudioSourceType sourceType = static_cast<AudioSourceType>(type);
    int32_t ret = SetAudioSource(sourceType, sourceId);
    reply.WriteInt32(sourceId);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t RecorderServiceStub::SetAudioEncoder(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t format = data.ReadInt32();
    AudioCodecFormat encoderFormat = static_cast<AudioCodecFormat>(format);
    int32_t ret = SetAudioEncoder(sourceId, encoderFormat);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t RecorderServiceStub::SetAudioSampleRate(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t rate = data.ReadInt32();
    int32_t ret = SetAudioSampleRate(sourceId, rate);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t RecorderServiceStub::SetAudioChannels(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t num = data.ReadInt32();
    int32_t ret = SetAudioChannels(sourceId, num);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t RecorderServiceStub::SetAudioEncodingBitRate(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t bitRate = data.ReadInt32();
    int32_t ret = SetAudioEncodingBitRate(sourceId, bitRate);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t RecorderServiceStub::SetDataSource(MessageParcel &data, MessageParcel &reply)
{
    int32_t type = data.ReadInt32();
    int32_t sourceId = 0;
    DataSourceType dataType = static_cast<DataSourceType>(type);
    int32_t ret = SetDataSource(dataType, sourceId);
    reply.WriteInt32(sourceId);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t RecorderServiceStub::SetMaxDuration(MessageParcel &data, MessageParcel &reply)
{
    int32_t duration = data.ReadInt32();
    int32_t ret = SetMaxDuration(duration);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t RecorderServiceStub::SetOutputFormat(MessageParcel &data, MessageParcel &reply)
{
    int32_t type = data.ReadInt32();
    OutputFormatType formatType = static_cast<OutputFormatType>(type);
    int32_t ret = SetOutputFormat(formatType);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t RecorderServiceStub::SetOutputPath(MessageParcel &data, MessageParcel &reply)
{
    std::string path = data.ReadString();
    int32_t ret = SetOutputPath(path);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t RecorderServiceStub::SetOutputFile(MessageParcel &data, MessageParcel &reply)
{
    int32_t fd = data.ReadFileDescriptor();
    int32_t ret = SetOutputFile(fd);
    reply.WriteInt32(ret);
    ::close(fd);
    return ERR_OK;
}

int32_t RecorderServiceStub::SetNextOutputFile(MessageParcel &data, MessageParcel &reply)
{
    int32_t fd = data.ReadFileDescriptor();
    int32_t ret = SetNextOutputFile(fd);
    reply.WriteInt32(ret);
    ::close(fd);
    return ERR_OK;
}

int32_t RecorderServiceStub::SetMaxFileSize(MessageParcel &data, MessageParcel &reply)
{
    int64_t size = data.ReadInt64();
    int32_t ret = SetMaxFileSize(size);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t RecorderServiceStub::Prepare(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    int32_t ret = Prepare();
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t RecorderServiceStub::Start(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    int32_t ret = Start();
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t RecorderServiceStub::Pause(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    int32_t ret = Pause();
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t RecorderServiceStub::Resume(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    int32_t ret = Resume();
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t RecorderServiceStub::Stop(MessageParcel &data, MessageParcel &reply)
{
    bool block = data.ReadBool();
    int32_t ret = Stop(block);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t RecorderServiceStub::Reset(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    int32_t ret = Reset();
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t RecorderServiceStub::Release(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    int32_t ret = Release();
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t RecorderServiceStub::SetFileSplitDuration(MessageParcel &data, MessageParcel &reply)
{
    int32_t type = data.ReadInt32();
    FileSplitType splitType = static_cast<FileSplitType>(type);
    int64_t timestamp = data.ReadInt64();
    uint32_t duration = data.ReadUint32();
    int32_t ret = SetFileSplitDuration(splitType, timestamp, duration);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t RecorderServiceStub::DestroyStub(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    int32_t ret = DestroyStub();
    reply.WriteInt32(ret);
    return ERR_OK;
}
} // namespace Media
} // namespace OHOS
