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
#include "media_log.h"
#include "audio_errors.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioCaptureAsImpl"};
    constexpr uint32_t MAXIMUM_BUFFER_SIZE = 100000;
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
    MEDIA_LOGD("SetCaptureParameter in, channels:%{public}d, sampleRate:%{public}d", channels, sampleRate);
    audioCapturer_ = AudioStandard::AudioCapturer::Create(AudioStandard::AudioStreamType::STREAM_MUSIC);
    CHECK_AND_RETURN_RET(audioCapturer_ != nullptr, MSERR_NO_MEMORY);

    AudioStandard::AudioCapturerParams params;
    std::vector<AudioStandard::AudioSamplingRate> supportedSampleList = AudioStandard::
                                                                        AudioCapturer::GetSupportedSamplingRates();
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
    CHECK_AND_RETURN_RET(isValidSampleRate == true, MSERR_UNSUPPORT_AUD_SAMPLE_RATE);

    std::vector<AudioStandard::AudioChannel> supportedChannelsList = AudioStandard::
                                                                     AudioCapturer::GetSupportedChannels();
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
    CHECK_AND_RETURN_RET(isValidChannels == true, MSERR_UNSUPPORT_AUD_CHANNEL_NUM);

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
    CHECK_AND_RETURN_RET(bufferSize_ > 0, MSERR_UNKNOWN);
    const uint32_t bitsPerByte = 8;
    double durationSec = static_cast<double>(bufferSize_) /
        (sampleRate * (AudioStandard::SAMPLE_S16LE / bitsPerByte) * channels);
    const uint32_t secToNanosecond = 1000000000;
    CHECK_AND_RETURN_RET_LOG((UINT32_MAX / secToNanosecond) >= durationSec, MSERR_UNKNOWN,
                             "duration too long, this shouldn't happen");
    duration_ = static_cast<uint32_t>(durationSec * secToNanosecond);
    MEDIA_LOGD("audio frame duration is %{public}u ns", duration_);
    return MSERR_OK;
}

int32_t AudioCaptureAsImpl::GetSegmentInfo(uint64_t &start)
{
    CHECK_AND_RETURN_RET(audioCapturer_ != nullptr, MSERR_INVALID_OPERATION);
    AudioStandard::Timestamp timeStamp;
    CHECK_AND_RETURN_RET(
        audioCapturer_->GetAudioTime(timeStamp, AudioStandard::Timestamp::Timestampbase::MONOTONIC) == true,
        MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(timeStamp.time.tv_nsec >= 0 && timeStamp.time.tv_sec >= 0, MSERR_UNKNOWN);
    const uint64_t secToNanosecond = 1000000000;
    if (((UINT64_MAX - timeStamp.time.tv_nsec) / secToNanosecond) <= static_cast<uint64_t>(timeStamp.time.tv_sec)) {
        MEDIA_LOGW("audio frame pts too long, this shouldn't happen");
    }
    start = timeStamp.time.tv_nsec + timeStamp.time.tv_sec * secToNanosecond;
    return MSERR_OK;
}

int32_t AudioCaptureAsImpl::StartAudioCapture()
{
    MEDIA_LOGD("StartAudioCapture");
    CHECK_AND_RETURN_RET(audioCapturer_ != nullptr, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(audioCapturer_->Start(), MSERR_UNKNOWN);
    return MSERR_OK;
}

std::shared_ptr<AudioBuffer> AudioCaptureAsImpl::GetBuffer()
{
    CHECK_AND_RETURN_RET(audioCapturer_ != nullptr, nullptr);
    std::shared_ptr<AudioBuffer> buffer = std::make_shared<AudioBuffer>();
    CHECK_AND_RETURN_RET(buffer != nullptr, nullptr);
    CHECK_AND_RETURN_RET(bufferSize_ > 0, nullptr);
    buffer->gstBuffer = gst_buffer_new_allocate(nullptr, bufferSize_, nullptr);
    if (buffer->gstBuffer == nullptr) {
        return nullptr;
    }
    GstMapInfo map;
    if (gst_buffer_map(buffer->gstBuffer, &map, GST_MAP_READ) != TRUE) {
        g_free(buffer->gstBuffer);
        return nullptr;
    }
    bool isBlocking = true;
    int32_t bytesRead = audioCapturer_->Read(*(map.data), map.size, isBlocking);
    gst_buffer_unmap(buffer->gstBuffer, &map);
    if (bytesRead <= 0) {
        g_free(buffer->gstBuffer);
        return nullptr;
    }

    if (queryTimestamp_) {
        if (GetSegmentInfo(buffer->timestamp) != MSERR_OK) {
            g_free(buffer->gstBuffer);
            return nullptr;
        }
    } else {
        timestamp_ += duration_;
        buffer->timestamp = timestamp_;
    }

    buffer->duration = duration_;
    buffer->dataLen = bufferSize_;
    sequence_++;
    buffer->dataSeq = sequence_;
    return buffer;
}

int32_t AudioCaptureAsImpl::StopAudioCapture()
{
    MEDIA_LOGD("StopAudioCapture");
    CHECK_AND_RETURN_RET(audioCapturer_ != nullptr, MSERR_INVALID_OPERATION);
    if (audioCapturer_->GetStatus() == AudioStandard::CapturerState::CAPTURER_RUNNING) {
        CHECK_AND_RETURN_RET(audioCapturer_->Flush(), MSERR_UNKNOWN);
        CHECK_AND_RETURN_RET(audioCapturer_->Stop(), MSERR_UNKNOWN);
    }
    if (audioCapturer_->GetStatus() != AudioStandard::CapturerState::CAPTURER_RELEASED) {
        CHECK_AND_RETURN_RET(audioCapturer_->Release(), MSERR_UNKNOWN);
    }
    audioCapturer_ = nullptr;
    return MSERR_OK;
}
}  // namespace Media
}  // namespace OHOS
