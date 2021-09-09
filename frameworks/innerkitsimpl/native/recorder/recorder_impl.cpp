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

#include "recorder_impl.h"
#include <map>
#include "i_media_service.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderImpl"};
}

namespace OHOS {
namespace Media {
const std::map<RecorderErrorType, std::string> RECORDER_ERRTYPE_INFOS = {
    {RECORDER_ERROR_INTERNAL, "internal error type"},
    {RECORDER_ERROR_EXTEND_START, "recorder extend start error type"},
};

std::string RecorderErrorTypeToString(RecorderErrorType type)
{
    if (RECORDER_ERRTYPE_INFOS.count(type) != 0) {
        return RECORDER_ERRTYPE_INFOS.at(type);
    }

    if (type > RECORDER_ERROR_EXTEND_START) {
        return "extend error type:" + std::to_string(static_cast<int32_t>(type - RECORDER_ERROR_EXTEND_START));
    }

    return "invalid error type:" + std::to_string(static_cast<int32_t>(type));
}

std::shared_ptr<Recorder> RecorderFactory::CreateRecorder()
{
    std::shared_ptr<RecorderImpl> impl = std::make_shared<RecorderImpl>();
    CHECK_AND_RETURN_RET_LOG(impl != nullptr, nullptr, "failed to new RecorderImpl");

    int32_t ret = impl->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to init RecorderImpl");
    return impl;
}

int32_t RecorderImpl::Init()
{
    recorderService_ = MeidaServiceFactory::GetInstance().CreateRecorderService();
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_NO_MEMORY, "failed to create recorder service");
    return MSERR_OK;
}

RecorderImpl::RecorderImpl()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

RecorderImpl::~RecorderImpl()
{
    if (recorderService_ != nullptr) {
        (void)MeidaServiceFactory::GetInstance().DestroyRecorderService(recorderService_);
        recorderService_ = nullptr;
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t RecorderImpl::SetVideoSource(VideoSourceType source, int32_t &sourceId)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetVideoSource(source, sourceId);
}

int32_t RecorderImpl::SetVideoEncoder(int32_t sourceId, VideoCodecFormat encoder)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetVideoEncoder(sourceId, encoder);
}

int32_t RecorderImpl::SetVideoSize(int32_t sourceId, int32_t width, int32_t height)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetVideoSize(sourceId, width, height);
}

int32_t RecorderImpl::SetVideoFrameRate(int32_t sourceId, int32_t frameRate)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetVideoFrameRate(sourceId, frameRate);
}

int32_t RecorderImpl::RecorderImpl::SetVideoEncodingBitRate(int32_t sourceId, int32_t rate)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetVideoEncodingBitRate(sourceId, rate);
}

int32_t RecorderImpl::SetCaptureRate(int32_t sourceId, double fps)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetCaptureRate(sourceId, fps);
}

sptr<OHOS::Surface> RecorderImpl::GetSurface(int32_t sourceId)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, nullptr, "recorder service does not exist..");
    return recorderService_->GetSurface(sourceId);
}

int32_t RecorderImpl::SetAudioSource(AudioSourceType source, int32_t &sourceId)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetAudioSource(source, sourceId);
}

int32_t RecorderImpl::SetAudioEncoder(int32_t sourceId, AudioCodecFormat encoder)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetAudioEncoder(sourceId, encoder);
}

int32_t RecorderImpl::SetAudioSampleRate(int32_t sourceId, int32_t rate)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetAudioSampleRate(sourceId, rate);
}

int32_t RecorderImpl::SetAudioChannels(int32_t sourceId, int32_t num)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetAudioChannels(sourceId, num);
}

int32_t RecorderImpl::SetAudioEncodingBitRate(int32_t sourceId, int32_t bitRate)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetAudioEncodingBitRate(sourceId, bitRate);
}

int32_t RecorderImpl::SetDataSource(DataSourceType dataType, int32_t &sourceId)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetDataSource(dataType, sourceId);
}

int32_t RecorderImpl::SetMaxDuration(int32_t duration)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetMaxDuration(duration);
}

int32_t RecorderImpl::SetOutputFormat(OutputFormatType format)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetOutputFormat(format);
}

int32_t RecorderImpl::SetOutputPath(const std::string &path)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetOutputPath(path);
}

int32_t RecorderImpl::SetOutputFile(int32_t fd)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetOutputFile(fd);
}

int32_t RecorderImpl::SetNextOutputFile(int32_t fd)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetNextOutputFile(fd);
}

int32_t RecorderImpl::SetMaxFileSize(int64_t size)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetMaxFileSize(size);
}

int32_t RecorderImpl::SetRecorderCallback(const std::shared_ptr<RecorderCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_INVALID_VAL, "input callback is nullptr.");
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetRecorderCallback(callback);
}

int32_t RecorderImpl::Prepare()
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->Prepare();
}

int32_t RecorderImpl::Start()
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->Start();
}

int32_t RecorderImpl::Pause()
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->Pause();
}

int32_t RecorderImpl::Resume()
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->Resume();
}

int32_t RecorderImpl::Stop(bool block)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->Stop(block);
}

int32_t RecorderImpl::Reset()
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->Reset();
}

int32_t RecorderImpl::Release()
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    (void)recorderService_->Release();
    (void)MeidaServiceFactory::GetInstance().DestroyRecorderService(recorderService_);
    recorderService_ = nullptr;
    return MSERR_OK;
}

int32_t RecorderImpl::SetFileSplitDuration(FileSplitType type, int64_t timestamp, uint32_t duration)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetFileSplitDuration(type, timestamp, duration);
}

int32_t RecorderImpl::SetParameter(int32_t sourceId, const Format &format)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetParameter(sourceId, format);
}
} // Media
} // OHOS
