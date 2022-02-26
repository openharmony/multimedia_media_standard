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

#ifndef AVMUXER_UTIL_H
#define AVMUXER_UTIL_H

#include <functional>
#include <set>
#include <map>
#include <vector>
#include <gst/gst.h>
#include "gstappsrc.h"
#include "i_avmuxer_engine.h"
#include "nocopyable.h"
#include "gst_shmem_wrap_allocator.h"
#include "gst_mux_bin.h"

namespace OHOS {
namespace Media {

const std::map<std::string, std::string> FORMAT_TO_MUX {
    {"mp4", "qtmux"},
    {"m4a", "qtmux"}
};

const std::map<std::string, std::set<std::string>> FORMAT_TO_MIME {
    {"mp4", {"video/avc", "video/mp4v-es", "video/h263", "audio/mp4a-latm", "audio/mpeg"}},
    {"m4a", {"audio/mp4a-latm"}}
};

const std::map<const std::string, std::tuple<const std::string, CodecMimeType>> MIME_MAP_TYPE {
    {"video/avc", {"video/x-h264", CODEC_MIMIE_TYPE_VIDEO_AVC}},
    {"video/h263", {"video/x-h263", CODEC_MIMIE_TYPE_VIDEO_H263}},
    {"video/mp4v-es", {"video/mpeg", CODEC_MIMIE_TYPE_VIDEO_MPEG4}},
    {"audio/mp4a-latm", {"audio/mpeg", CODEC_MIMIE_TYPE_AUDIO_AAC}},
    {"audio/mpeg", {"audio/mpeg", CODEC_MIMIE_TYPE_AUDIO_MPEG}}
};

class TrackInfo {
public:
    bool hasCodecData_ = false;
    bool hasBuffer_ = false;
    bool needData_ = false;
    GstCaps *caps_ = nullptr;
    GstAppSrc *src_ = nullptr;
    CodecMimeType type_ ;
};

class FormatParam {
public:
    int32_t width;
    int32_t height;
    int32_t frameRate;
    int32_t channels;
    int32_t rate;
};

class AVMuxerUtil {
public:
    AVMuxerUtil() = delete;
    ~AVMuxerUtil() = delete;
    DISALLOW_COPY_AND_MOVE(AVMuxerUtil);

    static bool isVideo(CodecMimeType type);
    static int32_t SetCaps(const MediaDescription &trackDesc, const std::string &mimeType,
        GstCaps *src_caps, CodecMimeType type);
    static int32_t WriteData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
        GstAppSrc *src, std::map<int, TrackInfo>& trackInfo, GstShMemWrapAllocator *allocator);
};
}
}
#endif