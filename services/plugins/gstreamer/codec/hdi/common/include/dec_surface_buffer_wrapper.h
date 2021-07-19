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
#include <surface.h>

#include <gmodule.h>
#include <gst/gst.h>
#include <gst/video/video.h>

namespace OHOS {
namespace Media {
class DecSurfaceBufferWrapper {
public:
  DecSurfaceBufferWrapper(sptr<Surface> surface, sptr<SurfaceBuffer> &surfaceBuffer)
    : surface(surface), surfaceBuffer(surfaceBuffer), bufferIsFlush(false)
  {
  };
  ~DecSurfaceBufferWrapper() {};
  inline sptr<SurfaceBuffer> GetSurfaceBuffer()
  {
    return surfaceBuffer;
  }

  inline void SetBufferFlushed()
  {
    bufferIsFlush = true;
  }

  inline bool BufferIsFlushed()
  {
    return bufferIsFlush;
  }

  inline sptr<Surface> getSurface()
  {
    return surface;
  }

private:
  sptr<Surface> surface;
  sptr<SurfaceBuffer> surfaceBuffer;
  bool bufferIsFlush;
};
}
}

#endif /* GST_DEC_SURFACE_WRAPPER_H */