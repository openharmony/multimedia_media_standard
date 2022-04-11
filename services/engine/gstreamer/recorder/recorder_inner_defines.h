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

#ifndef RECORDER_INNER_DEFINES_H
#define RECORDER_INNER_DEFINES_H

#include <cstdint>
#include <functional>
#include <unordered_map>
#include "media_errors.h"
#include "i_recorder_engine.h"

namespace OHOS {
namespace Media {
/**
 * @brief Max number of sources supported for multi-source concurrent recording.
 */
static constexpr uint8_t VIDEO_SOURCE_MAX_COUNT = 1;
static constexpr uint8_t AUDIO_SOURCE_MAX_COUNT = 1;

/**
 * @brief Invalid source id, represent that the source set fail result
 */
static constexpr int32_t INVALID_SOURCE_ID = -1;

enum RecorderSourceKind : int32_t {
    RECORDER_SOURCE_KIND_VIDEO,
    RECORDER_SOURCE_KIND_AUDIO,
    RECORDER_SOURCE_KIND_MAX,
};

struct RecorderSourceDesc {
    static constexpr uint32_t RECORDER_SOURCE_KIND_MASK = 0xF00;
    static constexpr uint32_t RECORDER_VIDEO_SOURCE_KIND_MASK = 0x100;
    static constexpr uint32_t RECORDER_AUDIO_SOURCE_KIND_MASK = 0x200;
    static constexpr uint32_t RECORDER_SOURCE_INDEX_MASK = 0xFF;
    /**
      Handle is currently represented as int32_t, and internal descripted as source kind mask + index :
      high 20bits(reserved) + 4bits(source kind mask) + 8bits(index).
      The handle can uniquely identify the recorder source.
    */
    int32_t handle_ = DUMMY_SOURCE_ID;
    /* Auxiliary info */
    int32_t type_ = -1; // VideoSourceType or AudioSourceType etc. -1 represents invalid value for both source kind.

    inline void SetVideoSource(int32_t type, int32_t index)
    {
        type_ = type;
        handle_ = static_cast<int32_t>(RECORDER_VIDEO_SOURCE_KIND_MASK +
            (RECORDER_SOURCE_INDEX_MASK & static_cast<uint32_t>(index)));
    }

    inline void SetAudioSource(int32_t type, int32_t index)
    {
        type_ = type;
        handle_ = static_cast<int32_t>(RECORDER_AUDIO_SOURCE_KIND_MASK +
            (RECORDER_SOURCE_INDEX_MASK & static_cast<uint32_t>(index)));
    }

    inline bool IsAudio() const
    {
        return ((handle_ > 0) &&
               ((static_cast<uint32_t>(handle_) & RECORDER_SOURCE_KIND_MASK) == RECORDER_AUDIO_SOURCE_KIND_MASK));
    }

    inline bool IsVideo() const
    {
        return ((handle_ > 0) &&
               ((static_cast<uint32_t>(handle_) & RECORDER_SOURCE_KIND_MASK) == RECORDER_VIDEO_SOURCE_KIND_MASK));
    }
};
} // namespace Media
} // namespace OHOS
#endif
