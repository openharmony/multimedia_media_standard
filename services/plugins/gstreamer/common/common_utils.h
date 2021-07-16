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

#ifndef GST_COMMON_UTILS_H
#define GST_COMMON_UTILS_H

#include <stdint.h>
#include <gst/gst.h>

// only C style code is accepted in this file
struct VideoFrameBuffer {
    uint32_t keyFrameFlag;
    uint64_t timeStamp;
    uint64_t duration;
    uint64_t size;
    GstBuffer *gstBuffer;
};

struct EsAvcCodecBuffer {
    uint32_t width;
    uint32_t height;
    uint64_t segmentStart;
    GstBuffer *gstCodecBuffer;
};

enum VideoStreamType {
    VIDEO_STREAM_TYPE_UNKNOWN = 0,
    VIDEO_STREAM_TYPE_ES_AVC,
    VIDEO_STREAM_TYPE_YUV_420,
};

struct AudioBuffer {
    uint64_t timestamp;
    uint32_t dataSeq;
    uint32_t dataLen;
    uint64_t duration;
    GstBuffer *gstBuffer;
};

enum AudioStreamType {
    AUDIO_STREAM_TYPE_UNKNOWN = 0,
    AUDIO_STREAM_TYPE_PCM_S32_LE,
};

enum AudioSourceType {
    AUDIO_SOURCE_TYPE_MIC = 0,
};
#endif  // GST_COMMON_UTILS_H
