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
const std::map<VideoPixelFormat, std::string> PIEXEL_TO_STRING = {
    {YUVI420, "I420"},
    {NV12, "NV12"},
    {NV21, "NV21"},
};

const std::map<AudioRawFormat, std::string> PCM_TO_STRING = {
    {AUDIO_PCM_S8, "S8"},
    {AUDIO_PCM_8, "U8"},
    {AUDIO_PCM_S16_BE, "S16BE"},
    {AUDIO_PCM_S16_LE, "S16LE"},
    {AUDIO_PCM_16_BE, "U16BE"},
    {AUDIO_PCM_16_LE, "U16LE"},
    {AUDIO_PCM_S24_BE, "S24BE"},
    {AUDIO_PCM_S24_LE, "S24LE"},
    {AUDIO_PCM_24_BE, "U24BE"},
    {AUDIO_PCM_24_LE, "U24LE"},
    {AUDIO_PCM_S32_BE, "S32BE"},
    {AUDIO_PCM_S32_LE, "S32LE"},
    {AUDIO_PCM_32_BE, "U32BE"},
    {AUDIO_PCM_32_LE, "U32LE"},
    {AUDIO_PCM_F32_BE, "F32BE"},
    {AUDIO_PCM_F32_LE, "F32LE"},
};

const std::map<std::string, CodecMimeType> MIME_TO_CODEC_NAME = {
    {"video/3gpp", CODEC_MIMIE_TYPE_VIDEO_H263},
    {"video/avc", CODEC_MIMIE_TYPE_VIDEO_AVC},
    {"video/hevc", CODEC_MIMIE_TYPE_VIDEO_HEVC},
    {"video/mpeg2", CODEC_MIMIE_TYPE_VIDEO_MPEG2},
    {"video/mp4v-es", CODEC_MIMIE_TYPE_VIDEO_MPEG4},
    {"audio/vorbis", CODEC_MIMIE_TYPE_AUDIO_VORBIS},
    {"audio/mpeg", CODEC_MIMIE_TYPE_AUDIO_MPEG},
    {"audio/mp4a-latm", CODEC_MIMIE_TYPE_AUDIO_AAC},
    {"audio/flac", CODEC_MIMIE_TYPE_AUDIO_FLAC},
};

std::string PixelFormatToGst(VideoPixelFormat pixel)
{
    if (PIEXEL_TO_STRING.count(pixel) != 0) {
        return PIEXEL_TO_STRING.at(pixel);
    }
    return "Invalid";
}

std::string RawAudioFormatToGst(AudioRawFormat format)
{
    if (PCM_TO_STRING.count(format) != 0) {
        return PCM_TO_STRING.at(format);
    }
    return "Invalid";
}

int32_t MapCodecMime(const std::string &mime, CodecMimeType &name)
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
        case NV21:
            size = width * height * 3 / 2;
            size = (size + alignment - 1) & ~(alignment - 1);
            break;
        default:
            break;
    }
    return size;
}

uint32_t EncodedBufSize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0) {
        return 0;
    }

    const uint32_t compressRatio = 15;
    const uint32_t maxSize = 3150000; // 3MB

    if ((UINT32_MAX / width) <= (height / compressRatio)) {
        return maxSize;
    }

    return height / compressRatio * width;
}
} // namespace Media
} // namespace OHOS