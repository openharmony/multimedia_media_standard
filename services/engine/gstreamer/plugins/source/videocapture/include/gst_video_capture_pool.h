/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef GST_VIDEO_CAPTURE_POOL_H
#define GST_VIDEO_CAPTURE_POOL_H

#include <gst/gst.h>
#include "gst_consumer_surface_pool.h"

G_BEGIN_DECLS

#define GST_TYPE_VIDEO_CAPTURE_POOL (gst_video_capture_pool_get_type())
#define GST_VIDEO_CAPTURE_POOL(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_VIDEO_CAPTURE_POOL, GstVideoCapturePool))
#define GST_VIDEO_CAPTURE_POOL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_VIDEO_CAPTURE_POOL, GstVideoCapturePoolClass))
#define GST_IS_VIDEO_CAPTURE_POOL(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_VIDEO_CAPTURE_POOL))
#define GST_IS_VIDEO_CAPTURE_POOL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_VIDEO_CAPTURE_POOL))
#define GST_VIDEO_CAPTURE_POOL_CAST(obj) ((GstVideoCapturePool*)(obj))

struct GstVideoCapturePool {
    GstConsumerSurfacePool element;
};

struct GstVideoCapturePoolClass {
    GstConsumerSurfacePoolClass parent_class;
};

using GstVideoCapturePool = struct GstVideoCapturePool;
using GstVideoCapturePoolClass = struct GstVideoCapturePoolClass;

GType gst_video_capture_pool_get_type(void);

GstBufferPool *gst_video_capture_pool_new();

G_END_DECLS

#endif /* GST_VIDEO_CAPTURE_POOL_H */
