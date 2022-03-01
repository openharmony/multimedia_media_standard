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

#ifndef AVCONTAINER_TYPES_H
#define AVCONTAINER_TYPES_H

#include <string_view>
#include "av_common.h"
#include "avcodec_common.h"

namespace OHOS {
namespace Media {
/**
 * @brief Enumerates the container format types.
 */
class ContainerFormatType {
public:
    static constexpr std::string_view CFT_MPEG_4A = "m4a";
    static constexpr std::string_view CFT_MPEG_4 = "mp4";
};

/**
 * @brief Description information of a sample associated a media track.
 */
struct TrackSampleInfo {
    /**
     * @brief the id of track that this sample belongs to.
     */
    uint32_t trackIdx;
    /**
     * @brief the presentation timestamp in microseconds.
     */
    int64_t timeUs;
    /**
     * @brief the size in bytes.
     */
    uint32_t size;
    /**
     * @brief the flags associated with the sample, this
     * maybe be a combination of multiple {@link AVCodecBufferFlag}.
     */
    AVCodecBufferFlag flags;
};
}
}
#endif
