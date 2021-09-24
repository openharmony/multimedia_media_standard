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
constexpr int DEFAULT_INPUT_BUFFER_SIZE = 5;
}

extern "C" guint8 *GetSurfaceBufferVirAddr(GstBuffer *gstSurfaceBuffer)
{
    g_return_val_if_fail(gstSurfaceBuffer != nullptr, nullptr);
    GstMapInfo map = GST_MAP_INFO_INIT;

    if (!gst_buffer_map(gstSurfaceBuffer, &map, GST_MAP_READ)) {
        GST_ERROR_OBJECT(nullptr, "Failed to map output buffer");
        return nullptr;
    }
    Media::DecSurfaceBufferWrapper *surfaceBufferWrap = reinterpret_cast<Media::DecSurfaceBufferWrapper *>(map.data);
    gst_buffer_unmap(gstSurfaceBuffer, &map);
    g_return_val_if_fail(surfaceBufferWrap != nullptr, nullptr);
    sptr<SurfaceBuffer> surfaceBuffer = surfaceBufferWrap->GetSurfaceBuffer();
    return static_cast<guint8 *>(surfaceBuffer->GetVirAddr());
}

extern "C" guint GetSurfaceBufferSize(GstBuffer *gstSurfaceBuffer)
{
    g_return_val_if_fail(gstSurfaceBuffer != nullptr, 0);
    GstMapInfo map = GST_MAP_INFO_INIT;

    if (!gst_buffer_map(gstSurfaceBuffer, &map, GST_MAP_READ)) {
        GST_ERROR_OBJECT(nullptr, "Failed to map output buffer");
        return 0;
    }
    Media::DecSurfaceBufferWrapper *surfaceBufferWrap = reinterpret_cast<Media::DecSurfaceBufferWrapper *>(map.data);
    gst_buffer_unmap(gstSurfaceBuffer, &map);
    g_return_val_if_fail(surfaceBufferWrap != nullptr, 0);
    return static_cast<guint>(surfaceBufferWrap->GetSurfaceSize());
}

static void FreeSurfaceBufferWrapper(gpointer surfaceBufferWrapper)
{
    GST_DEBUG_OBJECT(nullptr, "Free SurfaceBuffer");
    g_return_if_fail(surfaceBufferWrapper != nullptr);
    Media::DecSurfaceBufferWrapper *surfaceBufferWrap =
        static_cast<Media::DecSurfaceBufferWrapper *>(surfaceBufferWrapper);
    if (!surfaceBufferWrap->BufferIsFlushed()) {
        sptr<Surface> producerSurface = surfaceBufferWrap->GetSurface();
        sptr<SurfaceBuffer> surfaceBuffer = surfaceBufferWrap->GetSurfaceBuffer();
        (void)producerSurface->CancelBuffer(surfaceBuffer);
    }
    delete surfaceBufferWrap;
}

extern "C" GstBuffer *SurfaceBufferToGstBuffer(void *surface, guint width, guint height)
{
    sptr<Surface> producerSurface = static_cast<Surface *>(surface);
    if (producerSurface->GetQueueSize() != DEFAULT_INPUT_BUFFER_SIZE) {
        SurfaceError ret = producerSurface->SetQueueSize(DEFAULT_INPUT_BUFFER_SIZE);
        if (ret != SURFACE_ERROR_OK) {
            GST_ERROR_OBJECT(nullptr, "Failed to SetQueueSize");
        }
    }
    sptr<SurfaceBuffer> surfaceBuffer;
    int32_t releaseFence;
    BufferRequestConfig requestConfig;
    requestConfig.width = static_cast<int32_t>(width);
    requestConfig.height = static_cast<int32_t>(height);
    requestConfig.strideAlignment = DEFAULT_HDI_ALIGNMENT;
    requestConfig.format = static_cast<int32_t>(PIXEL_FMT_YCRCB_420_SP);
    requestConfig.usage = HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA;
    requestConfig.timeout = 0;
    SurfaceError ret = producerSurface->RequestBuffer(surfaceBuffer, releaseFence, requestConfig);
    if (ret != SURFACE_ERROR_OK) {
        GST_ERROR_OBJECT(nullptr, "Failed to get surface output buffer");
        return nullptr;
    }
    Media::DecSurfaceBufferWrapper *surfaceBufferWrap =
        new(std::nothrow) Media::DecSurfaceBufferWrapper(producerSurface, surfaceBuffer);
    g_return_val_if_fail(surfaceBufferWrap != nullptr, nullptr);
    surfaceBufferWrap->SetSurfaceSize(surfaceBuffer->GetSize());
    GstBuffer *gstSurfaceBuffer = gst_buffer_new_wrapped_full((GstMemoryFlags)0, (gpointer)surfaceBufferWrap,
        sizeof(*surfaceBufferWrap), 0, sizeof(*surfaceBufferWrap), (guint8 *)(surfaceBufferWrap),
        (GDestroyNotify)(FreeSurfaceBufferWrapper));

    return gstSurfaceBuffer;
}