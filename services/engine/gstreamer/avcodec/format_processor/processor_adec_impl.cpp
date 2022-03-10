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

#include "processor_adec_impl.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "ProcessorAdecImpl"};
    constexpr uint32_t DEFAULT_BUFFER_SIZE = 30000;
    static const GstAudioChannelPosition CHANNEL_POSITION[6][6] = {
    {
        GST_AUDIO_CHANNEL_POSITION_MONO
    },
    {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT
    },
    {
        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT
    },
    {
        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_REAR_CENTER
    },
    {
        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
        GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT
    },
    {
        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
        GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_LFE1
    },
};
}

namespace OHOS {
namespace Media {
ProcessorAdecImpl::ProcessorAdecImpl()
{
}

ProcessorAdecImpl::~ProcessorAdecImpl()
{
}

int32_t ProcessorAdecImpl::ProcessMandatory(const Format &format)
{
    CHECK_AND_RETURN_RET(format.GetIntValue("channel_count", channels_) == true, MSERR_INVALID_VAL);
    CHECK_AND_RETURN_RET(format.GetIntValue("sample_rate", sampleRate_) == true, MSERR_INVALID_VAL);
    CHECK_AND_RETURN_RET(format.GetIntValue("audio_raw_format", AudioSampleFormat_) == true, MSERR_INVALID_VAL);
    MEDIA_LOGD("channels:%{public}d, sampleRate:%{public}d, pcm:%{public}d", channels_, sampleRate_, AudioSampleFormat_);

    gstRawFormat_ = RawAudioFormatToGst(static_cast<AudioStandard::AudioSampleFormat>(AudioSampleFormat_));

    return MSERR_OK;
}

int32_t ProcessorAdecImpl::ProcessOptional(const Format &format)
{
    return MSERR_OK;
}

std::shared_ptr<ProcessorConfig> ProcessorAdecImpl::GetInputPortConfig()
{
    CHECK_AND_RETURN_RET(channels_ > 0 && sampleRate_ > 0, nullptr);

    guint64 channelMask = 0;
    if (!gst_audio_channel_positions_to_mask(CHANNEL_POSITION[channels_ - 1], channels_, FALSE, &channelMask)) {
        MEDIA_LOGE("Invalid channel positions");
        return nullptr;
    }

    GstCaps *caps = nullptr;
    switch (codecName_) {
        case CODEC_MIMIE_TYPE_AUDIO_VORBIS:
            caps = gst_caps_new_simple("audio/x-vorbis",
                "rate", G_TYPE_INT, sampleRate_, "channels", G_TYPE_INT, channels_, nullptr);
            break;
        case CODEC_MIMIE_TYPE_AUDIO_MPEG:
            caps = gst_caps_new_simple("audio/mpeg",
                "rate", G_TYPE_INT, sampleRate_, "channels", G_TYPE_INT, channels_,
                "channel-mask", GST_TYPE_BITMASK, channelMask, "mpegversion", G_TYPE_INT, 1,
                "layer", G_TYPE_INT, 3, nullptr);
            break;
        case CODEC_MIMIE_TYPE_AUDIO_AAC:
            caps = gst_caps_new_simple("audio/mpeg",
                "rate", G_TYPE_INT, sampleRate_, "channels", G_TYPE_INT, channels_,
                "mpegversion", G_TYPE_INT, 4, "stream-format", G_TYPE_STRING, "adts",
                "base-profile", G_TYPE_STRING, "lc", nullptr);
            break;
        case CODEC_MIMIE_TYPE_AUDIO_FLAC:
            caps = gst_caps_new_simple("audio/x-flac",
                "rate", G_TYPE_INT, sampleRate_, "channels", G_TYPE_INT, channels_,
                "framed", G_TYPE_BOOLEAN, TRUE, nullptr);
            break;
        default:
            break;
    }
    CHECK_AND_RETURN_RET_LOG(caps != nullptr, nullptr, "Unsupported format");

    auto config = std::make_shared<ProcessorConfig>(caps, false);
    if (config == nullptr) {
        MEDIA_LOGE("No memory");
        gst_caps_unref(caps);
        return nullptr;
    }

    config->needParser_ = (codecName_ == CODEC_MIMIE_TYPE_AUDIO_FLAC) ? true : false;
    config->needCodecData_ = (codecName_ == CODEC_MIMIE_TYPE_AUDIO_VORBIS) ? true : false;
    config->bufferSize_ = DEFAULT_BUFFER_SIZE;

    return config;
}

std::shared_ptr<ProcessorConfig> ProcessorAdecImpl::GetOutputPortConfig()
{
    CHECK_AND_RETURN_RET(channels_ > 0 && sampleRate_ > 0, nullptr);

    GstCaps *caps = gst_caps_new_simple("audio/x-raw",
        "rate", G_TYPE_INT, sampleRate_,
        "channels", G_TYPE_INT, channels_,
        "format", G_TYPE_STRING, gstRawFormat_.c_str(),
        "layout", G_TYPE_STRING, "interleaved", nullptr);
    CHECK_AND_RETURN_RET_LOG(caps != nullptr, nullptr, "No memory");

    auto config = std::make_shared<ProcessorConfig>(caps, false);
    if (config == nullptr) {
        MEDIA_LOGE("No memory");
        gst_caps_unref(caps);
        return nullptr;
    }

    config->bufferSize_ = DEFAULT_BUFFER_SIZE;

    return config;
}
} // namespace Media
} // namespace OHOS
