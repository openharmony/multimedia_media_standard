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
#include "media_errors.h"

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
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to recorder stub init");
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
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "failed to create RecorderServer");

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
    return MSERR_OK;
}

int32_t RecorderServiceStub::DestroyStub()
{
    recorderServer_ = nullptr;

    MediaServerManager::GetInstance().DestroyStubObject(MediaServerManager::RECORDER, AsObject());
    return MSERR_OK;
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
            if (ret != MSERR_OK) {
                MEDIA_LOGE("calling memberFunc is failed.");
            }
            return MSERR_OK;
        }
    }
    MEDIA_LOGW("RecorderServiceStub: no member func supporting, applying default process");

    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int32_t RecorderServiceStub::SetListenerObject(const sptr<IRemoteObject> &object)
{
    CHECK_AND_RETURN_RET_LOG(object != nullptr, MSERR_NO_MEMORY, "set listener object is nullptr");

    sptr<IStandardRecorderListener> listener = iface_cast<IStandardRecorderListener>(object);
    CHECK_AND_RETURN_RET_LOG(listener != nullptr, MSERR_NO_MEMORY, "failed to convert IStandardRecorderListener");

    std::shared_ptr<RecorderCallback> callback = std::make_shared<RecorderListenerCallback>(listener);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_NO_MEMORY, "failed to new RecorderListenerCallback");

    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    (void)recorderServer_->SetRecorderCallback(callback);
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetVideoSource(VideoSourceType source, int32_t &sourceId)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetVideoSource(source, sourceId);
}

int32_t RecorderServiceStub::SetVideoEncoder(int32_t sourceId, VideoCodecFormat encoder)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetVideoEncoder(sourceId, encoder);
}

int32_t RecorderServiceStub::SetVideoSize(int32_t sourceId, int32_t width, int32_t height)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetVideoSize(sourceId, width, height);
}

int32_t RecorderServiceStub::SetVideoFrameRate(int32_t sourceId, int32_t frameRate)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetVideoFrameRate(sourceId, frameRate);
}

int32_t RecorderServiceStub::SetVideoEncodingBitRate(int32_t sourceId, int32_t rate)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetVideoEncodingBitRate(sourceId, rate);
}

int32_t RecorderServiceStub::SetCaptureRate(int32_t sourceId, double fps)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetCaptureRate(sourceId, fps);
}

sptr<OHOS::Surface> RecorderServiceStub::GetSurface(int32_t sourceId)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, nullptr, "recorder server is nullptr");
    return recorderServer_->GetSurface(sourceId);
}

int32_t RecorderServiceStub::SetAudioSource(AudioSourceType source, int32_t &sourceId)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetAudioSource(source, sourceId);
}

int32_t RecorderServiceStub::SetAudioEncoder(int32_t sourceId, AudioCodecFormat encoder)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetAudioEncoder(sourceId, encoder);
}

int32_t RecorderServiceStub::SetAudioSampleRate(int32_t sourceId, int32_t rate)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetAudioSampleRate(sourceId, rate);
}

int32_t RecorderServiceStub::SetAudioChannels(int32_t sourceId, int32_t num)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetAudioChannels(sourceId, num);
}

int32_t RecorderServiceStub::SetAudioEncodingBitRate(int32_t sourceId, int32_t bitRate)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetAudioEncodingBitRate(sourceId, bitRate);
}

int32_t RecorderServiceStub::SetDataSource(DataSourceType dataType, int32_t &sourceId)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetDataSource(dataType, sourceId);
}

int32_t RecorderServiceStub::SetMaxDuration(int32_t duration)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetMaxDuration(duration);
}

int32_t RecorderServiceStub::SetOutputFormat(OutputFormatType format)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetOutputFormat(format);
}

int32_t RecorderServiceStub::SetOutputPath(const std::string &path)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetOutputPath(path);
}

int32_t RecorderServiceStub::SetOutputFile(int32_t fd)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetOutputFile(fd);
}

int32_t RecorderServiceStub::SetNextOutputFile(int32_t fd)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetNextOutputFile(fd);
}

int32_t RecorderServiceStub::SetMaxFileSize(int64_t size)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetMaxFileSize(size);
}

int32_t RecorderServiceStub::Prepare()
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->Prepare();
}

int32_t RecorderServiceStub::Start()
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->Start();
}

int32_t RecorderServiceStub::Pause()
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->Pause();
}

int32_t RecorderServiceStub::Resume()
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->Resume();
}

int32_t RecorderServiceStub::Stop(bool block)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->Stop(block);
}

int32_t RecorderServiceStub::Reset()
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->Reset();
}

int32_t RecorderServiceStub::Release()
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->Release();
}

int32_t RecorderServiceStub::SetFileSplitDuration(FileSplitType type, int64_t timestamp, uint32_t duration)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetFileSplitDuration(type, timestamp, duration);
}

int32_t RecorderServiceStub::SetListenerObject(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    reply.WriteInt32(SetListenerObject(object));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetVideoSource(MessageParcel &data, MessageParcel &reply)
{
    int32_t source = data.ReadInt32();
    VideoSourceType sourceType = static_cast<VideoSourceType>(source);
    int32_t sourceId = 0;
    int32_t ret = SetVideoSource(sourceType, sourceId);
    reply.WriteInt32(sourceId);
    reply.WriteInt32(ret);
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetVideoEncoder(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t encoder = data.ReadInt32();
    VideoCodecFormat codecFormat = static_cast<VideoCodecFormat>(encoder);
    reply.WriteInt32(SetVideoEncoder(sourceId, codecFormat));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetVideoSize(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t width = data.ReadInt32();
    int32_t height = data.ReadInt32();
    reply.WriteInt32(SetVideoSize(sourceId, width, height));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetVideoFrameRate(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t frameRate = data.ReadInt32();
    reply.WriteInt32(SetVideoFrameRate(sourceId, frameRate));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetVideoEncodingBitRate(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t rate = data.ReadInt32();
    reply.WriteInt32(SetVideoEncodingBitRate(sourceId, rate));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetCaptureRate(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    double fps = data.ReadDouble();
    reply.WriteInt32(SetCaptureRate(sourceId, fps));
    return MSERR_OK;
}

int32_t RecorderServiceStub::GetSurface(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    sptr<OHOS::Surface> surface = GetSurface(sourceId);
    if (surface != nullptr && surface->GetProducer() != nullptr) {
        sptr<IRemoteObject> object = surface->GetProducer()->AsObject();
        (void)reply.WriteRemoteObject(object);
    }
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetAudioSource(MessageParcel &data, MessageParcel &reply)
{
    int32_t type = data.ReadInt32();
    int32_t sourceId = 0;
    AudioSourceType sourceType = static_cast<AudioSourceType>(type);
    int32_t ret = SetAudioSource(sourceType, sourceId);
    reply.WriteInt32(sourceId);
    reply.WriteInt32(ret);
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetAudioEncoder(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t format = data.ReadInt32();
    AudioCodecFormat encoderFormat = static_cast<AudioCodecFormat>(format);
    reply.WriteInt32(SetAudioEncoder(sourceId, encoderFormat));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetAudioSampleRate(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t rate = data.ReadInt32();
    reply.WriteInt32(SetAudioSampleRate(sourceId, rate));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetAudioChannels(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t num = data.ReadInt32();
    reply.WriteInt32(SetAudioChannels(sourceId, num));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetAudioEncodingBitRate(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t bitRate = data.ReadInt32();
    reply.WriteInt32(SetAudioEncodingBitRate(sourceId, bitRate));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetDataSource(MessageParcel &data, MessageParcel &reply)
{
    int32_t type = data.ReadInt32();
    int32_t sourceId = 0;
    DataSourceType dataType = static_cast<DataSourceType>(type);
    int32_t ret = SetDataSource(dataType, sourceId);
    reply.WriteInt32(sourceId);
    reply.WriteInt32(ret);
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetMaxDuration(MessageParcel &data, MessageParcel &reply)
{
    int32_t duration = data.ReadInt32();
    reply.WriteInt32(SetMaxDuration(duration));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetOutputFormat(MessageParcel &data, MessageParcel &reply)
{
    int32_t type = data.ReadInt32();
    OutputFormatType formatType = static_cast<OutputFormatType>(type);
    reply.WriteInt32(SetOutputFormat(formatType));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetOutputPath(MessageParcel &data, MessageParcel &reply)
{
    std::string path = data.ReadString();
    reply.WriteInt32(SetOutputPath(path));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetOutputFile(MessageParcel &data, MessageParcel &reply)
{
    int32_t fd = data.ReadFileDescriptor();
    reply.WriteInt32(SetOutputFile(fd));
    (void)::close(fd);
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetNextOutputFile(MessageParcel &data, MessageParcel &reply)
{
    int32_t fd = data.ReadFileDescriptor();
    reply.WriteInt32(SetNextOutputFile(fd));
    (void)::close(fd);
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetMaxFileSize(MessageParcel &data, MessageParcel &reply)
{
    int64_t size = data.ReadInt64();
    reply.WriteInt32(SetMaxFileSize(size));
    return MSERR_OK;
}

int32_t RecorderServiceStub::Prepare(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Prepare());
    return MSERR_OK;
}

int32_t RecorderServiceStub::Start(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Start());
    return MSERR_OK;
}

int32_t RecorderServiceStub::Pause(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Pause());
    return MSERR_OK;
}

int32_t RecorderServiceStub::Resume(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Resume());
    return MSERR_OK;
}

int32_t RecorderServiceStub::Stop(MessageParcel &data, MessageParcel &reply)
{
    bool block = data.ReadBool();
    reply.WriteInt32(Stop(block));
    return MSERR_OK;
}

int32_t RecorderServiceStub::Reset(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Reset());
    return MSERR_OK;
}

int32_t RecorderServiceStub::Release(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Release());
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetFileSplitDuration(MessageParcel &data, MessageParcel &reply)
{
    int32_t type = data.ReadInt32();
    FileSplitType splitType = static_cast<FileSplitType>(type);
    int64_t timestamp = data.ReadInt64();
    uint32_t duration = data.ReadUint32();
    reply.WriteInt32(SetFileSplitDuration(splitType, timestamp, duration));
    return MSERR_OK;
}

int32_t RecorderServiceStub::DestroyStub(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(DestroyStub());
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
