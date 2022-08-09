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
#include <vector>
#include "media_log.h"
#include "media_errors.h"
#include "media_dfx.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioSinkSvImpl"};
    const std::string DEFAULT_CAPS = "audio/x-raw, format = (string) S16LE, layout = (string) interleaved";
}

namespace OHOS {
namespace Media {
AudioRendererMediaCallback::AudioRendererMediaCallback(GstBaseSink *audioSink)
{
    audioSink_ = audioSink;
}

void AudioRendererMediaCallback::SaveCallback(void (*interruptCb)(GstBaseSink *, guint, guint, guint))
{
    interruptCb_ = interruptCb;
}

void AudioRendererMediaCallback::OnInterrupt(const AudioStandard::InterruptEvent &interruptEvent)
{
    if (interruptCb_ != nullptr) {
        interruptCb_(audioSink_, interruptEvent.eventType, interruptEvent.forceType, interruptEvent.hintType);
    }
}

void AudioRendererMediaCallback::OnStateChange(const AudioStandard::RendererState state)
{
    MEDIA_LOGD("RenderState is %{public}d", static_cast<uint32_t>(state));
}

AudioSinkSvImpl::AudioSinkSvImpl(GstBaseSink *sink)
    : audioRenderer_(nullptr)
{
    audioRendererMediaCallback_ = std::make_shared<AudioRendererMediaCallback>(sink);
}

AudioSinkSvImpl::~AudioSinkSvImpl()
{
    if (audioRenderer_ != nullptr) {
        (void)audioRenderer_->Release();
        audioRenderer_ = nullptr;
    }
}

GstCaps *AudioSinkSvImpl::GetCaps()
{
    GstCaps *caps = gst_caps_from_string(DEFAULT_CAPS.c_str());
    CHECK_AND_RETURN_RET_LOG(caps != nullptr, nullptr, "caps is null");
    InitChannelRange(caps);
    InitRateRange(caps);
    return caps;
}

void AudioSinkSvImpl::InitChannelRange(GstCaps *caps) const
{
    CHECK_AND_RETURN_LOG(caps != nullptr, "caps is null");
    std::vector<AudioStandard::AudioChannel> supportedChannelsList = AudioStandard::
                                                                     AudioRenderer::GetSupportedChannels();
    GValue list = G_VALUE_INIT;
    (void)g_value_init(&list, GST_TYPE_LIST);
    for (auto channel : supportedChannelsList) {
        GValue value = G_VALUE_INIT;
        (void)g_value_init(&value, G_TYPE_INT);
        g_value_set_int(&value, channel);
        gst_value_list_append_value(&list, &value);
        g_value_unset(&value);
    }
    gst_caps_set_value(caps, "channels", &list);
    g_value_unset(&list);
}

void AudioSinkSvImpl::InitRateRange(GstCaps *caps) const
{
    CHECK_AND_RETURN_LOG(caps != nullptr, "caps is null");
    std::vector<AudioStandard::AudioSamplingRate> supportedSampleList = AudioStandard::
                                                                        AudioRenderer::GetSupportedSamplingRates();
    GValue list = G_VALUE_INIT;
    (void)g_value_init(&list, GST_TYPE_LIST);
    for (auto rate : supportedSampleList) {
        GValue value = G_VALUE_INIT;
        (void)g_value_init(&value, G_TYPE_INT);
        g_value_set_int(&value, rate);
        gst_value_list_append_value(&list, &value);
        g_value_unset(&value);
    }
    gst_caps_set_value(caps, "rate", &list);
    g_value_unset(&list);
}

int32_t AudioSinkSvImpl::SetVolume(float volume)
{
    MediaTrace trace("AudioSink::SetVolume");
    MEDIA_LOGD("audioRenderer SetVolume(%{public}lf) In", volume);
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION, "audioRenderer_ is nullptr");
    int32_t ret = audioRenderer_->SetVolume(volume);
    CHECK_AND_RETURN_RET_LOG(ret == AudioStandard::SUCCESS, MSERR_UNKNOWN, "audio server setvolume failed!");
    MEDIA_LOGD("audioRenderer SetVolume(%{public}lf) Out", volume);
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::SetRendererInfo(int32_t desc, int32_t rendererFlags)
{
    int32_t contentType = (static_cast<uint32_t>(desc) & 0x0000FFFF);
    int32_t streamUsage = static_cast<uint32_t>(desc) >> AudioStandard::RENDERER_STREAM_USAGE_SHIFT;
    rendererOptions_.rendererInfo.contentType = static_cast<AudioStandard::ContentType>(contentType);
    rendererOptions_.rendererInfo.streamUsage = static_cast<AudioStandard::StreamUsage>(streamUsage);
    rendererOptions_.rendererInfo.rendererFlags = rendererFlags;
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::GetVolume(float &volume)
{
    MEDIA_LOGD("GetVolume");
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION, "audioRenderer_ is nullptr");
    volume = audioRenderer_->GetVolume();
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::GetMaxVolume(float &volume)
{
    MEDIA_LOGD("GetMaxVolume");
    volume = 1.0; // audioRenderer maxVolume
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::GetMinVolume(float &volume)
{
    MEDIA_LOGD("GetMinVolume");
    volume = 0.0; // audioRenderer minVolume
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::Prepare(int32_t appUid, int32_t appPid)
{
    MediaTrace trace("AudioSink::Prepare");
    MEDIA_LOGD("audioRenderer Prepare In");
    AudioStandard::AppInfo appInfo = {};
    appInfo.appUid = appUid;
    appInfo.appPid = appPid;
    rendererOptions_.streamInfo.samplingRate = AudioStandard::SAMPLE_RATE_8000;
    rendererOptions_.streamInfo.encoding = AudioStandard::ENCODING_PCM;
    rendererOptions_.streamInfo.format = AudioStandard::SAMPLE_S16LE;
    rendererOptions_.streamInfo.channels = AudioStandard::MONO;
    audioRenderer_ = AudioStandard::AudioRenderer::Create(rendererOptions_, appInfo);
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);
    MEDIA_LOGD("audioRenderer Prepare Out");
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::Start()
{
    MediaTrace trace("AudioSink::Start");
    MEDIA_LOGD("audioRenderer Start In");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);
    (void)audioRenderer_->Start();
    MEDIA_LOGD("audioRenderer Start Out");
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::Stop()
{
    MediaTrace trace("AudioSink::Stop");
    MEDIA_LOGD("audioRenderer Stop In");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);
    (void)audioRenderer_->Stop();
    MEDIA_LOGD("audioRenderer Stop Out");
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::Pause()
{
    MediaTrace trace("AudioSink::Pause");
    MEDIA_LOGD("audioRenderer Pause In");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);
    if (audioRenderer_->GetStatus() == OHOS::AudioStandard::RENDERER_RUNNING) {
        CHECK_AND_RETURN_RET(audioRenderer_->Pause() == true, MSERR_UNKNOWN);
    }
    MEDIA_LOGD("audioRenderer Pause Out");
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::Drain()
{
    MediaTrace trace("AudioSink::Drain");
    MEDIA_LOGD("Drain");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(audioRenderer_->Drain() == true, MSERR_UNKNOWN);
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::Flush()
{
    MediaTrace trace("AudioSink::Flush");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);
    if (audioRenderer_->GetStatus() == OHOS::AudioStandard::RENDERER_RUNNING) {
        MEDIA_LOGD("Flush");
        CHECK_AND_RETURN_RET(audioRenderer_->Flush() == true, MSERR_UNKNOWN);
    }
    
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::Release()
{
    MediaTrace trace("AudioSink::Release");
    MEDIA_LOGD("audioRenderer Release In");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);
    (void)audioRenderer_->Release();
    audioRenderer_ = nullptr;
    MEDIA_LOGD("audioRenderer Release Out");
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::SetParameters(uint32_t bitsPerSample, uint32_t channels, uint32_t sampleRate)
{
    MediaTrace trace("AudioSink::SetParameters");
    (void)bitsPerSample;
    MEDIA_LOGD("SetParameters in, channels:%{public}d, sampleRate:%{public}d", channels, sampleRate);
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);

    AudioStandard::AudioRendererParams params;
    std::vector<AudioStandard::AudioSamplingRate> supportedSampleList = AudioStandard::
                                                                        AudioRenderer::GetSupportedSamplingRates();
    CHECK_AND_RETURN_RET(supportedSampleList.size() > 0, MSERR_UNKNOWN);
    bool isValidSampleRate = false;
    for (auto iter = supportedSampleList.cbegin(); iter != supportedSampleList.end(); ++iter) {
        CHECK_AND_RETURN_RET(static_cast<int32_t>(*iter) > 0, MSERR_UNKNOWN);
        uint32_t supportedSampleRate = static_cast<uint32_t>(*iter);
        if (sampleRate <= supportedSampleRate) {
            params.sampleRate = *iter;
            isValidSampleRate = true;
            break;
        }
    }
    CHECK_AND_RETURN_RET(isValidSampleRate == true, MSERR_UNSUPPORT_AUD_SAMPLE_RATE);

    std::vector<AudioStandard::AudioChannel> supportedChannelsList = AudioStandard::
                                                                     AudioRenderer::GetSupportedChannels();
    CHECK_AND_RETURN_RET(supportedChannelsList.size() > 0, MSERR_UNKNOWN);
    bool isValidChannels = false;
    for (auto iter = supportedChannelsList.cbegin(); iter != supportedChannelsList.end(); ++iter) {
        CHECK_AND_RETURN_RET(static_cast<int32_t>(*iter) > 0, MSERR_UNKNOWN);
        uint32_t supportedChannels = static_cast<uint32_t>(*iter);
        if (channels == supportedChannels) {
            params.channelCount = *iter;
            isValidChannels = true;
            break;
        }
    }
    CHECK_AND_RETURN_RET(isValidChannels == true, MSERR_UNSUPPORT_AUD_CHANNEL_NUM);

    params.sampleFormat = AudioStandard::SAMPLE_S16LE;
    params.encodingType = AudioStandard::ENCODING_PCM;
    MEDIA_LOGD("SetParameters out, channels:%{public}d, sampleRate:%{public}d", params.channelCount, params.sampleRate);
    MEDIA_LOGD("audioRenderer SetParams In");
    CHECK_AND_RETURN_RET(audioRenderer_->SetParams(params) == AudioStandard::SUCCESS, MSERR_UNKNOWN);
    MEDIA_LOGD("audioRenderer SetParams Out");
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::GetParameters(uint32_t &bitsPerSample, uint32_t &channels, uint32_t &sampleRate)
{
    (void)bitsPerSample;
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
    size_t size = 0;
    CHECK_AND_RETURN_RET(audioRenderer_->GetBufferSize(size) == AudioStandard::SUCCESS, MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(size > 0, MSERR_UNKNOWN);
    bufferSize = static_cast<uint32_t>(size);
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

bool AudioSinkSvImpl::Writeable() const
{
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, false);
    return audioRenderer_->GetStatus() == AudioStandard::RENDERER_RUNNING;
}

int32_t AudioSinkSvImpl::Write(uint8_t *buffer, size_t size)
{
    MediaTrace trace("AudioSink::Write");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(buffer != nullptr, MSERR_INVALID_OPERATION);
    size_t bytesWritten = 0;
    int32_t bytesSingle = 0;
    while (bytesWritten < size) {
        bytesSingle = audioRenderer_->Write(buffer + bytesWritten, size - bytesWritten);
        CHECK_AND_RETURN_RET(bytesSingle > 0, MSERR_UNKNOWN);
        bytesWritten += static_cast<size_t>(bytesSingle);
        CHECK_AND_RETURN_RET(bytesWritten >= static_cast<size_t>(bytesSingle), MSERR_UNKNOWN);
    }
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::GetAudioTime(uint64_t &time)
{
    MediaTrace trace("AudioSink::GetAudioTime");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);
    AudioStandard::Timestamp timeStamp;
    bool ret = audioRenderer_->GetAudioTime(timeStamp, AudioStandard::Timestamp::Timestampbase::MONOTONIC);
    CHECK_AND_RETURN_RET(ret == true, MSERR_UNKNOWN);
    time = static_cast<uint64_t>(timeStamp.time.tv_nsec);
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::GetLatency(uint64_t &latency) const
{
    MediaTrace trace("AudioSink::GetLatency");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(audioRenderer_->GetLatency(latency) == AudioStandard::SUCCESS, MSERR_UNKNOWN);
    return MSERR_OK;
}

void AudioSinkSvImpl::SetAudioSinkInterruptCb(void (*interruptCb)(GstBaseSink *, guint, guint, guint))
{
    CHECK_AND_RETURN(audioRendererMediaCallback_ != nullptr);
    audioRendererMediaCallback_->SaveCallback(interruptCb);
    audioRenderer_->SetRendererCallback(audioRendererMediaCallback_);
}

void AudioSinkSvImpl::SetAudioInterruptMode(int32_t interruptMode)
{
    CHECK_AND_RETURN(audioRendererMediaCallback_ != nullptr);
    audioRenderer_->SetInterruptMode(static_cast<AudioStandard::InterruptMode>(interruptMode));
}
} // namespace Media
} // namespace OHOS
