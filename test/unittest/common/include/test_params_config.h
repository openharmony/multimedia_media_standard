/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef TEST_PARAM_COMMON_H
#define TEST_PARAM_COMMON_H

#include <cstdint>
#include <string>
#include "avmetadatahelper.h"
#include "recorder.h"

namespace OHOS {
namespace Media {
namespace PlayerTestParam {
inline constexpr int32_t SEEK_TIME_5_SEC = 5000;
inline constexpr int32_t SEEK_TIME_2_SEC = 2000;
inline constexpr int32_t WAITSECOND = 6;
inline constexpr int32_t DELTA_TIME = 1000;
const std::string MEDIA_ROOT = "file:///data/test/";
const std::string VIDEO_FILE1 = MEDIA_ROOT + "H264_AAC.mp4";
} // namespace PlayerTestParam
namespace AVMetadataTestParam {
inline constexpr int32_t PARA_MAX_LEN = 256;
#define AVMETA_KEY_TO_STRING_MAP_ITEM(key) { key, #key }
static const std::unordered_map<int32_t, std::string_view> AVMETA_KEY_TO_STRING_MAP = {
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_ALBUM),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_ALBUM_ARTIST),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_DATE_TIME),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_ARTIST),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_AUTHOR),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_COMPOSER),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_DURATION),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_GENRE),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_HAS_AUDIO),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_HAS_VIDEO),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_MIME_TYPE),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_NUM_TRACKS),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_SAMPLE_RATE),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_TITLE),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_VIDEO_HEIGHT),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_VIDEO_WIDTH),
};
} // namespace PlAVMetadataTestParamayerTestParam
namespace RecorderTestParam {
    constexpr uint32_t STUB_STREAM_SIZE = 602;
    constexpr uint32_t FRAME_RATE = 30000;
    constexpr uint32_t CODEC_BUFFER_WIDTH = 1024;
    constexpr uint32_t CODEC_BUFFER_HEIGHT = 25;
    constexpr uint32_t YUV_BUFFER_WIDTH = 1280;
    constexpr uint32_t YUV_BUFFER_HEIGHT = 720;
    constexpr uint32_t STRIDE_ALIGN = 8;
    constexpr uint32_t FRAME_DURATION = 40000000;
    constexpr uint32_t RECORDER_TIME = 5;
    constexpr uint32_t YUV_BUFFER_SIZE = YUV_BUFFER_WIDTH * YUV_BUFFER_HEIGHT * 3 / 2;
    constexpr uint32_t SEC_TO_NS = 1000000000;
    const std::string PURE_VIDEO = "video";
    const std::string PURE_AUDIO = "audio";
    const std::string AUDIO_VIDEO = "av";
    // this array contains each buffer size of the stub stream
    const uint32_t HIGH_VIDEO_FRAME_SIZE[STUB_STREAM_SIZE] = {
        13571, 321, 72, 472, 68, 76, 79, 509, 90, 677, 88, 956, 99, 347, 77, 452, 681, 81, 1263, 94, 106, 97, 998,
        97, 797, 93, 1343, 150, 116, 117, 926, 1198, 128, 110, 78, 1582, 158, 135, 112, 1588, 165, 132, 128, 1697,
        168, 149, 117, 1938, 170, 141, 142, 1830, 106, 161, 122, 1623, 160, 154, 156, 1998, 230, 177, 139, 1650,
        186, 128, 134, 1214, 122, 1411, 120, 1184, 128, 1591, 195, 145, 105, 1587, 169, 140, 118, 1952, 177, 150,
        161, 1437, 159, 123, 1758, 180, 165, 144, 1936, 214, 191, 175, 2122, 180, 179, 160, 1927, 161, 184, 119,
        1973, 218, 210, 129, 1962, 196, 127, 154, 2308, 173, 127, 1572, 142, 122, 2065, 262, 159, 206, 2251, 269,
        179, 170, 2056, 308, 168, 191, 2090, 303, 191, 110, 1932, 272, 162, 122, 1877, 245, 167, 141, 1908, 294,
        162, 118, 1493, 132, 1782, 273, 184, 133, 1958, 274, 180, 149, 2070, 216, 169, 143, 1882, 224, 149, 139,
        1749, 277, 184, 139, 2141, 197, 170, 140, 2002, 269, 162, 140, 1862, 202, 179, 131, 1868, 214, 164, 140,
        1546, 226, 150, 130, 1707, 162, 146, 1824, 181, 147, 130, 1898, 209, 143, 131, 1805, 180, 148, 106, 1776,
        147, 141, 1572, 177, 130, 105, 1776, 178, 144, 122, 1557, 142, 124, 114, 1436, 143, 126, 1326, 127, 1755,
        169, 127, 105, 1807, 177, 131, 134, 1613, 187, 137, 136, 1314, 134, 118, 2005, 194, 129, 147, 1566, 185,
        132, 131, 1236, 174, 137, 106, 11049, 574, 126, 1242, 188, 130, 119, 1450, 187, 137, 141, 1116, 124, 1848,
        138, 122, 1605, 186, 127, 140, 1798, 170, 124, 121, 1666, 157, 128, 130, 1678, 135, 118, 1804, 169, 135,
        125, 1837, 168, 124, 124, 2049, 180, 122, 128, 1334, 143, 128, 1379, 116, 1884, 149, 122, 150, 1962, 176,
        122, 122, 1197, 139, 1853, 184, 151, 148, 1692, 209, 129, 126, 1736, 149, 135, 104, 1775, 165, 160, 121,
        1653, 163, 123, 112, 1907, 181, 129, 107, 1808, 177, 125, 111, 2405, 166, 144, 114, 1833, 198, 136, 113,
        1960, 206, 139, 116, 1791, 175, 130, 129, 1909, 194, 138, 119, 1807, 160, 156, 124, 1998, 184, 173, 114,
        2069, 181, 127, 139, 2212, 182, 138, 146, 1993, 214, 135, 139, 2286, 194, 137, 120, 2090, 196, 159, 132,
        2294, 194, 148, 137, 2312, 183, 163, 106, 2118, 201, 158, 127, 2291, 187, 144, 116, 2413, 139, 115, 2148,
        178, 122, 103, 2370, 207, 161, 117, 2291, 213, 159, 129, 2244, 243, 157, 133, 2418, 255, 171, 127, 2316,
        185, 160, 132, 2405, 220, 165, 155, 2539, 219, 172, 128, 2433, 199, 154, 119, 1681, 140, 1960, 143, 2682,
        202, 153, 127, 2794, 239, 158, 155, 2643, 229, 172, 125, 2526, 201, 181, 159, 2554, 233, 167, 125, 2809,
        205, 164, 117, 2707, 221, 156, 138, 2922, 240, 160, 146, 2952, 267, 177, 149, 3339, 271, 175, 136, 3006,
        242, 168, 141, 3068, 232, 194, 149, 2760, 214, 208, 143, 2811, 218, 184, 149, 137, 15486, 2116, 235, 167,
        157, 2894, 305, 184, 139, 3090, 345, 179, 155, 3226, 347, 160, 164, 3275, 321, 184, 174, 3240, 269, 166,
        170, 3773, 265, 169, 155, 3023, 301, 188, 161, 3343, 275, 174, 155, 3526, 309, 177, 173, 3546, 307, 183,
        149, 3648, 295, 213, 170, 3568, 305, 198, 166, 3641, 297, 172, 148, 3608, 301, 200, 159, 3693, 322, 209,
        166, 3453, 318, 206, 162, 3696, 341, 200, 176, 3386, 320, 192, 176, 3903, 373, 207, 187, 3305, 361, 200,
        202, 3110, 367, 220, 197, 2357, 332, 196, 201, 1827, 377, 187, 199, 860, 472, 173, 223, 238};

    struct VideoRecorderConfig {
        int32_t audioSourceId = 0;
        int32_t videoSourceId = 0;
        int32_t audioEncodingBitRate = 48000;
        int32_t channelCount = 2;
        int32_t duration = 60;
        int32_t width = YUV_BUFFER_WIDTH;
        int32_t height = YUV_BUFFER_HEIGHT;
        int32_t frameRate = 30;
        int32_t videoEncodingBitRate = 48000;
        int32_t sampleRate = 48000;
        double captureFps = 30;
        int32_t outputFd = 0;
        AudioCodecFormat audioFormat = AAC_LC;
        AudioSourceType aSource = AUDIO_MIC;
        OutputFormatType outPutFormat = FORMAT_MPEG_4;
        VideoSourceType vSource = VIDEO_SOURCE_SURFACE_ES;
        VideoCodecFormat videoFormat = MPEG4;
    };
    struct AudioRecorderConfig {
        int32_t outputFd = 0;
        int32_t audioSourceId = 0;
        int32_t audioEncodingBitRate = 48000;
        int32_t channelCount = 2;
        int32_t duration = 60;
        int32_t sampleRate = 48000;
        AudioCodecFormat audioFormat = AAC_LC;
        AudioSourceType inputSource = AUDIO_MIC;
        OutputFormatType outPutFormat = FORMAT_M4A;
    };
} // namespace RecorderTestParam
} // namespace Media
} // namespace OHOS

#endif