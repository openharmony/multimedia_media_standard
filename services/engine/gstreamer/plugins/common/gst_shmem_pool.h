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

#ifndef GST_SHMEM_POOL_H
#define GST_SHMEM_POOL_H

#include <gst/gst.h>
#include <gst/video/video-info.h>
#include "gst_shmem_allocator.h"

G_BEGIN_DECLS

#ifndef GST_API_EXPORT
#define GST_API_EXPORT __attribute__((visibility("default")))
#endif

#define GST_TYPE_SHMEM_POOL (gst_shmem_pool_get_type())
#define GST_SHMEM_POOL(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_SHMEM_POOL, GstShMemPool))
#define GST_SHMEM_POOL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_SHMEM_POOL, GstShMemPoolClass))
#define GST_IS_SHMEM_POOL(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_SHMEM_POOL))
#define GST_IS_SHMEM_POOL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_SHMEM_POOL))
#define GST_SHMEM_POOL_CAST(obj) ((GstShMemPool*)(obj))

typedef struct _GstShMemPool GstShMemPool;
typedef struct _GstShMemPoolClass GstShMemPoolClass;

struct _GstShMemPool {
    GstBufferPool basepool;
    gboolean started;
    GstShMemAllocator *allocator;
    GstAllocationParams params;
    guint size;
    guint minBuffers;
    guint maxBuffers;
    GMutex lock;
    GCond cond;
    gint curBuffers;
    GstVideoInfo info;
    gboolean addVideoMeta;
    gboolean end;
    gchar *debugName;
    std::shared_ptr<OHOS::Media::AVSharedMemoryPool> avshmempool;
};

struct _GstShMemPoolClass {
    GstBufferPoolClass basepool_class;
};

GST_API_EXPORT GType gst_shmem_pool_get_type(void);

GST_API_EXPORT GstShMemPool *gst_shmem_pool_new();

GST_API_EXPORT gboolean gst_shmem_pool_set_avshmempool(GstShMemPool *pool,
    const std::shared_ptr<OHOS::Media::AVSharedMemoryPool> &avshmempool);

G_END_DECLS

#endif