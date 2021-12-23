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

#include "av_common.h"
#include <map>
#include <string>
#include "media_errors.h"

namespace OHOS {
namespace Media {
const std::map<std::string, CodecMimeType> STRING_TO_CODECMIME = {
    {"video/h263", CODEC_MIMIE_TYPE_VIDEO_H263},
    {"video/avc", CODEC_MIMIE_TYPE_VIDEO_AVC},
    {"video/mpeg2", CODEC_MIMIE_TYPE_VIDEO_MPEG2},
    {"video/hevc", CODEC_MIMIE_TYPE_VIDEO_HEVC},
    {"video/mp4v-es", CODEC_MIMIE_TYPE_VIDEO_MPEG4},
    {"video/x-vnd.on2.vp8", CODEC_MIMIE_TYPE_VIDEO_VP8},
    {"video/x-vnd.on2.vp9", CODEC_MIMIE_TYPE_VIDEO_VP9},
    {"audio/3gpp", CODEC_MIMIE_TYPE_AUDIO_AMR_NB},
    {"audio/amr-wb", CODEC_MIMIE_TYPE_AUDIO_AMR_WB},
    {"audio/mpeg", CODEC_MIMIE_TYPE_AUDIO_MPEG},
    {"audio/mp4a-latm", CODEC_MIMIE_TYPE_AUDIO_AAC},
    {"audio/vorbis", CODEC_MIMIE_TYPE_AUDIO_VORBIS},
    {"audio/opus", CODEC_MIMIE_TYPE_AUDIO_OPUS},
    {"audio/flac", CODEC_MIMIE_TYPE_AUDIO_FLAC},
    {"audio/raw", CODEC_MIMIE_TYPE_AUDIO_RAW},
};

const std::map<std::string, ContainerFormatType> STRING_TO_CFT = {
    {"mp4", CFT_MPEG_4},
    {"mpeg-ts", CFT_MPEG_TS},
    {"mkv", CFT_MKV},
    {"webm", CFT_WEBM},
    {"m4a", CFT_MPEG_4A},
    {"ogg", CFT_OGG},
    {"wav", CFT_WAV},
    {"aac", CFT_AAC},
    {"flac", CFT_FLAC},
};

const std::map<ContainerFormatType, OutputFormatType> CTF_TO_OUTFORMAT = {
    {CFT_MPEG_4, FORMAT_MPEG_4},
    {CFT_MPEG_TS, FORMAT_BUTT},
    {CFT_MKV, FORMAT_BUTT},
    {CFT_WEBM, FORMAT_BUTT},
    {CFT_MPEG_4A, FORMAT_M4A},
    {CFT_OGG, FORMAT_BUTT},
    {CFT_WAV, FORMAT_BUTT},
    {CFT_AAC, FORMAT_BUTT},
    {CFT_FLAC, FORMAT_BUTT},
};

const std::map<CodecMimeType, AudioCodecFormat> CMT_TO_AUDIO_CODEC = {
    {CODEC_MIMIE_TYPE_DEFAULT, AUDIO_DEFAULT},
    {CODEC_MIMIE_TYPE_AUDIO_AMR_NB, AUDIO_CODEC_FORMAT_BUTT},
    {CODEC_MIMIE_TYPE_AUDIO_AMR_WB, AUDIO_CODEC_FORMAT_BUTT},
    {CODEC_MIMIE_TYPE_AUDIO_MPEG, AUDIO_CODEC_FORMAT_BUTT},
    {CODEC_MIMIE_TYPE_AUDIO_AAC, AAC_LC},
    {CODEC_MIMIE_TYPE_AUDIO_VORBIS, AUDIO_CODEC_FORMAT_BUTT},
    {CODEC_MIMIE_TYPE_AUDIO_OPUS, AUDIO_CODEC_FORMAT_BUTT},
    {CODEC_MIMIE_TYPE_AUDIO_FLAC, AUDIO_CODEC_FORMAT_BUTT},
    {CODEC_MIMIE_TYPE_AUDIO_RAW, AUDIO_CODEC_FORMAT_BUTT},
};

const std::map<CodecMimeType, VideoCodecFormat> CMT_TO_VIDEO_CODEC = {
    {CODEC_MIMIE_TYPE_VIDEO_H263, VIDEO_CODEC_FORMAT_BUTT},
    {CODEC_MIMIE_TYPE_VIDEO_AVC, H264},
    {CODEC_MIMIE_TYPE_VIDEO_MPEG2, VIDEO_CODEC_FORMAT_BUTT},
    {CODEC_MIMIE_TYPE_VIDEO_HEVC, HEVC},
    {CODEC_MIMIE_TYPE_VIDEO_MPEG4, MPEG4},
    {CODEC_MIMIE_TYPE_VIDEO_VP8, VIDEO_CODEC_FORMAT_BUTT},
    {CODEC_MIMIE_TYPE_VIDEO_VP9, VIDEO_CODEC_FORMAT_BUTT},
};

int32_t MapStringToCodecMime(const std::string &mime, CodecMimeType &name)
{
    if (STRING_TO_CODECMIME.count(mime) != 0) {
        name =  STRING_TO_CODECMIME.at(mime);
        return MSERR_OK;
    }
    return MSERR_INVALID_VAL;
}

int32_t MapStringToContainerFormat(const std::string &format, ContainerFormatType &cft)
{
    if (STRING_TO_CFT.count(format) != 0) {
        cft =  STRING_TO_CFT.at(format);
        return MSERR_OK;
    }
    return MSERR_INVALID_VAL;
}

int32_t MapContainerFormatToOutputFormat(const ContainerFormatType &cft, OutputFormatType &opf)
{
    if (CTF_TO_OUTFORMAT.count(cft) != 0) {
        opf =  CTF_TO_OUTFORMAT.at(cft);
        return MSERR_OK;
    }
    return MSERR_INVALID_VAL;
}

int32_t MapCodecMimeToAudioCodec(const CodecMimeType &mime, AudioCodecFormat &audio)
{
    if (CMT_TO_AUDIO_CODEC.count(mime) != 0) {
        audio =  CMT_TO_AUDIO_CODEC.at(mime);
        return MSERR_OK;
    }
    return MSERR_INVALID_VAL;
}

int32_t MapCodecMimeToVideoCodec(const CodecMimeType &mime, VideoCodecFormat &video)
{
    if (CMT_TO_VIDEO_CODEC.count(mime) != 0) {
        video =  CMT_TO_VIDEO_CODEC.at(mime);
        return MSERR_OK;
    }
    return MSERR_INVALID_VAL;
}
} // namespace OHOS
} // namespace Media
