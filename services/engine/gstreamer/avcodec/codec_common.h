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

#ifndef CODEC_COMMON_H
#define CODEC_COMMON_H

#include <string>
#include <gst/audio/audio.h>
#include <gst/gst.h>
#include "av_common.h"
#include "avsharedmemory.h"
#include "format.h"

namespace OHOS {
namespace Media {
enum VideoEncoderBitrateMode : int32_t {
    VIDEO_ENCODER_BITRATE_MODE_CBR = 0,
    VIDEO_ENCODER_BITRATE_MODE_VBR,
    VIDEO_ENCODER_BITRATE_MODE_CQ,
};

enum VideoPixelFormat : int32_t {
    VIDEO_PIXEL_FORMAT_YUVI420 = 0,
    VIDEO_PIXEL_FORMAT_NV12 = 1,
    VIDEO_PIXEL_FORMAT_NV21 = 2,
};

struct BufferWrapper {
    enum Owner : int32_t {
        APP = 0,
        SERVER,
        DOWNSTREAM
    };
    BufferWrapper(std::shared_ptr<AVSharedMemory> mem, GstBuffer *gstBuffer, uint32_t index, Owner owner)
        : mem_(mem),
          gstBuffer_(gstBuffer),
          index_(index),
          owner_(owner)
    {
    }
    ~BufferWrapper()
    {
        if (gstBuffer_ != nullptr) {
            gst_buffer_unref(gstBuffer_);
        }
    }
    std::shared_ptr<AVSharedMemory> mem_ = nullptr;
    GstBuffer *gstBuffer_ = nullptr;
    uint32_t index_ = 0;
    Owner owner_ = DOWNSTREAM;
    GstSample *sample_ = nullptr;
};

struct ProcessorConfig {
    explicit ProcessorConfig(GstCaps *caps)
        : caps_(caps)
    {
    }
    ~ProcessorConfig()
    {
        if (caps_ != nullptr) {
            gst_caps_unref(caps_);
        }
    }
    GstCaps *caps_ = nullptr;
    bool needCodecData_ = false;
};

__attribute__((visibility("default"))) int32_t MapVideoPixelFormat(int32_t number, VideoPixelFormat &pixel);
__attribute__((visibility("default"))) std::string PixelFormatToString(VideoPixelFormat pixel);
__attribute__((visibility("default"))) int32_t MapPCMFormat(int32_t number, AudioRawFormat &format);
__attribute__((visibility("default"))) std::string PCMFormatToString(AudioRawFormat format);
__attribute__((visibility("default"))) int32_t MapBitrateMode(int32_t number, VideoEncoderBitrateMode &mode);
__attribute__((visibility("default"))) int32_t MapCodecMime(const std::string &mime, CodecMimeType &name);
__attribute__((visibility("default"))) int32_t MapProfile(int32_t number, AVCProfile &profile);
__attribute__((visibility("default"))) int32_t ParseCaps(GstCaps *caps, Format &format);
} // Media
} // OHOS
#endif // CODEC_COMMON_H
