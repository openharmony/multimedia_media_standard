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

#ifndef GST_CONSUMER_SURFACE_POOL_H
#define GST_CONSUMER_SURFACE_POOL_H

#include <gst/gst.h>
#include <gst/video/gstvideopool.h>
#include "surface.h"

G_BEGIN_DECLS

#define GST_TYPE_CONSUMER_SURFACE_POOL (gst_consumer_surface_pool_get_type())
#define GST_CONSUMER_SURFACE_POOL(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_CONSUMER_SURFACE_POOL, GstConsumerSurfacePool))
#define GST_CONSUMER_SURFACE_POOL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_CONSUMER_SURFACE_POOL, GstConsumerSurfacePoolClass))
#define GST_IS_CONSUMER_SURFACE_POOL(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_CONSUMER_SURFACE_POOL))
#define GST_IS_CONSUMER_SURFACE_POOL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_CONSUMER_SURFACE_POOL))
#define GST_CONSUMER_SURFACE_POOL_CAST(obj) ((GstConsumerSurfacePool*)(obj))

typedef struct _GstConsumerSurfacePool GstConsumerSurfacePool;
typedef struct _GstConsumerSurfacePoolClass GstConsumerSurfacePoolClass;
typedef struct _GstConsumerSurfacePoolPrivate GstConsumerSurfacePoolPrivate;

/* 1. The acquire and release functions are overloaded and the queue is no longer used.
 * 2. GstConsumerSurfacePool is mainly used to maintain the surface buffer.
 */
struct _GstConsumerSurfacePool {
    GstVideoBufferPool basepool;

    /* < private > */
    GstConsumerSurfacePoolPrivate *priv;
};

struct _GstConsumerSurfacePoolClass {
    GstVideoBufferPoolClass parent_class;
};

GType gst_consumer_surface_pool_get_type(void);

GstBufferPool *gst_consumer_surface_pool_new();

GstCaps *gst_consumer_surface_pool_get_caps(GstConsumerSurfacePool *pool);

void gst_consumer_surface_pool_set_surface(GstBufferPool *surfacePool,
    OHOS::sptr<OHOS::Surface> &consumerSurface);

G_END_DECLS

#endif
