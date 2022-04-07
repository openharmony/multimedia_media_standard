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
#ifndef AV_COMMOM_H
#define AV_COMMOM_H

#include <vector>
#include <string>
#include "format.h"

namespace OHOS {
namespace Media {
/**
 * @brief Media type
 *
 * @since 3.1
 * @version 3.1
 */
enum MediaType : int32_t {
    /**
     * track is audio.
     */
    MEDIA_TYPE_AUD = 0,
    /**
     * track is video.
     */
    MEDIA_TYPE_VID = 1,
    /**
     * track is subtitle.
     */
    MEDIA_TYPE_SUBTITLE = 2,
};

/**
 * @brief
 *
 * @since 3.1
 * @version 3.1
 */
enum VideoPixelFormat {
    /**
     * yuv 420 planar.
     */
    YUVI420 = 1,
    /**
     *  NV12. yuv 420 semiplanar.
     */
    NV12 = 2,
    /**
     *  NV21. yvu 420 semiplanar.
     */
    NV21 = 3,
    /**
     * format from surface.
     */
    SURFACE_FORMAT = 4,
    /**
     * RGBA.
     */
    RGBA = 5,
};

/**
 * @brief the struct of geolocation
 *
 * @param latitude float: latitude in degrees. Its value must be in the range [-90, 90].
 * @param longitude float: longitude in degrees. Its value must be in the range [-180, 180].
 * @since  3.1
 * @version 3.1
 */
struct Location {
    float latitude = 0;
    float longitude = 0;
};

/**
 * @brief Enumerates the seek mode.
 */
enum AVSeekMode : uint8_t {
    /**
     * @brief this mode is used to seek to a key frame that is located right or before at
     * the given timestamp.
     */
    AV_SEEK_PREV_SYNC = 0,
    /**
     * @brief this mode is used to seek to a key frame that is located right or after at
     * the given timestamp.
     */
    AV_SEEK_NEXT_SYNC = 1,
    /**
     * @brief this mode is used to seek to a key frame that is located right or closest at
     * the given timestamp.
     */
    AV_SEEK_CLOSEST_SYNC = 2,
    /**
     * @brief this mode is used to seek to a frame that is located right or closest at
     * the given timestamp.
     */
    AV_SEEK_CLOSEST = 3,
};
} // namespace Media
} // namespace OHOS
#endif // AV_COMMOM_H