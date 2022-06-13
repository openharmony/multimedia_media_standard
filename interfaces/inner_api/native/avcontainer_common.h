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

#include "nocopyable.h"
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
    static constexpr std::string_view CFT_MPEG_TS = "mpeg-ts";
    static constexpr std::string_view CFT_MKV = "mkv";
    static constexpr std::string_view CFT_WEBM = "webm";
    static constexpr std::string_view CFT_OGG = "ogg";
    static constexpr std::string_view CFT_WAV = "wav";
    static constexpr std::string_view CFT_AAC = "aac";
    static constexpr std::string_view CFT_FLAC = "flac";
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

class AVContainerMemory : public NoCopyable {
public:
    AVContainerMemory(uint8_t *base, size_t capacity) : base_(base), capacity_(capacity) {};

    ~AVContainerMemory() = default;

    uint8_t *Base() const
    {
        return base_;
    }

    uint8_t *Data() const
    {
        return base_ + offset_;
    }

    size_t Capacity() const
    {
        return capacity_;
    }

    size_t Size() const
    {
        return size_;
    }

    size_t Offset() const
    {
        return offset_;
    }

    void SetRange(size_t offset, size_t size)
    {
        offset_ = offset;
        size_ = size;
    }

private:
    uint8_t *base_ = nullptr;
    size_t offset_ = 0;
    size_t size_ = 0;
    size_t capacity_ = 0;
};
} // namespace Media
} // namespace OHOS
#endif // AVCONTAINER_TYPES_H
