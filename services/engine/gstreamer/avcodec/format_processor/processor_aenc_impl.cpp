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

#include "processor_aenc_impl.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "ProcessorAencImpl"};
}

namespace OHOS {
namespace Media {
ProcessorAencImpl::ProcessorAencImpl()
{
}

ProcessorAencImpl::~ProcessorAencImpl()
{
}

int32_t ProcessorAencImpl::ProcessMandatory(const Format &format)
{
    CHECK_AND_RETURN_RET(format.GetIntValue("channel_count", channels_) == true, MSERR_INVALID_VAL);
    CHECK_AND_RETURN_RET(format.GetIntValue("sample_rate", sampleRate_) == true, MSERR_INVALID_VAL);
    int32_t audioRawFormat = 0;
    CHECK_AND_RETURN_RET(format.GetIntValue("audio_raw_format", audioRawFormat) == true, MSERR_INVALID_VAL);

    MEDIA_LOGD("channels:%{public}d, sampleRate:%{public}d, pcm:%{public}d", channels_, sampleRate_, audioRawFormat);

    AudioRawFormat rawFormat = AUDIO_PCM_S8;
    CHECK_AND_RETURN_RET(MapPCMFormat(audioRawFormat, rawFormat) == MSERR_OK, MSERR_INVALID_VAL);
    audioRawFormat_ = PCMFormatToString(rawFormat);

    return MSERR_OK;
}

int32_t ProcessorAencImpl::ProcessOptional(const Format &format)
{
    (void)format.GetIntValue("profile", profile_);
    return MSERR_OK;
}

std::shared_ptr<ProcessorConfig> ProcessorAencImpl::GetInputPortConfig()
{
    guint64 channelMask = 0;
    if (!gst_audio_channel_positions_to_mask(CHANNEL_POSITION[channels_], channels_, FALSE, &channelMask)) {
        MEDIA_LOGE("Invalid channel positions");
        return nullptr;
    }

    GstCaps *caps = gst_caps_new_simple("audio/x-raw",
        "rate", G_TYPE_INT, sampleRate_,
        "channels", G_TYPE_INT, channels_,
        "format", G_TYPE_STRING, audioRawFormat_.c_str(),
        "channel-mask", GST_TYPE_BITMASK, channelMask,
        "layout", G_TYPE_STRING, "interleaved", nullptr);
    CHECK_AND_RETURN_RET_LOG(caps != nullptr, nullptr, "No memory");

    auto config = std::make_shared<ProcessorConfig>(caps);
    if (config == nullptr) {
        MEDIA_LOGE("No memory");
        gst_caps_unref(caps);
        return nullptr;
    }
    return config;
}

std::shared_ptr<ProcessorConfig> ProcessorAencImpl::GetOutputPortConfig()
{
    GstCaps *caps = nullptr;
    switch (codecName_) {
        case CODEC_MIMIE_TYPE_AUDIO_AAC:
            caps = gst_caps_new_simple("audio/mpeg",
                "rate", G_TYPE_INT, sampleRate_,
                "channels", G_TYPE_INT, channels_,
                "mpegversion", G_TYPE_INT, 4,
                "stream-format", G_TYPE_STRING, "raw",
                "base-profile", G_TYPE_STRING, "lc", nullptr);
            break;
        default:
            break;
    }
    CHECK_AND_RETURN_RET_LOG(caps != nullptr, nullptr, "Unsupported format");

    auto config = std::make_shared<ProcessorConfig>(caps);
    if (config == nullptr) {
        MEDIA_LOGE("No memory");
        gst_caps_unref(caps);
        return nullptr;
    }
    return config;
}
} // Media
} // OHOS
