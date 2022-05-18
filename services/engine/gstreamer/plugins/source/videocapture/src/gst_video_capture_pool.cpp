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

#include "gst_video_capture_pool.h"
#include <gst/gst.h>
#include "media_errors.h"

using namespace OHOS::Media;
#define gst_video_capture_pool_parent_class parent_class

GST_DEBUG_CATEGORY_STATIC(gst_video_capture_pool_debug_category);
#define GST_CAT_DEFAULT gst_video_capture_pool_debug_category

G_DEFINE_TYPE(GstVideoCapturePool, gst_video_capture_pool, GST_TYPE_CONSUMER_SURFACE_POOL);

static void gst_video_capture_pool_class_init(GstVideoCapturePoolClass *klass)
{
    g_return_if_fail(klass != nullptr);

    GST_DEBUG_CATEGORY_INIT(gst_video_capture_pool_debug_category, "videocapturepool", 0,
        "video capture pool base class");
}

static void gst_video_capture_pool_init(GstVideoCapturePool *pool)
{
    (void)pool;
}

GstBufferPool *gst_video_capture_pool_new()
{
    GstBufferPool *pool = GST_BUFFER_POOL_CAST(g_object_new(
        GST_TYPE_VIDEO_CAPTURE_POOL, "name", "video_capture_pool", nullptr));
    (void)gst_object_ref_sink(pool);

    return pool;
}
