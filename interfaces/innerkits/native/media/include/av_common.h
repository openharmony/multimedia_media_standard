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

#include <cstdint>
#include <vector>
#include "format.h"

namespace OHOS {
namespace Media {
/**
 * @brief 
 *
 * @since 1.0
 * @version 1.0
 */
enum AVCodecType : int32_t {
    AVCODEC_TYPE_NONE = -1,
    AVCODEC_TYPE_VIDEO_ENCODER = 0,
    AVCODEC_TYPE_VIDEO_DECODER,
    AVCODEC_TYPE_AUDIO_ENCODER,
    AVCODEC_TYPE_AUDIO_DECODER,
};
/**
 * @brief 
 *
 * @since 1.0
 * @version 1.0
 */
struct Range {
    int32_t minVal = 0;
    int32_t maxVal = 0;
};

/**
 * @brief 
 *
 * @since 1.0
 * @version 1.0
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
    std::vector<int32_t> sampleRate;
    std::vector<int32_t> format;
    std::vector<int32_t> profiles;
    std::vector<int32_t> bitrateMode;
    std::vector<int32_t> levels;
};

/**
 * @brief 
 *
 * @since 1.0
 * @version 1.0
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
 * @brief 
 *
 * @since 1.0
 * @version 1.0
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
 * @brief 
 *
 * @since 1.0
 * @version 1.0
 */
enum HEVCProfile {
    HEVC_PROFILE_MAIN = 0,
    HEVC_PROFILE_MAIN_10 = 1,
    HEVC_PROFILE_MAIN_10_HDR10 = 2,
    HEVC_PROFILE_MAIN_10_HDR10_PLUS = 3,
    HEVC_PROFILE_MAIN_STILL = 4,
};

/**
 * @brief 
 *
 * @since 1.0
 * @version 1.0
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
 * @brief 
 *
 * @since 1.0
 * @version 1.0
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
 * @brief 
 *
 * @since 1.0
 * @version 1.0
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
 * @since 1.0
 * @version 1.0
 */
enum VP8Profile {
    VP8_PROFILE_MAIN = 0,
};

/**
 * @brief 
 *
 * @since 1.0
 * @version 1.0
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
 * @since 1.0
 * @version 1.0
 */
enum VideoPixelformat {
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
 * @since 1.0
 * @version 1.0
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
 * @since 1.0
 * @version 1.0
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

/**
 * @brief 
 *
 * @since 1.0
 * @version 1.0
 */
enum AVCodecListErrorType : int32_t {
    AVCODECLIST_ERROR_INTERNAL,
    AVCODECLIST_ERROR_EXTEND_START = 0X10000,
};
}
}
#endif // AV_COMMOM_H