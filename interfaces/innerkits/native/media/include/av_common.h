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
#ifndef AV_COMMOM_H
#define AV_COMMOM_H

#include <vector>
#include <string>
#include "format.h"

namespace OHOS {
namespace Media {
/**
 * @brief AVCodec Type
 *
 * @since 3.1
 * @version 3.1
 */
enum AVCodecType : int32_t {
    AVCODEC_TYPE_NONE = -1,
    AVCODEC_TYPE_VIDEO_ENCODER = 0,
    AVCODEC_TYPE_VIDEO_DECODER,
    AVCODEC_TYPE_AUDIO_ENCODER,
    AVCODEC_TYPE_AUDIO_DECODER,
};

/**
 * @brief Range contain min and max value
 *
 * @since 3.1
 * @version 3.1
 */
struct Range {
    int32_t minVal;
    int32_t maxVal;
    Range() : minVal(0), maxVal(0) {}
    Range(const int32_t &min, const int32_t &max)
    {
        if (min <= max) {
            this->minVal = min;
            this->maxVal = max;
        } else {
            this->minVal = 0;
            this->maxVal = 0;
        }
    }

    Range Create(const int32_t &min, const int32_t &max)
    {
        return Range(min, max);
    }

    Range Intersect(const int32_t &min, const int32_t &max)
    {
        int32_t minCmp = this->minVal > min ? this->minVal : min;
        int32_t maxCmp = this->maxVal < max ? this->maxVal : max;
        return this->Create(minCmp, maxCmp);
    }

    Range Intersect(const Range &range)
    {
        int32_t minCmp = this->minVal > range.minVal ? this->minVal : range.minVal;
        int32_t maxCmp = this->maxVal < range.maxVal ? this->maxVal : range.maxVal;
        return this->Create(minCmp, maxCmp);
    }
};

/**
 * @brief ImgSize contain width and height
 *
 * @since 3.1
 * @version 3.1
 */
struct ImgSize {
    int32_t width;
    int32_t height;

    ImgSize() : width(0), height(0) {}

    ImgSize(const int32_t &width, const int32_t &height)
    {
        this->width = width;
        this->height = height;
    }

    bool operator<(const ImgSize &p) const
    {
        return (width < p.width) || (width == p.width && height < p.height);
    }
};

/**
 * @brief Capability Data struct of Codec, parser from config file
 *
 * @since 3.1
 * @version 3.1
 */
struct CapabilityData {
    std::string codecName = "";
    int32_t codecType = AVCODEC_TYPE_NONE;
    std::string mimeType = "";
    bool isVendor = false;
    Range bitrate;
    Range channels;
    Range complexity;
    Range alignment;
    Range width;
    Range height;
    Range frameRate;
    Range encodeQuality;
    Range quality;
    Range blockPerFrame;
    Range blockPerSecond;
    ImgSize blockSize;
    std::vector<int32_t> sampleRate;
    std::vector<int32_t> format;
    std::vector<int32_t> profiles;
    std::vector<int32_t> bitrateMode;
    std::vector<int32_t> levels;
    std::map<int32_t, std::vector<int32_t>> profileLevelsMap; // key = profile, vector = levels
    std::map<ImgSize, Range> measuredFrameRate;
};

/**
 * @brief Media type
 *
 * @since 3.1
 * @version 3.1
 */
enum MediaType : int32_t {
    /**
     * track is audio.
     */
    MEDIA_TYPE_AUD = 0,
    /**
     * track is video.
     */
    MEDIA_TYPE_VID = 1,
    /**
     * track is subtitle.
     */
    MEDIA_TYPE_SUBTITLE = 2,
};

/**
 * @brief AVC Profile
 *
 * @since 3.1
 * @version 3.1
 */
enum AVCProfile {
    AVC_PROFILE_BASELINE = 0,
    AVC_PROFILE_CONSTRAINED_BASELINE = 1,
    AVC_PROFILE_CONSTRAINED_HIGH = 2,
    AVC_PROFILE_EXTENDED = 3,
    AVC_PROFILE_HIGH = 4,
    AVC_PROFILE_HIGH_10 = 5,
    AVC_PROFILE_HIGH_422 = 6,
    AVC_PROFILE_HIGH_444 = 7,
    AVC_PROFILE_MAIN = 8,
};

/**
 * @brief HEVC Profile
 *
 * @since 3.1
 * @version 3.1
 */
enum HEVCProfile {
    HEVC_PROFILE_MAIN = 0,
    HEVC_PROFILE_MAIN_10 = 1,
    HEVC_PROFILE_MAIN_STILL = 2,
};

/**
 * @brief MPEG2 Profile
 *
 * @since 3.1
 * @version 3.1
 */
enum MPEG2Profile {
    MPEG2_PROFILE_422 = 0,
    MPEG2_PROFILE_HIGH = 1,
    MPEG2_PROFILE_MAIN = 2,
    MPEG2_PROFILE_SNR = 3,
    MPEG2_PROFILE_SIMPLE = 4,
    MPEG2_PROFILE_SPATIAL = 5,
};

/**
 * @brief MPEG4 Profile
 *
 * @since 3.1
 * @version 3.1
 */
enum MPEG4Profile {
    MPEG4_PROFILE_ADVANCED_CODING = 0,
    MPEG4_PROFILE_ADVANCED_CORE = 1,
    MPEG4_PROFILE_ADVANCED_REAL_TIME = 2,
    MPEG4_PROFILE_ADVANCED_SCALABLE = 3,
    MPEG4_PROFILE_ADVANCED_SIMPLE = 4,
    MPEG4_PROFILE_BASIC_ANIMATED = 5,
    MPEG4_PROFILE_CORE = 6,
    MPEG4_PROFILE_CORE_SCALABLE = 7,
    MPEG4_PROFILE_HYBRID = 8,
    MPEG4_PROFILE_MAIN = 9,
    MPEG4_PROFILE_NBIT = 10,
    MPEG4_PROFILE_SCALABLE_TEXXTURE = 11,
    MPEG4_PROFILE_SIMPLE = 12,
    MPEG4_PROFILE_SIMPLE_FBA = 13,
    MPEG4_PROFILE_SIMPLE_FACE = 14,
    MPEG4_PROFILE_SIMPLE_SCALABLE = 15,
};

/**
 * @brief H263 Profile
 *
 * @since 3.1
 * @version 3.1
 */
enum H263Profile {
    H263_PROFILE_BACKWARD_COMPATIBLE = 0,
    H263_PROFILE_BASELINE = 1,
    H263_PROFILE_H320_CODING = 2,
    H263_PROFILE_HIGH_COMPRESSION = 3,
    H263_PROFILE_HIGH_LATENCY = 4,
    H263_PROFILE_ISW_V2 = 5,
    H263_PROFILE_ISW_V3 = 6,
    H263_PROFILE_INTERLACE = 7,
    H263_PROFILE_INTERNET = 8,
};

/**
 * @brief
 *
 * @since 3.1
 * @version 3.1
 */
enum VP8Profile {
    VP8_PROFILE_MAIN = 0,
};

/**
 * @brief
 *
 * @since 3.1
 * @version 3.1
 */
enum AACProfile {
    AAC_PROFILE_LC = 0,
    AAC_PROFILE_ELD = 1,
    AAC_PROFILE_ERLC = 2,
    AAC_PROFILE_HE = 3,
    AAC_PROFILE_HE_V2 = 4,
    AAC_PROFILE_LD = 5,
    AAC_PROFILE_MAIN = 6,
};

/**
 * @brief
 *
 * @since 3.1
 * @version 3.1
 */
enum AVCLevel {
    AVC_LEVEL_1 = 0,
    AVC_LEVEL_1b = 1,
    AVC_LEVEL_11 = 2,
    AVC_LEVEL_12 = 3,
    AVC_LEVEL_13 = 4,
    AVC_LEVEL_2 = 5,
    AVC_LEVEL_21 = 6,
    AVC_LEVEL_22 = 7,
    AVC_LEVEL_3 = 8,
    AVC_LEVEL_31 = 9,
    AVC_LEVEL_32 = 10,
    AVC_LEVEL_4 = 11,
    AVC_LEVEL_41 = 12,
    AVC_LEVEL_42 = 13,
    AVC_LEVEL_5 = 14,
    AVC_LEVEL_51 = 15,
};

/**
 * @brief
 *
 * @since 3.1
 * @version 3.1
 */
enum HEVCLevel {
    HEVC_LEVEL_1 = 0,
    HEVC_LEVEL_2 = 1,
    HEVC_LEVEL_21 = 2,
    HEVC_LEVEL_3 = 3,
    HEVC_LEVEL_31 = 4,
    HEVC_LEVEL_4 = 5,
    HEVC_LEVEL_41 = 6,
    HEVC_LEVEL_5 = 7,
    HEVC_LEVEL_51 = 8,
};

/**
 * @brief
 *
 * @since 3.1
 * @version 3.1
 */
enum MPEG2Level {
    MPEG2_LEVEL_LL = 0,
    MPEG2_LEVEL_ML = 1,
    MPEG2_LEVEL_H14 = 2,
    MPEG2_LEVEL_HL = 3,
};

/**
 * @brief
 *
 * @since 3.1
 * @version 3.1
 */
enum MPEG4Level {
    MPEG4_LEVEL_0 = 0,
    MPEG4_LEVEL_0B = 1,
    MPEG4_LEVEL_1 = 2,
    MPEG4_LEVEL_2 = 3,
    MPEG4_LEVEL_3 = 4,
    MPEG4_LEVEL_4 = 5,
    MPEG4_LEVEL_4A = 6,
    MPEG4_LEVEL_5 = 7,
};

/**
 * @brief
 *
 * @since 3.1
 * @version 3.1
 */
enum VideoPixelFormat {
    /**
     * yuv 420 planar.
    */
    YUVI420 = 1,
    /**
     *  NV12. yuv 420 semiplanar.
    */
    NV12 = 2,
    /**
     *  NV21. yvu 420 semiplanar.
    */
    NV21 = 3,
    /**
     * format from surface.
    */
    // SURFACE_FORMAT = 4,
};

/**
 * @brief
 *
 * @since 3.1
 * @version 3.1
 */
enum AudioRawFormat {
    /**
     * signed 8 bits.
     */
    AUDIO_PCM_S8 = 1,
    /**
     * unsigned 8 bits.
     */
    AUDIO_PCM_8 = 2,
    /**
     * signed 16 bits in big endian.
     */
    AUDIO_PCM_S16_BE = 3,
    /**
     * signed 16 bits in little endian.
     */
    AUDIO_PCM_S16_LE = 4,
    /**
     * unsigned 16 bits in big endian.
     */
    AUDIO_PCM_16_BE = 5,
    /**
     * unsigned 16 bits in little endian.
     */
    AUDIO_PCM_16_LE = 6,
    /**
     * signed 24 bits in big endian.
     */
    AUDIO_PCM_S24_BE = 7,
    /**
     * signed 24 bits in little endian.
     */
    AUDIO_PCM_S24_LE = 8,
    /**
     * unsigned 24 bits in big endian.
     */
    AUDIO_PCM_24_BE = 9,
    /**
     * unsigned 24 bits in little endian.
     */
    AUDIO_PCM_24_LE = 10,
    /**
     * signed 32 bits in big endian.
     */
    AUDIO_PCM_S32_BE = 11,
    /**
     * signed 32 bits in little endian.
     */
    AUDIO_PCM_S32_LE = 12,
    /**
     * unsigned 32 bits in big endian.
     */
    AUDIO_PCM_32_BE = 13,
    /**
     * unsigned 32 bits in little endian.
     */
    AUDIO_PCM_32_LE = 14,
    /**
     * float 32 bits in big endian.
     */
    AUDIO_PCM_F32_BE = 15,
    /**
     * float 32 bits in little endian.
     */
    AUDIO_PCM_F32_LE = 16,
};

/**
 * @brief
 *
 * @since 3.1
 * @version 3.1
 */
enum VideoEncodeBitrateMode {
    /**
     * constant bit rate mode.
    */
    CBR = 0,
    /**
     * variable bit rate mode.
    */
    VBR = 1,
    /**
     * constant quality mode.
    */
    CQ = 2,
};

enum ContainerFormatType : int32_t {
    /** MP4 */
    CFT_MPEG_4 = 0,
    /** M4A */
    CFT_MPEG_4A,
};

/**
 * @brief the struct of geolocation
 *
 * @param latitude float: latitude in degrees. Its value must be in the range [-90, 90].
 * @param longitude float: longitude in degrees. Its value must be in the range [-180, 180].
 * @since  3.1
 * @version 3.1
 */
struct Location {
    float latitude = 0;
    float longitude = 0;
};

enum CodecMimeType : int32_t {
    CODEC_MIMIE_TYPE_DEFAULT = -1,
    /** H263 */
    CODEC_MIMIE_TYPE_VIDEO_H263,
    /** H264 */
    CODEC_MIMIE_TYPE_VIDEO_AVC,
    /** MPEG2 */
    CODEC_MIMIE_TYPE_VIDEO_MPEG2,
    /** HEVC */
    CODEC_MIMIE_TYPE_VIDEO_HEVC,
    /** MPEG4 */
    CODEC_MIMIE_TYPE_VIDEO_MPEG4,
    /** MP3 */
    CODEC_MIMIE_TYPE_AUDIO_MPEG,
    /** AAC */
    CODEC_MIMIE_TYPE_AUDIO_AAC,
    /** VORBIS */
    CODEC_MIMIE_TYPE_AUDIO_VORBIS,
    /** FLAC */
    CODEC_MIMIE_TYPE_AUDIO_FLAC,
};

/**
 * @brief Enumerates output format types.
 *
 * @since 3.1
 * @version 3.1
 */
enum OutputFormatType : int32_t {
    /** Default format */
    FORMAT_DEFAULT = 0,
    /** MPEG4 format */
    FORMAT_MPEG_4,
    /** M4A format */
    FORMAT_M4A,
    /** BUTT */
    FORMAT_BUTT,
};

/**
 * @brief Enumerates video codec formats.
 *
 * @since 3.1
 * @version 3.1
 */
enum VideoCodecFormat : int32_t {
    /** Default format */
    VIDEO_DEFAULT = 0,
    /** H.264 */
    H264 = 2,
    /** MPEG4 */
    MPEG4 = 6,
    VIDEO_CODEC_FORMAT_BUTT,
};

/**
 * @brief Enumerates audio codec formats.
 *
 * @since 3.1
 * @version 3.1
 */
enum AudioCodecFormat : int32_t {
    /** Default format */
    AUDIO_DEFAULT = 0,
    /** Advanced Audio Coding Low Complexity (AAC-LC) */
    AAC_LC      =   1,
    /** Invalid value */
    AUDIO_CODEC_FORMAT_BUTT,
};

__attribute__((visibility("default"))) int32_t MapStringToCodecMime(const std::string &mime, CodecMimeType &name);
__attribute__((visibility("default"))) int32_t MapStringToContainerFormat(const std::string &format,
    ContainerFormatType &cft);
__attribute__((visibility("default"))) int32_t MapContainerFormatToOutputFormat(const ContainerFormatType &cft,
    OutputFormatType &opf);
__attribute__((visibility("default"))) int32_t MapCodecMimeToAudioCodec(const CodecMimeType &mime,
    AudioCodecFormat &audio);
__attribute__((visibility("default"))) int32_t MapCodecMimeToVideoCodec(const CodecMimeType &mime,
    VideoCodecFormat &video);
} // namespace Media
} // namespace OHOS
#endif // AV_COMMOM_H