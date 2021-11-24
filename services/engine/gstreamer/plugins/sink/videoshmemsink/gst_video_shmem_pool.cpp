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

#include "gst_video_shmem_pool.h"
#include "gst_shmem_allocator.h"

#define gst_video_shmem_pool_parent_class parent_class
G_DEFINE_TYPE (GstVideoShMemPool, gst_video_shmem_pool, GST_TYPE_VIDEO_BUFFER_POOL);

static const gchar **gst_video_shmem_pool_get_options (GstBufferPool *pool)
{
    (void)pool;
    static const gchar *options[] = { GST_BUFFER_POOL_OPTION_VIDEO_META, NULL };
    return options;
}

static gboolean gst_video_shmem_pool_set_config(GstBufferPool *pool, GstStructure *config)
{
    GstVideoShMemPool *vpool = GST_VIDEO_SHMEM_POOL_CAST(pool);
    g_return_val_if_fail(vpool != nullptr, FALSE);

    GstAllocator *allocator = nullptr;
    (void)gst_buffer_pool_config_get_allocator(config, &allocator, nullptr);
    if (!(allocator && GST_IS_SHMEM_ALLOCATOR(allocator))) {
        GST_WARNING_OBJECT(pool, "no valid allocator in pool");
        return FALSE;
    }

    return GST_BUFFER_POOL_CLASS(parent_class)->set_config(pool, config);
}

static void gst_video_shmem_pool_finalize(GObject *obj)
{
    g_return_if_fail(obj != nullptr);

    G_OBJECT_CLASS(parent_class)->finalize(obj);
}

static void gst_video_shmem_pool_class_init (GstVideoShMemPoolClass *klass)
{
    g_return_if_fail(klass != nullptr);
    GstBufferPoolClass *poolClass = GST_BUFFER_POOL_CLASS(klass);
    g_return_if_fail(poolClass != nullptr);
    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);
    g_return_if_fail(gobjectClass != nullptr);

    gobjectClass->finalize = gst_video_shmem_pool_finalize;
    poolClass->get_options = gst_video_shmem_pool_get_options;
    poolClass->set_config = gst_video_shmem_pool_set_config;
}

static void gst_video_shmem_pool_init (GstVideoShMemPool *pool)
{
    (void)pool;
}

GstBufferPool *gst_video_shmem_pool_new()
{
    GstBufferPool *pool = GST_BUFFER_POOL_CAST(g_object_new(
        GST_TYPE_VIDEO_SHMEM_POOL, "name", "VidShMemPool", nullptr));
    (void)gst_object_ref_sink(pool);

    return pool;
}
