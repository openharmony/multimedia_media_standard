/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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
#include "i_avmuxer_engine.h"
#include "nocopyable.h"
#include "gst_shmem_wrap_allocator.h"

namespace OHOS {
namespace Media {
const std::map<std::string, std::string> FORMAT_TO_MUX {
    {"mp4", "mp4mux"},
    {"m4a", "mp4mux"}
};

const std::map<std::string, std::set<std::string>> FORMAT_TO_MIME {
    {"mp4", {"video/avc", "video/mp4v-es", "video/h263", "audio/mp4a-latm", "audio/mpeg"}},
    {"m4a", {"audio/mp4a-latm"}}
};

const std::map<const std::string, std::tuple<const std::string, std::string>> MIME_MAP_TYPE {
    {"video/avc", {"video/x-h264", "h264parse"}},
    {"video/h263", {"video/x-h263", ""}},
    {"video/mp4v-es", {"video/mpeg", "mpeg4videoparse"}},
    {"audio/mp4a-latm", {"audio/mpeg", "aacparse"}},
    {"audio/mpeg", {"audio/mpeg", ""}}
};

struct TrackInfo {
    bool hasCodecData_ = false;
    bool needData_ = false;
    GstCaps *caps_ = nullptr;
    GstElement *src_ = nullptr;
    std::string mimeType_;
};

struct FormatParam {
    int32_t width = 0;
    int32_t height = 0;
    int32_t frameRate = 0;
    int32_t channels = 0;
    int32_t rate = 0;
};

enum TrackType : int32_t {
    UNKNOWN_TYPE = -1,
    VIDEO = 0,
    AUDIO = 1,
};

class AVMuxerUtil {
public:
    AVMuxerUtil() = delete;
    ~AVMuxerUtil() = delete;

    static TrackType CheckType(const std::string &mimeType);
    static int32_t SetCaps(const MediaDescription &trackDesc, const std::string &mimeType,
        GstCaps **src_caps);
};
}  // namespace Media
}  // namespace OHOS
#endif  // AVMUXER_UTIL_H