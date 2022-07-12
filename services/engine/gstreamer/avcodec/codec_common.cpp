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

#include "codec_common.h"
#include "media_errors.h"

namespace OHOS {
namespace Media {
const std::map<VideoPixelFormat, std::string> PIXEL_TO_STRING = {
    {YUVI420, "I420"},
    {NV12, "NV12"},
    {NV21, "NV21"},
    {SURFACE_FORMAT, "NV21"},
    {RGBA, "RGBA"},
};

const std::map<AudioStandard::AudioSampleFormat, std::string> PCM_TO_STRING = {
    {AudioStandard::SAMPLE_U8, "U8"},
    {AudioStandard::SAMPLE_S16LE, "S16LE"},
    {AudioStandard::SAMPLE_S24LE, "S24LE"},
    {AudioStandard::SAMPLE_S32LE, "S32LE"},
};

const std::map<MPEG4Profile, std::string> MPEG4_PROFILE_TO_STRING = {
    {MPEG4_PROFILE_ADVANCED_CODING, "advanced-coding-efficiency"},
    {MPEG4_PROFILE_ADVANCED_CORE, "advanced-core"},
    {MPEG4_PROFILE_ADVANCED_REAL_TIME, "advanced-real-time"},
    {MPEG4_PROFILE_ADVANCED_SCALABLE, "advanced-scalable-texture"},
    {MPEG4_PROFILE_ADVANCED_SIMPLE, "advanced-simple"},
    {MPEG4_PROFILE_BASIC_ANIMATED, "basic-animated-texture"},
    {MPEG4_PROFILE_CORE, "core"},
    {MPEG4_PROFILE_CORE_SCALABLE, "core-scalable"},
    {MPEG4_PROFILE_HYBRID, "hybrid"},
    {MPEG4_PROFILE_MAIN, "main"},
    {MPEG4_PROFILE_NBIT, "n-bit"},
    {MPEG4_PROFILE_SCALABLE_TEXTURE, "scalable"},
    {MPEG4_PROFILE_SIMPLE, "simple"},
    {MPEG4_PROFILE_SIMPLE_FBA, "simple-fba"},
    {MPEG4_PROFILE_SIMPLE_FACE, "simple-face"},
    {MPEG4_PROFILE_SIMPLE_SCALABLE, "simple-scalable"},
};

const std::map<AVCProfile, std::string> AVC_PROFILE_TO_STRING = {
    {AVC_PROFILE_BASELINE, "baseline"},
    {AVC_PROFILE_CONSTRAINED_BASELINE, "constrained-baseline"},
    {AVC_PROFILE_CONSTRAINED_HIGH, "constrained-high"},
    {AVC_PROFILE_EXTENDED, "extended"},
    {AVC_PROFILE_HIGH, "high"},
    {AVC_PROFILE_HIGH_10, "high-10"},
    {AVC_PROFILE_HIGH_422, "high-4:2:2"},
    {AVC_PROFILE_HIGH_444, "high-4:4:4"},
    {AVC_PROFILE_MAIN, "main"},
};

const std::map<HEVCProfile, std::string> HEVC_PROFILE_TO_STRING = {
    {HEVC_PROFILE_MAIN, "main"},
    {HEVC_PROFILE_MAIN_10, "main-10"},
    {HEVC_PROFILE_MAIN_STILL, "main-still-picture"},
};

const std::map<std::string_view, InnerCodecMimeType> MIME_TO_CODEC_NAME = {
    {CodecMimeType::VIDEO_H263, CODEC_MIME_TYPE_VIDEO_H263},
    {CodecMimeType::VIDEO_AVC, CODEC_MIME_TYPE_VIDEO_AVC},
    {CodecMimeType::VIDEO_HEVC, CODEC_MIME_TYPE_VIDEO_HEVC},
    {CodecMimeType::VIDEO_MPEG2, CODEC_MIME_TYPE_VIDEO_MPEG2},
    {CodecMimeType::VIDEO_MPEG4, CODEC_MIME_TYPE_VIDEO_MPEG4},
    {CodecMimeType::AUDIO_VORBIS, CODEC_MIME_TYPE_AUDIO_VORBIS},
    {CodecMimeType::AUDIO_MPEG, CODEC_MIME_TYPE_AUDIO_MPEG},
    {CodecMimeType::AUDIO_AAC, CODEC_MIME_TYPE_AUDIO_AAC},
    {CodecMimeType::AUDIO_FLAC, CODEC_MIME_TYPE_AUDIO_FLAC},
    {CodecMimeType::AUDIO_OPUS, CODEC_MIME_TYPE_AUDIO_OPUS},
};

std::string PixelFormatToGst(VideoPixelFormat pixel)
{
    if (PIXEL_TO_STRING.count(pixel) != 0) {
        return PIXEL_TO_STRING.at(pixel);
    }
    return "Invalid";
}

std::string MPEG4ProfileToGst(MPEG4Profile profile)
{
    if (MPEG4_PROFILE_TO_STRING.count(profile) != 0) {
        return MPEG4_PROFILE_TO_STRING.at(profile);
    }
    return "Invalid";
}

std::string AVCProfileToGst(AVCProfile profile)
{
    if (AVC_PROFILE_TO_STRING.count(profile) != 0) {
        return AVC_PROFILE_TO_STRING.at(profile);
    }
    return "Invalid";
}

std::string HEVCProfileToGst(HEVCProfile profile)
{
    if (HEVC_PROFILE_TO_STRING.count(profile) != 0) {
        return HEVC_PROFILE_TO_STRING.at(profile);
    }
    return "Invalid";
}

std::string RawAudioFormatToGst(AudioStandard::AudioSampleFormat format)
{
    if (PCM_TO_STRING.count(format) != 0) {
        return PCM_TO_STRING.at(format);
    }
    return "Invalid";
}

int32_t MapCodecMime(const std::string &mime, InnerCodecMimeType &name)
{
    if (MIME_TO_CODEC_NAME.count(mime) != 0) {
        name =  MIME_TO_CODEC_NAME.at(mime);
        return MSERR_OK;
    }
    return MSERR_INVALID_VAL;
}

int32_t CapsToFormat(GstCaps *caps, Format &format)
{
    GstStructure *structure = gst_caps_get_structure(caps, 0);
    if (structure == nullptr) {
        return MSERR_UNKNOWN;
    }
    auto mediaType = gst_structure_get_name(structure);
    bool isVideo = g_str_has_prefix(mediaType, "video/");

    gint ret = 0;
    if (isVideo) {
        (void)gst_structure_get(structure, "width", G_TYPE_INT, &ret, nullptr);
        (void)format.PutIntValue("width", ret);
        (void)gst_structure_get(structure, "height", G_TYPE_INT, &ret, nullptr);
        (void)format.PutIntValue("height", ret);
    } else {
        (void)gst_structure_get(structure, "rate", G_TYPE_INT, &ret, nullptr);
        (void)format.PutIntValue("sample_rate", ret);
        (void)gst_structure_get(structure, "channels", G_TYPE_INT, &ret, nullptr);
        (void)format.PutIntValue("channel_count", ret);
    }
    return MSERR_OK;
}

uint32_t PixelBufferSize(VideoPixelFormat pixel, uint32_t width, uint32_t height, uint32_t alignment)
{
    uint32_t size = 0;
    if (width == 0 || height == 0 || alignment == 0) {
        return size;
    }

    switch (pixel) {
        case YUVI420:
            // fall-through
        case NV12:
            // fall-through
        case SURFACE_FORMAT:
            // fall-through
        case NV21:
            size = width * height * 3 / 2;
            break;
        case RGBA:
            size = width * height * 4;
            break;
        default:
            break;
    }

    if (size > 0) {
        size = (size + alignment - 1) & ~(alignment - 1);
    }

    return size;
}
} // namespace Media
} // namespace OHOS