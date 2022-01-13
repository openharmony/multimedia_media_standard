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

#ifndef GST_SURFACE_POOL_H
#define GST_SURFACE_POOL_H

#include <gst/gst.h>
#include <gst/video/video-info.h>
#include "surface.h"
#include "gst_surface_allocator.h"
#include "display_type.h"

G_BEGIN_DECLS

#define GST_TYPE_SURFACE_POOL (gst_surface_pool_get_type())
#define GST_SURFACE_POOL(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_SURFACE_POOL, GstSurfacePool))
#define GST_SURFACE_POOL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_SURFACE_POOL, GstSurfacePoolClass))
#define GST_IS_SURFACE_POOL(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_SURFACE_POOL))
#define GST_IS_SURFACE_POOL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_SURFACE_POOL))

typedef struct _GstSurfacePool GstSurfacePool;
typedef struct _GstSurfacePoolClass GstSurfacePoolClass;

struct _GstSurfacePool {
    GstBufferPool basepool;
    OHOS::sptr<OHOS::Surface> surface;
    gboolean started;
    GstSurfaceAllocator *allocator;
    GstAllocationParams params;
    PixelFormat format;
    GstVideoInfo info;
    guint minBuffers;
    guint maxBuffers;
    guint waittime;
    GMutex lock;
    GCond cond;
    GList *preAllocated;
    guint freeBufCnt;
    gint usage;
};

struct _GstSurfacePoolClass {
    GstBufferPoolClass basepool_class;
};

GType gst_surface_pool_get_type(void);

GstSurfacePool *gst_surface_pool_new(void);

GST_API gboolean gst_surface_pool_set_surface(GstSurfacePool *pool,
    OHOS::sptr<OHOS::Surface> surface, guint waittime);

G_END_DECLS

#endif