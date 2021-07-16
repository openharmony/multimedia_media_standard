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

#include "audio_sink_sv_impl.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioSinkSvImpl"};
}

namespace OHOS {
namespace Media {
AudioSinkSvImpl::AudioSinkSvImpl()
    : audioRenderer_(nullptr),
      audioManager_(nullptr)
{
}

AudioSinkSvImpl::~AudioSinkSvImpl()
{
    audioManager_ = nullptr;
    if (audioRenderer_ != nullptr) {
        audioRenderer_->Release();
        audioRenderer_ = nullptr;
    }
}

int32_t AudioSinkSvImpl::SetVolume(float volume)
{
    MEDIA_LOGD("SetVolume");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(audioManager_ != nullptr, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(audioManager_->SetVolume(AudioStandard::AudioSystemManager::AudioVolumeType::STREAM_MUSIC,
                                                  volume) == AudioStandard::SUCCESS, MSERR_UNKNOWN);
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::GetVolume(float &volume)
{
    MEDIA_LOGD("GetVolume");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(audioManager_ != nullptr, MSERR_INVALID_OPERATION);
    volume = audioManager_->GetVolume(AudioStandard::AudioSystemManager::AudioVolumeType::STREAM_MUSIC);
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::GetMaxVolume(float &volume)
{
    MEDIA_LOGD("GetMaxVolume");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(audioManager_ != nullptr, MSERR_INVALID_OPERATION);
    volume = audioManager_->GetMaxVolume(AudioStandard::AudioSystemManager::AudioVolumeType::STREAM_MUSIC);
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::GetMinVolume(float &volume)
{
    MEDIA_LOGD("GetMinVolume");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(audioManager_ != nullptr, MSERR_INVALID_OPERATION);
    volume = audioManager_->GetMinVolume(AudioStandard::AudioSystemManager::AudioVolumeType::STREAM_MUSIC);
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::Prepare()
{
    MEDIA_LOGD("Prepare");
    audioRenderer_ = AudioStandard::AudioRenderer::Create(AudioStandard::AudioStreamType::STREAM_MUSIC);
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);
    audioManager_ = AudioStandard::AudioSystemManager::GetInstance();
    CHECK_AND_RETURN_RET(audioManager_ != nullptr, MSERR_INVALID_OPERATION);
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::Start()
{
    MEDIA_LOGD("Start");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(audioRenderer_->Start() == true, MSERR_UNKNOWN);
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::Stop()
{
    MEDIA_LOGD("Stop");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(audioRenderer_->Stop() == true, MSERR_UNKNOWN);
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::Drain()
{
    MEDIA_LOGD("Drain");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(audioRenderer_->Drain() == true, MSERR_UNKNOWN);
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::Release()
{
    MEDIA_LOGD("Release");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(audioRenderer_->Release() == true, MSERR_UNKNOWN);
    audioRenderer_ = nullptr;
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::SetParameters(uint32_t bitsPerSample, uint32_t channels, uint32_t sampleRate)
{
    MEDIA_LOGD("SetParameters, channels:%{public}d, sampleRate:%{public}d", channels, sampleRate);
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);
    AudioStandard::AudioRendererParams params;
    if (sampleRate <= AudioStandard::SAMPLE_RATE_8000) {
        params.sampleRate = AudioStandard::SAMPLE_RATE_8000;
    } else if (sampleRate <= AudioStandard::SAMPLE_RATE_11025) {
        params.sampleRate = AudioStandard::SAMPLE_RATE_11025;
    } else if (sampleRate <= AudioStandard::SAMPLE_RATE_16000) {
        params.sampleRate = AudioStandard::SAMPLE_RATE_16000;
    } else if (sampleRate <= AudioStandard::SAMPLE_RATE_22050) {
        params.sampleRate = AudioStandard::SAMPLE_RATE_22050;
    } else if (sampleRate <= AudioStandard::SAMPLE_RATE_32000) {
        params.sampleRate = AudioStandard::SAMPLE_RATE_32000;
    } else if (sampleRate <= AudioStandard::SAMPLE_RATE_44100) {
        params.sampleRate = AudioStandard::SAMPLE_RATE_44100;
    } else if (sampleRate <= AudioStandard::SAMPLE_RATE_48000) {
        params.sampleRate = AudioStandard::SAMPLE_RATE_48000;
    } else {
        params.sampleRate = AudioStandard::SAMPLE_RATE_48000;
    }

    if (channels == 1) {
        params.channelCount = AudioStandard::MONO;
    } else if (channels == 2) {
        params.channelCount = AudioStandard::STEREO;
    } else {
        MEDIA_LOGE("unsupported channels count");
        return MSERR_UNSUPPORT_AUD_CHANNEL_NUM;
    }
    params.sampleFormat = AudioStandard::SAMPLE_S16LE;
    params.encodingType = AudioStandard::ENCODING_PCM;
    CHECK_AND_RETURN_RET(audioRenderer_->SetParams(params) == AudioStandard::SUCCESS, MSERR_UNKNOWN);
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::GetParameters(uint32_t &bitsPerSample, uint32_t &channels, uint32_t &sampleRate)
{
    MEDIA_LOGD("GetParameters");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);
    AudioStandard::AudioRendererParams params;
    CHECK_AND_RETURN_RET(audioRenderer_->GetParams(params) == AudioStandard::SUCCESS, MSERR_UNKNOWN);
    channels = params.channelCount;
    sampleRate = params.sampleRate;
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::GetMinimumBufferSize(uint32_t &bufferSize)
{
    MEDIA_LOGD("GetMinimumBufferSize");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);
    uint32_t size = 0;
    CHECK_AND_RETURN_RET(audioRenderer_->GetBufferSize(size) == AudioStandard::SUCCESS, MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(size > 0, MSERR_UNKNOWN);
    bufferSize = size;
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::GetMinimumFrameCount(uint32_t &frameCount)
{
    MEDIA_LOGD("GetMinimumFrameCount");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);
    uint32_t count = 0;
    CHECK_AND_RETURN_RET(audioRenderer_->GetFrameCount(count) == AudioStandard::SUCCESS, MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(count > 0, MSERR_UNKNOWN);
    frameCount = count;
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::Write(uint8_t *buffer, uint64_t size)
{
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(audioRenderer_->Write(buffer, size) > 0, MSERR_UNKNOWN);
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::GetAudioTime(uint64_t &time)
{
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);
    AudioStandard::Timestamp timeStamp;
    CHECK_AND_RETURN_RET(audioRenderer_->GetAudioTime(timeStamp, AudioStandard::Timestamp::Timestampbase::MONOTONIC)
                                         == true, MSERR_UNKNOWN);
    time = timeStamp.time.tv_nsec;
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::GetLatency(uint64_t &latency) const
{
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(audioRenderer_->GetLatency(latency) == AudioStandard::SUCCESS, MSERR_UNKNOWN);
    return MSERR_OK;
}
}  // namespace Media
}  // namespace OHOS
