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

#include "audio_renderer_sink.h"

#include <cstdio>
#include <map>

#include "media_log.h"

using namespace std;

namespace OHOS {
namespace Media {
const int32_t FIVE_MSEC = 5000;
const int32_t MAX_AUDIO_ADAPTER_NUM = 2;

const char *g_audioOutTestFilePath = "/data/local/tmp/audioout_test.pcm";

AudioRendererSink::AudioRendererSink()
{
    audioManager_ = nullptr;
    audioAdapter_ = nullptr;
    audioRender_ = nullptr;
    audioFormat_ = AUDIO_FORMAT_PCM_16_BIT;
    sampleRate = 0;
    channelCount = 0;
    pfd = nullptr;
    enableDebug = false;
}

bool AudioRendererSink::ParseCaps(GstCaps &Caps, GstAudioRingBufferSpec &Spec)
{
    Spec.latency_time = GST_SECOND;

    if (!gst_audio_ring_buffer_parse_caps (&Spec, &Caps)) {
        MEDIA_ERR_LOG("parse Caps failed, not accepting!");
        return FALSE;
    }

    return TRUE;
}

bool AudioRendererSink::Open()
{
    int32_t ret = -1;
    int32_t size = -1;

    audioManager_ = GetAudioManagerFuncs();
    if (audioManager_ == nullptr) {
        MEDIA_ERR_LOG("%s: GetAudioManagerFuncs return null!", __FUNCTION__);
        return FALSE;
    }

    struct AudioAdapterDescriptor *Descs = nullptr;
    audioManager_->GetAllAdapters(audioManager_, &Descs, &size);
    if (size > MAX_AUDIO_ADAPTER_NUM) {
        MEDIA_ERR_LOG("%s: GetAllAdapters size exceeded max!", __FUNCTION__);
        return FALSE;
    }

    for (int index = 0; index < size; index++) {
        struct AudioAdapterDescriptor *Desc = &Descs[index];
        for (int port = 0; (Desc != nullptr && port < static_cast<int>(Desc->portNum)); port++) {
            if (Desc->ports[port].dir == PORT_OUT &&
                (audioManager_->LoadAdapter(audioManager_, Desc, &(audioAdapter_))) == 0) {
                ret = (audioAdapter_)->InitAllPorts(audioAdapter_);
                MEDIA_INFO_LOG("%s: Init AudioPorts returned:%d", __FUNCTION__, ret);
                if (ret == 0) {
                    break;
                }
            }
        }
    }

    if (ret != 0) {
        MEDIA_ERR_LOG("load audiodevice failed!");
        return FALSE;
    }

    MEDIA_INFO_LOG("Opened audio renderer sink");
    return TRUE;
}

static map<GstAudioFormat, AudioFormat> g_formatMap = {
    { GST_AUDIO_FORMAT_U8, AUDIO_FORMAT_PCM_8_BIT },
    { GST_AUDIO_FORMAT_S8, AUDIO_FORMAT_PCM_8_BIT },
    { GST_AUDIO_FORMAT_U16LE, AUDIO_FORMAT_PCM_16_BIT },
    { GST_AUDIO_FORMAT_U16BE, AUDIO_FORMAT_PCM_16_BIT },
    { GST_AUDIO_FORMAT_S16LE, AUDIO_FORMAT_PCM_16_BIT },
    { GST_AUDIO_FORMAT_S16BE, AUDIO_FORMAT_PCM_16_BIT },
    { GST_AUDIO_FORMAT_U24LE, AUDIO_FORMAT_PCM_24_BIT },
    { GST_AUDIO_FORMAT_U24BE, AUDIO_FORMAT_PCM_24_BIT },
    { GST_AUDIO_FORMAT_S24LE, AUDIO_FORMAT_PCM_24_BIT },
    { GST_AUDIO_FORMAT_S24BE, AUDIO_FORMAT_PCM_24_BIT },
    { GST_AUDIO_FORMAT_U32LE, AUDIO_FORMAT_PCM_32_BIT },
    { GST_AUDIO_FORMAT_U32BE, AUDIO_FORMAT_PCM_32_BIT },
    { GST_AUDIO_FORMAT_S32LE, AUDIO_FORMAT_PCM_32_BIT },
    { GST_AUDIO_FORMAT_S32BE, AUDIO_FORMAT_PCM_32_BIT },
    { GST_AUDIO_FORMAT_F32LE, AUDIO_FORMAT_PCM_32_BIT },
    { GST_AUDIO_FORMAT_F32BE, AUDIO_FORMAT_PCM_32_BIT }
};

bool AudioRendererSink::ParseSpec(GstAudioRingBufferSpec &Spec)
{
    MEDIA_INFO_LOG("audioinfo type:%d, format:%d", Spec.type, GST_AUDIO_INFO_FORMAT(&Spec.info));
    if (Spec.type == GST_AUDIO_RING_BUFFER_FORMAT_TYPE_RAW) {
        auto iter = g_formatMap.find(GST_AUDIO_INFO_FORMAT(&Spec.info));
        if (iter == g_formatMap.end()) {
            MEDIA_INFO_LOG("audioinfo type");
            audioFormat_ = AUDIO_FORMAT_PCM_16_BIT;
        } else {
            MEDIA_INFO_LOG("audioinfo type = %d", iter->second);
            audioFormat_ = iter->second;
        }
    } else {
        MEDIA_INFO_LOG("unsupported Spec type %d!", static_cast<gint>(Spec.type));
        return FALSE;
    }

    sampleRate = GST_AUDIO_INFO_RATE(&Spec.info);
    channelCount = GST_AUDIO_INFO_CHANNELS(&Spec.info);

    MEDIA_INFO_LOG("Audioinfo: sample rate: %d, channel count: %d", sampleRate, channelCount);

    return TRUE;
}

bool AudioRendererSink::Prepare(GstCaps &AudioCaps)
{
    int32_t ret;
    GstAudioRingBufferSpec Spec = { 0 };

    if (!Open()) {
        MEDIA_ERR_LOG("%s: Error open failed!", __FUNCTION__);
        return FALSE;
    }

    if (!ParseCaps(AudioCaps, Spec)) {
        MEDIA_ERR_LOG("%s: Error parsing Caps!", __FUNCTION__);
        Close();
        return FALSE;
    }

    if (!ParseSpec(Spec)) {
        MEDIA_ERR_LOG("%s: Error parsing Spec!", __FUNCTION__);
        Close();
        return FALSE;
    }

    struct AudioSampleAttributes param = {};
    param.sampleRate = sampleRate;
    param.format = AUDIO_FORMAT_PCM_16_BIT;
    param.channelCount = channelCount;
    param.interleaved = false;
    MEDIA_INFO_LOG("create renderer sampleRate:%d, format:%d, channelCount:%d",
        param.sampleRate, param.format, param.channelCount);

    struct AudioDeviceDescriptor deviceDesc = {0, PIN_OUT_SPEAKER, nullptr};

    ret = audioAdapter_->CreateRender(audioAdapter_, &deviceDesc, &param, &(audioRender_));
    if (ret != 0 || audioRender_ == nullptr) {
        MEDIA_ERR_LOG("AudioDeviceCreateRender failed!");
        Close();
        return FALSE;
    }

    ret = audioRender_->control.Start((AudioHandle)audioRender_);
    if (ret != 0) {
        MEDIA_ERR_LOG("audiorenderer control start failed!");
        Close();
        return FALSE;
    }

    if (enableDebug) {
        pfd = fopen(g_audioOutTestFilePath, "wb+");
        if (pfd == nullptr) {
            MEDIA_ERR_LOG("Error opening pcm test file!");
        }
    }

    return TRUE;
}

bool AudioRendererSink::Stop()
{
    MEDIA_INFO_LOG("AudioRendererSinkStop.");
    Reset();
    if (audioRender_ != nullptr) {
        audioRender_->control.Stop((AudioHandle)audioRender_);
    }
    return TRUE;
}

void AudioRendererSink::Close()
{
    MEDIA_INFO_LOG("close audio renderer sink.");

    if (audioAdapter_ != nullptr && audioRender_ != nullptr) {
        audioAdapter_->DestroyRender(audioAdapter_, audioRender_);
    }
    audioRender_ = nullptr;

    if ((audioManager_ != nullptr) && (audioAdapter_ != nullptr)) {
        audioManager_->UnloadAdapter(audioManager_, audioAdapter_);
    }
    audioAdapter_ = nullptr;
    audioManager_ = nullptr;

    if (enableDebug) {
        if (pfd) {
            fclose(pfd);
            pfd = nullptr;
        }
    }
}

int32_t AudioRendererSink::Write(gpointer data, guint len)
{
    int32_t ret;
    uint64_t writeLen = 0;

    MEDIA_INFO_LOG ("Received audio samples buffer of %u bytes", len);

    if ((data == nullptr) || (len == 0)) {
        MEDIA_ERR_LOG("Error outputting to audio renderer, parameter error!");
        return 0;
    }

    if (enableDebug) {
        size_t writeResult = fwrite((void*)data, 1, len, pfd);
        if (writeResult != len) {
            MEDIA_ERR_LOG("Failed to write the file.");
            return len;
        }

        MEDIA_INFO_LOG("The file is written successfully.");
    }

    if (audioRender_ == nullptr) {
        MEDIA_ERR_LOG("audioRender is NULL!");
        return 0;
    }

    for (; ;) {
        ret = audioRender_->RenderFrame(audioRender_, static_cast<uint8_t *>(data),
            static_cast<uint64_t>(len), &writeLen);

        if (writeLen > len) {
            MEDIA_ERR_LOG("Error outputting to audio renderer, write failed!");
            return len;
        }

        if (writeLen == 0) {
            g_usleep(FIVE_MSEC);
        } else {
            MEDIA_ERR_LOG("outputting to audio renderer written %llu bytes, %u length, %d ret", writeLen, len, ret);
            break;
        }
    }

    return writeLen;
}

void AudioRendererSink::Reset()
{
    if (audioRender_ != nullptr) {
        audioRender_->control.Flush((AudioHandle)audioRender_);
    }
}

int32_t AudioRendererSink::SetVolume(float Volume)
{
    int32_t ret;

    if (audioRender_ == nullptr) {
        MEDIA_ERR_LOG("audioRender is NULL!");
        return -1;
    }

    ret = audioRender_->volume.SetVolume(reinterpret_cast<AudioHandle>(audioRender_), Volume);
    MEDIA_INFO_LOG("Set volume inside AudioRendererSink:SetVolume volume=%0.2f ret=%d", Volume, ret);
    return ret;
}
} // namespace Media
} // namespace OHOS
