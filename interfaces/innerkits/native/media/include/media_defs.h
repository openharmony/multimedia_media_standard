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

#ifndef MEDIA_DEFS_H
#define MEDIA_DEFS_H

#include <cstdint>

namespace OHOS {
namespace Media {
/**
 * @brief Enumetates the media track type
 */
enum MediaTrackType : uint8_t {
    /**
     * @brief Track is audio
     */
    MEDIA_TYPE_AUDIO = 0,
    /**
     * @brief Track is video
     */
    MEDIA_TYPE_VIDEO = 1,
    /**
     * @brief Track is subtitle
     */
    MEDIA_TYPE_SUBTITLE = 2,
};
}
}
#endif