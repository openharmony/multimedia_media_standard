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

#include "gst_dec_surface.h"
#include "dec_surface_buffer_wrapper.h"
#include "display_type.h"

using namespace OHOS;
namespace {
    constexpr int DEFAULT_HDI_ALIGNMENT = 4;
}

guint8 * GetVirBuffer(GstBuffer * gstSurfaceBuffer)
{
  GstMapInfo map = GST_MAP_INFO_INIT;

  if (!gst_buffer_map (gstSurfaceBuffer, &map, GST_MAP_READ)) {
    GST_ERROR_OBJECT (nullptr, "Failed to map output buffer");
    return nullptr;
  }
  Media::DecSurfaceBufferWrapper * mSurfaceBufferWrap = (Media::DecSurfaceBufferWrapper *)map.data;
  gst_buffer_unmap (gstSurfaceBuffer, &map);
  sptr<SurfaceBuffer> surfaceBuffer = mSurfaceBufferWrap->GetSurfaceBuffer();
  return (guint8 *)surfaceBuffer->GetVirAddr();
}

static void FreeSurfaceBufferWrapper(gpointer mSurfaceBufferWrapper)
{
  GST_WARNING_OBJECT (nullptr, "Failed to get surface output buffer");
  Media::DecSurfaceBufferWrapper * mSurfaceBufferWrap = (Media::DecSurfaceBufferWrapper *)mSurfaceBufferWrapper;
  sptr<Surface> mProducerSurface = mSurfaceBufferWrap->getSurface();
  if (!mSurfaceBufferWrap->BufferIsFlushed()) {
    sptr<SurfaceBuffer> surfaceBuffer = mSurfaceBufferWrap->GetSurfaceBuffer();
    mProducerSurface->CancelBuffer(surfaceBuffer);
  }
  delete mSurfaceBufferWrap;
}

GstBuffer * GetSurfaceBuffer(void * surface, uint64_t size, uint32_t width, uint32_t height)
{
  sptr<Surface> mProducerSurface = *(sptr<Surface> *)surface;
  sptr<SurfaceBuffer> surfaceBuffer;
  int32_t releaseFence;
  BufferRequestConfig requestConfig;
  requestConfig.width = width;
  requestConfig.height = height;
  requestConfig.strideAlignment = DEFAULT_HDI_ALIGNMENT;
  requestConfig.format = PIXEL_FMT_YCRCB_420_SP;
  requestConfig.usage = HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA;
  requestConfig.timeout = 0;

  SurfaceError ret = mProducerSurface->RequestBuffer(surfaceBuffer, releaseFence, requestConfig);
  if (ret != SURFACE_ERROR_OK) {
    GST_ERROR_OBJECT (nullptr, "Failed to get surface output buffer");
    return nullptr;
  }
  Media::DecSurfaceBufferWrapper * mSurfaceBufferWrapper =
    new Media::DecSurfaceBufferWrapper(mProducerSurface, surfaceBuffer);
  GstBuffer * gstSurfaceBuffer =
    gst_buffer_new_wrapped_full((GstMemoryFlags)0, (gpointer) mSurfaceBufferWrapper,
        sizeof(*mSurfaceBufferWrapper), 0, sizeof(*mSurfaceBufferWrapper), (guint8 *)(mSurfaceBufferWrapper),
        (GDestroyNotify)(FreeSurfaceBufferWrapper));
  return gstSurfaceBuffer;
}