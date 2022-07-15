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
#include "av_common.h"

namespace OHOS {
namespace Media {
struct TrackInfo {
    bool hasCodecData_ = false;
    bool needData_ = true;
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

class AVMuxerUtil {
public:
    AVMuxerUtil() = delete;
    ~AVMuxerUtil() = delete;

    static bool SetCaps(const MediaDescription &trackDesc, const std::string &mimeType,
        GstCaps **src_caps);
    static std::vector<std::string> FindFormat();
    static bool FindMux(const std::string &format, std::string &mux);
    static bool FindMimeTypes(const std::string &format, std::set<std::string> &mimeTypes);
    static bool FindInnerTypes(const std::string &mimeType, std::string &innerType);
    static bool FindParse(const std::string &mimeType, std::string &parse);
    static bool FindMediaType(const std::string &mimeType, MediaType &mediaType);
};
}  // namespace Media
}  // namespace OHOS
#endif  // AVMUXER_UTIL_H