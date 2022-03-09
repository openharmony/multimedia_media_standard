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

#include "recorder_client.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderClient"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<RecorderClient> RecorderClient::Create(const sptr<IStandardRecorderService> &ipcProxy)
{
    CHECK_AND_RETURN_RET_LOG(ipcProxy != nullptr, nullptr, "ipcProxy is nullptr..");

    std::shared_ptr<RecorderClient> recorder = std::make_shared<RecorderClient>(ipcProxy);
    CHECK_AND_RETURN_RET_LOG(recorder != nullptr, nullptr, "failed to new RecorderClient..");

    int32_t ret = recorder->CreateListenerObject();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to create listener object..");

    return recorder;
}

RecorderClient::RecorderClient(const sptr<IStandardRecorderService> &ipcProxy)
    : recorderProxy_(ipcProxy)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

RecorderClient::~RecorderClient()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (recorderProxy_ != nullptr) {
        (void)recorderProxy_->DestroyStub();
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void RecorderClient::MediaServerDied()
{
    std::lock_guard<std::mutex> lock(mutex_);
    recorderProxy_ = nullptr;
    listenerStub_ = nullptr;
    if (callback_ != nullptr) {
        callback_->OnError(RECORDER_ERROR_INTERNAL, MSERR_SERVICE_DIED);
    }
}

int32_t RecorderClient::CreateListenerObject()
{
    std::lock_guard<std::mutex> lock(mutex_);
    listenerStub_ = new(std::nothrow) RecorderListenerStub();
    CHECK_AND_RETURN_RET_LOG(listenerStub_ != nullptr, MSERR_NO_MEMORY, "failed to new RecorderListenerStub object");
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    sptr<IRemoteObject> object = listenerStub_->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, MSERR_NO_MEMORY, "listener object is nullptr..");

    MEDIA_LOGD("SetListenerObject");
    return recorderProxy_->SetListenerObject(object);
}

int32_t RecorderClient::SetVideoSource(VideoSourceType source, int32_t &sourceId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    MEDIA_LOGD("SetVideoSource source(%{public}d), sourceId(%{public}d)", source, sourceId);
    return recorderProxy_->SetVideoSource(source, sourceId);
}

int32_t RecorderClient::SetVideoEncoder(int32_t sourceId, VideoCodecFormat encoder)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    MEDIA_LOGD("SetVideoSource sourceId(%{public}d), encoder(%{public}d)", sourceId, encoder);
    return recorderProxy_->SetVideoEncoder(sourceId, encoder);
}

int32_t RecorderClient::SetVideoSize(int32_t sourceId, int32_t width, int32_t height)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    MEDIA_LOGD("SetVideoSize sourceId(%{public}d), width(%{public}d), height(%{public}d)", sourceId, width, height);
    return recorderProxy_->SetVideoSize(sourceId, width, height);
}

int32_t RecorderClient::SetVideoFrameRate(int32_t sourceId, int32_t frameRate)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    MEDIA_LOGD("SetVideoFrameRate sourceId(%{public}d), frameRate(%{public}d)", sourceId, frameRate);
    return recorderProxy_->SetVideoFrameRate(sourceId, frameRate);
}

int32_t RecorderClient::SetVideoEncodingBitRate(int32_t sourceId, int32_t rate)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    MEDIA_LOGD("SetVideoEncodingBitRate sourceId(%{public}d), rate(%{public}d)", sourceId, rate);
    return recorderProxy_->SetVideoEncodingBitRate(sourceId, rate);
}

int32_t RecorderClient::SetCaptureRate(int32_t sourceId, double fps)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    MEDIA_LOGD("SetCaptureRate sourceId(%{public}d), fps(%{public}lf)", sourceId, fps);
    return recorderProxy_->SetCaptureRate(sourceId, fps);
}

sptr<OHOS::Surface> RecorderClient::GetSurface(int32_t sourceId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, nullptr, "recorder service does not exist.");

    MEDIA_LOGD("GetSurface sourceId(%{public}d)", sourceId);
    return recorderProxy_->GetSurface(sourceId);
}

int32_t RecorderClient::SetAudioSource(AudioSourceType source, int32_t &sourceId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    MEDIA_LOGD("SetAudioSource source(%{public}d), sourceId(%{public}d)", source, sourceId);
    return recorderProxy_->SetAudioSource(source, sourceId);
}

int32_t RecorderClient::SetAudioEncoder(int32_t sourceId, AudioCodecFormat encoder)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    MEDIA_LOGD("SetAudioEncoder sourceId(%{public}d), encoder(%{public}d)", sourceId, encoder);
    return recorderProxy_->SetAudioEncoder(sourceId, encoder);
}

int32_t RecorderClient::SetAudioSampleRate(int32_t sourceId, int32_t rate)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    MEDIA_LOGD("SetAudioSampleRate sourceId(%{public}d), rate(%{public}d)", sourceId, rate);
    return recorderProxy_->SetAudioSampleRate(sourceId, rate);
}

int32_t RecorderClient::SetAudioChannels(int32_t sourceId, int32_t num)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    MEDIA_LOGD("SetAudioChannels sourceId(%{public}d), num(%{public}d)", sourceId, num);
    return recorderProxy_->SetAudioChannels(sourceId, num);
}

int32_t RecorderClient::SetAudioEncodingBitRate(int32_t sourceId, int32_t bitRate)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    MEDIA_LOGD("SetAudioEncodingBitRate sourceId(%{public}d), bitRate(%{public}d)", sourceId, bitRate);
    return recorderProxy_->SetAudioEncodingBitRate(sourceId, bitRate);
}

int32_t RecorderClient::SetDataSource(DataSourceType dataType, int32_t &sourceId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    MEDIA_LOGD("SetDataSource dataType(%{public}d), sourceId(%{public}d)", dataType, sourceId);
    return recorderProxy_->SetDataSource(dataType, sourceId);
}

int32_t RecorderClient::SetMaxDuration(int32_t duration)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    MEDIA_LOGD("SetMaxDuration duration(%{public}d)", duration);
    return recorderProxy_->SetMaxDuration(duration);
}

int32_t RecorderClient::SetOutputFormat(OutputFormatType format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    MEDIA_LOGD("SetOutputFormat format(%{public}d)", format);
    return recorderProxy_->SetOutputFormat(format);
}

int32_t RecorderClient::SetOutputPath(const std::string &path)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    MEDIA_LOGD("SetOutputPath path(%{private}s)", path.c_str());
    return recorderProxy_->SetOutputPath(path);
}

int32_t RecorderClient::SetOutputFile(int32_t fd)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    MEDIA_LOGD("SetOutputFile fd(%{public}d)", fd);
    return recorderProxy_->SetOutputFile(fd);
}

int32_t RecorderClient::SetNextOutputFile(int32_t fd)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    MEDIA_LOGD("SetNextOutputFile fd(%{public}d)", fd);
    return recorderProxy_->SetNextOutputFile(fd);
}

int32_t RecorderClient::SetMaxFileSize(int64_t size)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    MEDIA_LOGD("SetMaxFileSize size(%{public}" PRId64 ")", size);
    return recorderProxy_->SetMaxFileSize(size);
}

void RecorderClient::SetLocation(float latitude, float longitude)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(recorderProxy_ != nullptr, "recorder service does not exist.");

    recorderProxy_->SetLocation(latitude, longitude);
}

void RecorderClient::SetOrientationHint(int32_t rotation)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(recorderProxy_ != nullptr, "recorder service does not exist.");

    MEDIA_LOGD ("SetLocation orientation hint: %{public}d", rotation);
    recorderProxy_->SetOrientationHint(rotation);
}

int32_t RecorderClient::SetRecorderCallback(const std::shared_ptr<RecorderCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_NO_MEMORY, "input param callback is nullptr.");
    CHECK_AND_RETURN_RET_LOG(listenerStub_ != nullptr, MSERR_NO_MEMORY, "listenerStub_ is nullptr.");

    callback_ = callback;
    MEDIA_LOGD("SetRecorderCallback");
    listenerStub_->SetRecorderCallback(callback);
    return MSERR_OK;
}

int32_t RecorderClient::Prepare()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    MEDIA_LOGD("Prepare");
    return recorderProxy_->Prepare();
}

int32_t RecorderClient::Start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    MEDIA_LOGD("Start");
    return recorderProxy_->Start();
}

int32_t RecorderClient::Pause()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    MEDIA_LOGD("Pause");
    return recorderProxy_->Pause();
}

int32_t RecorderClient::Resume()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    MEDIA_LOGD("Resume");
    return recorderProxy_->Resume();
}

int32_t RecorderClient::Stop(bool block)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    MEDIA_LOGD("Stop");
    return recorderProxy_->Stop(block);
}

int32_t RecorderClient::Reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    MEDIA_LOGD("Reset");
    return recorderProxy_->Reset();
}

int32_t RecorderClient::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    MEDIA_LOGD("Release");
    return recorderProxy_->Release();
}

int32_t RecorderClient::SetFileSplitDuration(FileSplitType type, int64_t timestamp, uint32_t duration)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    MEDIA_LOGD("SetFileSplitDuration FileSplitType(%{public}d), timestamp(%{public}" PRId64 "), duration(%{public}u)",
        type, timestamp, duration);
    return recorderProxy_->SetFileSplitDuration(type, timestamp, duration);
}

int32_t RecorderClient::SetParameter(int32_t sourceId, const Format &format)
{
    return MSERR_INVALID_OPERATION;
}
} // namespace Media
} // namespace OHOS
