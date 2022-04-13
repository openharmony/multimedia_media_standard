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

#include "audio_capture_as_impl.h"
#include <vector>
#include <cmath>
#include "media_log.h"
#include "audio_errors.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioCaptureAsImpl"};
    constexpr size_t MAXIMUM_BUFFER_SIZE = 100000;
    constexpr uint64_t SEC_TO_NANOSECOND = 1000000000;
}

namespace OHOS {
namespace Media {
AudioCaptureAsImpl::AudioCaptureAsImpl()
{
}

AudioCaptureAsImpl::~AudioCaptureAsImpl()
{
    if (audioCapturer_ != nullptr) {
        (void)audioCapturer_->Release();
        audioCapturer_ = nullptr;
    }
}

int32_t AudioCaptureAsImpl::SetCaptureParameter(uint32_t bitrate, uint32_t channels, uint32_t sampleRate)
{
    (void)bitrate;
    MEDIA_LOGD("SetCaptureParameter in, channels:%{public}u, sampleRate:%{public}u", channels, sampleRate);
    if (audioCapturer_ == nullptr) {
        audioCapturer_ = AudioStandard::AudioCapturer::Create(AudioStandard::AudioStreamType::STREAM_MUSIC);
        CHECK_AND_RETURN_RET_LOG(audioCapturer_ != nullptr, MSERR_NO_MEMORY, "create audio capturer failed");
    }
    audioCacheCtrl_ = std::make_unique<AudioCacheCtrl>();
    CHECK_AND_RETURN_RET_LOG(audioCacheCtrl_ != nullptr, MSERR_NO_MEMORY, "create audio cache ctrl failed");

    AudioStandard::AudioCapturerParams params;
    auto supportedSampleList = AudioStandard::AudioCapturer::GetSupportedSamplingRates();
    CHECK_AND_RETURN_RET(supportedSampleList.size() > 0, MSERR_UNKNOWN);
    bool isValidSampleRate = false;
    for (auto iter = supportedSampleList.cbegin(); iter != supportedSampleList.cend(); ++iter) {
        CHECK_AND_RETURN_RET(static_cast<int32_t>(*iter) > 0, MSERR_UNKNOWN);
        uint32_t supportedSampleRate = static_cast<uint32_t>(*iter);
        if (sampleRate <= supportedSampleRate) {
            params.samplingRate = *iter;
            isValidSampleRate = true;
            break;
        }
    }
    CHECK_AND_RETURN_RET(isValidSampleRate, MSERR_UNSUPPORT_AUD_SAMPLE_RATE);

    auto supportedChannelsList = AudioStandard::AudioCapturer::GetSupportedChannels();
    CHECK_AND_RETURN_RET(supportedChannelsList.size() > 0, MSERR_UNKNOWN);
    bool isValidChannels = false;
    for (auto iter = supportedChannelsList.cbegin(); iter != supportedChannelsList.cend(); ++iter) {
        CHECK_AND_RETURN_RET(static_cast<int32_t>(*iter) > 0, MSERR_UNKNOWN);
        uint32_t supportedChannels = static_cast<uint32_t>(*iter);
        if (channels == supportedChannels) {
            params.audioChannel = *iter;
            isValidChannels = true;
            break;
        }
    }
    CHECK_AND_RETURN_RET(isValidChannels, MSERR_UNSUPPORT_AUD_CHANNEL_NUM);

    params.audioSampleFormat = AudioStandard::SAMPLE_S16LE;
    params.audioEncoding = AudioStandard::ENCODING_PCM;
    MEDIA_LOGD("SetCaptureParameter out, channels:%{public}d, sampleRate:%{public}d",
        params.audioChannel, params.samplingRate);
    CHECK_AND_RETURN_RET(audioCapturer_->SetParams(params) == AudioStandard::SUCCESS, MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(audioCapturer_->GetBufferSize(bufferSize_) == AudioStandard::SUCCESS, MSERR_UNKNOWN);
    MEDIA_LOGD("audio buffer size is: %{public}zu", bufferSize_);
    CHECK_AND_RETURN_RET_LOG(bufferSize_ < MAXIMUM_BUFFER_SIZE, MSERR_UNKNOWN, "audio buffer size too long");
    return MSERR_OK;
}

int32_t AudioCaptureAsImpl::GetCaptureParameter(uint32_t &bitrate, uint32_t &channels, uint32_t &sampleRate)
{
    (void)bitrate;
    MEDIA_LOGD("GetCaptureParameter");
    CHECK_AND_RETURN_RET(audioCapturer_ != nullptr, MSERR_INVALID_OPERATION);
    AudioStandard::AudioCapturerParams params;
    CHECK_AND_RETURN_RET(audioCapturer_->GetParams(params) == AudioStandard::SUCCESS, MSERR_UNKNOWN);
    channels = params.audioChannel;
    sampleRate = params.samplingRate;
    MEDIA_LOGD("get channels:%{public}u, sampleRate:%{public}u from audio server", channels, sampleRate);
    CHECK_AND_RETURN_RET(bufferSize_ > 0 && channels > 0 && sampleRate > 0, MSERR_UNKNOWN);
    constexpr uint32_t bitsPerByte = 8;
    bufferDurationNs_ = (bufferSize_ * SEC_TO_NANOSECOND) /
        (sampleRate * (AudioStandard::SAMPLE_S16LE / bitsPerByte) * channels);

    MEDIA_LOGD("audio frame duration is (%{public}" PRIu64 ") ns", bufferDurationNs_);
    return MSERR_OK;
}

int32_t AudioCaptureAsImpl::GetSegmentInfo(uint64_t &start)
{
    CHECK_AND_RETURN_RET(audioCapturer_ != nullptr, MSERR_INVALID_OPERATION);
    AudioStandard::Timestamp timeStamp;
    auto timestampBase = AudioStandard::Timestamp::Timestampbase::MONOTONIC;
    CHECK_AND_RETURN_RET(audioCapturer_->GetAudioTime(timeStamp, timestampBase), MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(timeStamp.time.tv_nsec >= 0 && timeStamp.time.tv_sec >= 0, MSERR_UNKNOWN);
    if (((UINT64_MAX - timeStamp.time.tv_nsec) / SEC_TO_NANOSECOND) <= static_cast<uint64_t>(timeStamp.time.tv_sec)) {
        MEDIA_LOGW("audio frame pts too long, this shouldn't happen");
    }
    start = timeStamp.time.tv_nsec + timeStamp.time.tv_sec * SEC_TO_NANOSECOND;
    MEDIA_LOGI("timestamp from audioCapturer: %{public}" PRIu64 "", start);
    return MSERR_OK;
}

void AudioCaptureAsImpl::GetAudioCaptureBuffer()
{
    while (true) {
        if (curState_.load() == RECORDER_STOP) {
            break;
        }

        {
            std::unique_lock<std::mutex> lock(pauseMutex_);
            audioCacheCtrl_->pauseCond_.wait(lock, [this]() {
                return curState_.load() == RECORDER_RUNNING || curState_.load() == RECORDER_RESUME;
            });
        }

        CHECK_AND_BREAK(audioCapturer_ != nullptr);
        std::shared_ptr<AudioBuffer> tempBuffer = std::make_shared<AudioBuffer>();
        CHECK_AND_BREAK(bufferSize_ > 0 && bufferSize_ < MAXIMUM_BUFFER_SIZE);
        tempBuffer->gstBuffer = gst_buffer_new_allocate(nullptr, bufferSize_, nullptr);
        CHECK_AND_BREAK(tempBuffer->gstBuffer != nullptr);

        GstMapInfo map = GST_MAP_INFO_INIT;
        if (gst_buffer_map(tempBuffer->gstBuffer, &map, GST_MAP_READ) != TRUE) {
            gst_buffer_unref(tempBuffer->gstBuffer);
            break;
        }
        bool isBlocking = true;
        int32_t bytesRead = audioCapturer_->Read(*(map.data), map.size, isBlocking);
        gst_buffer_unmap(tempBuffer->gstBuffer, &map);
        if (bytesRead <= 0) {
            gst_buffer_unref(tempBuffer->gstBuffer);
            break;
        }
        uint64_t curTimeStamp = 0;
        if (GetSegmentInfo(curTimeStamp) != MSERR_OK) {
            gst_buffer_unref(tempBuffer->gstBuffer);
            break;
        }

        tempBuffer->timestamp = curTimeStamp;
        tempBuffer->duration = bufferDurationNs_;
        tempBuffer->dataLen = bufferSize_;

        {
            std::unique_lock<std::mutex> loopLock(audioCacheCtrl_->captureMutex_);
            audioCacheCtrl_->captureQueue_.push(tempBuffer);
            MEDIA_LOGD("audio cache queue size is %{public}d", audioCacheCtrl_->captureQueue_.size());
            audioCacheCtrl_->captureCond_.notify_all();
        }
    }
}

std::shared_ptr<AudioBuffer> AudioCaptureAsImpl::GetBuffer()
{
    if (curState_.load() == RECORDER_PAUSED) {
        audioCacheCtrl_->pausedTime_ = audioCacheCtrl_->timestamp_;
        MEDIA_LOGD("audio pause timestamp %{public}" PRIu64 "", audioCacheCtrl_->pausedTime_);
    }

    std::unique_lock<std::mutex> loopLock(audioCacheCtrl_->captureMutex_);
    audioCacheCtrl_->captureCond_.wait(loopLock, [this]() { return audioCacheCtrl_->captureQueue_.size() > 0; });

    if (curState_.load() == RECORDER_STOP) {
        return nullptr;
    }

    std::shared_ptr<AudioBuffer> bufferOut = audioCacheCtrl_->captureQueue_.front();
    audioCacheCtrl_->captureQueue_.pop();

    if (curState_.load() == RECORDER_PAUSED) {
        audioCacheCtrl_->pausedTime_ = bufferOut->timestamp;
        MEDIA_LOGD("audio pause timestamp %{public}" PRIu64 "", audioCacheCtrl_->pausedTime_);
        MEDIA_LOGD("%{public}d audio buffer has been dropped", audioCacheCtrl_->captureQueue_.size());
        while (!audioCacheCtrl_->captureQueue_.empty()) {
            audioCacheCtrl_->captureQueue_.pop();
        }
    }
    if (curState_.load() == RECORDER_RESUME) {
        curState_.store(RECORDER_RUNNING);
        audioCacheCtrl_->resumeTime_ = bufferOut->timestamp;
        MEDIA_LOGD("audio resume timestamp %{public}" PRIu64 "", audioCacheCtrl_->resumeTime_);
        audioCacheCtrl_->persistTime_ = std::fabs(audioCacheCtrl_->resumeTime_ - audioCacheCtrl_->pausedTime_);
        audioCacheCtrl_->totalPauseTime_ += audioCacheCtrl_->persistTime_;
        MEDIA_LOGD("audio has %{public}d times pause, total PauseTime: %{public}" PRIu64 "",
            audioCacheCtrl_->pausedCount_, audioCacheCtrl_->totalPauseTime_);
    }
    bufferOut->timestamp = bufferOut->timestamp - audioCacheCtrl_->totalPauseTime_;
    audioCacheCtrl_->timestamp_ = bufferOut->timestamp;
    return bufferOut;
}

int32_t AudioCaptureAsImpl::StartAudioCapture()
{
    MEDIA_LOGD("StartAudioCapture");

    CHECK_AND_RETURN_RET(audioCapturer_ != nullptr, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(audioCapturer_->Start(), MSERR_UNKNOWN);

    curState_.store(RECORDER_RUNNING);
    captureLoop_ = std::make_unique<std::thread>(&AudioCaptureAsImpl::GetAudioCaptureBuffer, this);
    CHECK_AND_RETURN_RET_LOG(captureLoop_ != nullptr, MSERR_INVALID_OPERATION, "create audio cache thread failed");

    return MSERR_OK;
}

int32_t AudioCaptureAsImpl::StopAudioCapture()
{
    MEDIA_LOGD("StopAudioCapture");
    curState_.store(RECORDER_STOP);

    if (captureLoop_ != nullptr && captureLoop_->joinable()) {
        std::unique_lock<std::mutex> loopLock(audioCacheCtrl_->captureMutex_);
        audioCacheCtrl_->captureQueue_.push(nullptr); // to wake up the loop thread
        audioCacheCtrl_->captureCond_.notify_all();
        audioCacheCtrl_->pauseCond_.notify_all();
        loopLock.unlock();
        captureLoop_->join();
        captureLoop_.reset();
    }

    CHECK_AND_RETURN_RET(audioCapturer_ != nullptr, MSERR_INVALID_OPERATION);
    if (audioCapturer_->GetStatus() == AudioStandard::CapturerState::CAPTURER_RUNNING) {
        CHECK_AND_RETURN_RET(audioCapturer_->Stop(), MSERR_UNKNOWN);
    }
    if (audioCapturer_->GetStatus() != AudioStandard::CapturerState::CAPTURER_RELEASED) {
        CHECK_AND_RETURN_RET(audioCapturer_->Release(), MSERR_UNKNOWN);
    }
    audioCapturer_ = nullptr;
    audioCacheCtrl_ = nullptr;
    curState_.store(RECORDER_INITIALIZED);
    return MSERR_OK;
}

int32_t AudioCaptureAsImpl::PauseAudioCapture()
{
    MEDIA_LOGD("PauseAudioCapture");

    {
        std::unique_lock<std::mutex> lock(pauseMutex_);
        curState_.store(RECORDER_PAUSED);
        audioCacheCtrl_->pausedCount_++; // add one pause time count
    }

    CHECK_AND_RETURN_RET(audioCapturer_ != nullptr, MSERR_INVALID_OPERATION);
    if (audioCapturer_->GetStatus() == AudioStandard::CapturerState::CAPTURER_RUNNING) {
        CHECK_AND_RETURN_RET(audioCapturer_->Stop(), MSERR_UNKNOWN);
    }

    MEDIA_LOGD("exit PauseAudioCapture");
    return MSERR_OK;
}

int32_t AudioCaptureAsImpl::ResumeAudioCapture()
{
    MEDIA_LOGD("ResumeAudioCapture");

    CHECK_AND_RETURN_RET(audioCapturer_ != nullptr, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(audioCapturer_->Start(), MSERR_UNKNOWN);

    {
        std::unique_lock<std::mutex> lock(pauseMutex_);
        curState_.store(RECORDER_RESUME);
        audioCacheCtrl_->pauseCond_.notify_all();
    }

    MEDIA_LOGD("exit ResumeAudioCapture");
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
