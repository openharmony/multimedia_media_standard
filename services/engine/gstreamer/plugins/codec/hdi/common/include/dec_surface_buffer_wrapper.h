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

#ifndef GST_DEC_SURFACE_WRAPPER_H
#define GST_DEC_SURFACE_WRAPPER_H

#include "surface.h"

namespace OHOS {
namespace Media {
class DecSurfaceBufferWrapper {
public:
    DecSurfaceBufferWrapper(sptr<Surface> surface, sptr<SurfaceBuffer> &surfaceBuffer)
        : surface_(surface), surfaceBuffer_(surfaceBuffer)
    {
    }

    ~DecSurfaceBufferWrapper() = default;

    inline sptr<SurfaceBuffer> GetSurfaceBuffer()
    {
        return surfaceBuffer_;
    }

    inline void SetBufferFlushed()
    {
        bufferIsFlush_ = true;
    }

    inline bool BufferIsFlushed()
    {
        return bufferIsFlush_;
    }

    inline sptr<Surface> GetSurface()
    {
        return surface_;
    }

    inline uint32_t GetSurfaceSize()
    {
        return size_;
    }

    inline void SetSurfaceSize(uint32_t size)
    {
        size_ = size;
    }
private:
    sptr<Surface> surface_;
    sptr<SurfaceBuffer> surfaceBuffer_;
    uint32_t size_ = 0;
    bool bufferIsFlush_ = false;
};
} // namespace Media
} // namespace OHOS
#endif /* GST_DEC_SURFACE_WRAPPER_H */