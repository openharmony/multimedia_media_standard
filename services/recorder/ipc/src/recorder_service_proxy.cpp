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

#include "recorder_service_proxy.h"
#include "recorder_listener_stub.h"
#include "media_log.h"
#include "errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderServiceProxy"};
}

namespace OHOS {
namespace Media {
RecorderServiceProxy::RecorderServiceProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardRecorderService>(impl)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

RecorderServiceProxy::~RecorderServiceProxy()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t RecorderServiceProxy::SetListenerObject(const sptr<IRemoteObject> &object)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteRemoteObject(object);
    int error = Remote()->SendRequest(SET_LISTENER_OBJ, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Set listener obj failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t RecorderServiceProxy::SetVideoSource(VideoSourceType source, int32_t &sourceId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(static_cast<int32_t>(source));
    int error = Remote()->SendRequest(SET_VIDEO_SOURCE, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Set video source failed, error: %{public}d", error);
        return error;
    }
    sourceId = reply.ReadInt32();
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t RecorderServiceProxy::SetVideoEncoder(int32_t sourceId, VideoCodecFormat encoder)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(static_cast<int>(sourceId));
    data.WriteInt32(static_cast<int>(encoder));
    int error = Remote()->SendRequest(SET_VIDEO_ENCODER, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Set video encoder failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t RecorderServiceProxy::SetVideoSize(int32_t sourceId, int32_t width, int32_t height)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(sourceId);
    data.WriteInt32(width);
    data.WriteInt32(height);
    int error = Remote()->SendRequest(SET_VIDEO_SIZE, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Set video size failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t RecorderServiceProxy::SetVideoFrameRate(int32_t sourceId, int32_t frameRate)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(sourceId);
    data.WriteInt32(frameRate);
    int error = Remote()->SendRequest(SET_VIDEO_FARAME_RATE, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Set video frame rate failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t RecorderServiceProxy::SetVideoEncodingBitRate(int32_t sourceId, int32_t rate)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(sourceId);
    data.WriteInt32(rate);
    int error = Remote()->SendRequest(SET_VIDEO_ENCODING_BIT_RATE, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Set video encoding bit rate failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t RecorderServiceProxy::SetCaptureRate(int32_t sourceId, double fps)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(sourceId);
    data.WriteDouble(fps);
    int error = Remote()->SendRequest(SET_CAPTURE_RATE, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Set capture rate failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

sptr<OHOS::Surface> RecorderServiceProxy::GetSurface(int32_t sourceId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(sourceId);
    int error = Remote()->SendRequest(GET_SURFACE, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Get surface failed, error: %{public}d", error);
        return nullptr;
    }

    sptr<IRemoteObject> object = reply.ReadRemoteObject();
    if (object == nullptr) {
        MEDIA_LOGE("failed to read surface object");
        return nullptr;
    }

    sptr<IBufferProducer> producer = iface_cast<IBufferProducer>(object);
    if (producer == nullptr) {
        MEDIA_LOGE("failed to convert object to producer");
        return nullptr;
    }

    return OHOS::Surface::CreateSurfaceAsProducer(producer);
}

int32_t RecorderServiceProxy::SetAudioSource(AudioSourceType source, int32_t &sourceId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(static_cast<int32_t>(source));
    int error = Remote()->SendRequest(SET_AUDIO_SOURCE, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Set audio source failed, error: %{public}d", error);
        return error;
    }
    sourceId = reply.ReadInt32();
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t RecorderServiceProxy::SetAudioEncoder(int32_t sourceId, AudioCodecFormat encoder)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(sourceId);
    data.WriteInt32(static_cast<int32_t>(encoder));
    int error = Remote()->SendRequest(SET_AUDIO_ENCODER, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Set audio encoder failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t RecorderServiceProxy::SetAudioSampleRate(int32_t sourceId, int32_t rate)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(sourceId);
    data.WriteInt32(rate);
    int error = Remote()->SendRequest(SET_AUDIO_SAMPLE_RATE, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Set audio sample rate failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t RecorderServiceProxy::SetAudioChannels(int32_t sourceId, int32_t num)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(sourceId);
    data.WriteInt32(num);
    int error = Remote()->SendRequest(SET_AUDIO_CHANNELS, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Set audio channels failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t RecorderServiceProxy::SetAudioEncodingBitRate(int32_t sourceId, int32_t bitRate)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(sourceId);
    data.WriteInt32(bitRate);
    int error = Remote()->SendRequest(SET_AUDIO_ENCODING_BIT_RATE, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Set audio encoding bit rate failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t RecorderServiceProxy::SetDataSource(DataSourceType dataType, int32_t &sourceId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(static_cast<int32_t>(dataType));
    int error = Remote()->SendRequest(SET_DATA_SOURCE, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Set data source failed, error: %{public}d", error);
        return error;
    }
    sourceId = reply.ReadInt32();
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t RecorderServiceProxy::SetMaxDuration(int32_t duration)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(duration);
    int error = Remote()->SendRequest(SET_MAX_DURATION, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Set max duration failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t RecorderServiceProxy::SetOutputFormat(OutputFormatType format)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(static_cast<int32_t>(format));
    int error = Remote()->SendRequest(SET_OUTPUT_FORMAT, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Set output format failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t RecorderServiceProxy::SetOutputPath(const std::string &path)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteString(path);
    int error = Remote()->SendRequest(SET_OUTPUT_PATH, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Set output path failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t RecorderServiceProxy::SetOutputFile(int32_t fd)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(fd);
    int error = Remote()->SendRequest(SET_OUTPUT_FILE, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Set output fd failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t RecorderServiceProxy::SetNextOutputFile(int32_t fd)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(fd);
    int error = Remote()->SendRequest(SET_NEXT_OUTPUT_FILE, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Set next output fd failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t RecorderServiceProxy::SetMaxFileSize(int64_t size)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt64(size);
    int error = Remote()->SendRequest(SET_MAX_FILE_SIZE, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Set max file size failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t RecorderServiceProxy::Prepare()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(PREPARE, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("prepare failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t RecorderServiceProxy::Start()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(START, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("start failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t RecorderServiceProxy::Pause()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(PAUSE, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("pause failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t RecorderServiceProxy::Resume()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(RESUME, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("resume failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t RecorderServiceProxy::Stop(bool block)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteBool(block);
    int error = Remote()->SendRequest(STOP, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("stop failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t RecorderServiceProxy::Reset()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(RESET, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("reset failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t RecorderServiceProxy::Release()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(RELEASE, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("release failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t RecorderServiceProxy::SetFileSplitDuration(FileSplitType type, int64_t timestamp, uint32_t duration)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(static_cast<int32_t>(type));
    data.WriteInt64(timestamp);
    data.WriteUint32(duration);
    int error = Remote()->SendRequest(SET_FILE_SPLIT_DURATION, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("Set file split duration failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}

int32_t RecorderServiceProxy::DestroyStub()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(DESTROY, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("destroy failed, error: %{public}d", error);
        return error;
    }
    int32_t ret = reply.ReadInt32();
    return ret;
}
} // namespace Media
} // namespace OHOS