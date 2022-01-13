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

#ifndef GST_SURFACE_ALLOCATOR_H
#define GST_SURFACE_ALLOCATOR_H

#include <gst/gst.h>
#include "display_type.h"
#include "gst_surface_memory.h"

G_BEGIN_DECLS

#define GST_TYPE_SURFACE_ALLOCATOR (gst_surface_allocator_get_type())
#define GST_SURFACE_ALLOCATOR(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_SURFACE_ALLOCATOR, GstSurfaceAllocator))
#define GST_SURFACE_ALLOCATOR_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_SURFACE_ALLOCATOR, GstSurfaceAllocatorClass))
#define GST_IS_SURFACE_ALLOCATOR(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_SURFACE_ALLOCATOR))
#define GST_IS_SURFACE_ALLOCATOR_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_SURFACE_ALLOCATOR))
#define GST_SURFACE_ALLOCATOR_CAST(obj) ((GstSurfaceAllocator*)(obj))

typedef struct _GstSurfaceAllocator GstSurfaceAllocator;
typedef struct _GstSurfaceAllocatorClass GstSurfaceAllocatorClass;

struct _GstSurfaceAllocator {
    GstAllocator parent;
    OHOS::sptr<OHOS::Surface> surface;
};

struct _GstSurfaceAllocatorClass {
    GstAllocatorClass parent;
};

GType gst_surface_allocator_get_type(void);

GstSurfaceAllocator *gst_surface_allocator_new(void);

gboolean gst_surface_allocator_set_surface(GstSurfaceAllocator *allocator, OHOS::sptr<OHOS::Surface> surface);

GstSurfaceMemory *gst_surface_allocator_alloc(GstSurfaceAllocator *allocator,
    gint width, gint height, PixelFormat format, gint usage);

void gst_surface_allocator_free(GstSurfaceAllocator *allocator, GstSurfaceMemory *memory);

G_END_DECLS

#endif
