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

#ifndef MEDIA_SURFACE_H
#define MEDIA_SURFACE_H

#include "surface.h"

namespace OHOS {
namespace Media {
class MediaSurface {
public:
    virtual ~MediaSurface() = default;
    virtual std::string GetSurfaceId(const sptr<Surface> &surface) = 0;
    virtual sptr<Surface> GetSurface() = 0;
    virtual sptr<Surface> GetSurface(const std::string &id) = 0;
    virtual void Release() = 0;
};

class __attribute__((visibility("default"))) MediaSurfaceFactory {
public:
    static std::shared_ptr<MediaSurface> CreateMediaSurface();
    MediaSurfaceFactory() = delete;
    ~MediaSurfaceFactory() = delete;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_SURFACE_H