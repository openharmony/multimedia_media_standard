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

#ifndef GST_SHMEM_ALLOCATOR_H
#define GST_SHMEM_ALLOCATOR_H

#include <gst/gst.h>
#include "gst_shmem_memory.h"
#include "avsharedmemorypool.h"

G_BEGIN_DECLS

#ifndef GST_API_EXPORT
#define GST_API_EXPORT __attribute__((visibility("default")))
#endif

#define GST_TYPE_SHMEM_ALLOCATOR (gst_shmem_allocator_get_type())
#define GST_SHMEM_ALLOCATOR(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_SHMEM_ALLOCATOR, GstShMemAllocator))
#define GST_SHMEM_ALLOCATOR_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_SHMEM_ALLOCATOR, GstShMemAllocatorClass))
#define GST_IS_SHMEM_ALLOCATOR(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_SHMEM_ALLOCATOR))
#define GST_IS_SHMEM_ALLOCATOR_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_SHMEM_ALLOCATOR))
#define GST_SHMEM_ALLOCATOR_CAST(obj) ((GstShMemAllocator*)(obj))

typedef struct _GstShMemAllocator GstShMemAllocator;
typedef struct _GstShMemAllocatorClass GstShMemAllocatorClass;

struct _GstShMemAllocator {
    GstAllocator parent;
    std::shared_ptr<OHOS::Media::AVSharedMemoryPool> avShmemPool;
};

struct _GstShMemAllocatorClass {
    GstAllocatorClass parent;
};

GST_API_EXPORT GType gst_shmem_allocator_get_type(void);

GST_API_EXPORT GstShMemAllocator *gst_shmem_allocator_new();

GST_API_EXPORT void gst_shmem_allocator_set_pool(GstShMemAllocator *allocator,
    std::shared_ptr<OHOS::Media::AVSharedMemoryPool> pool);

G_END_DECLS

#endif
