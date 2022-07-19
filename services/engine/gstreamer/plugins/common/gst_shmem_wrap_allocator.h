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

#ifndef GST_SHMEM_WRAP_ALLOCATOR_H
#define GST_SHMEM_WRAP_ALLOCATOR_H

#include <gst/gst.h>
#include "gst_shmem_memory.h"
#include "avsharedmemory.h"

G_BEGIN_DECLS

#define GST_TYPE_SHMEM_WRAP_ALLOCATOR (gst_shmem_wrap_allocator_get_type())
#define GST_SHMEM_WRAP_ALLOCATOR(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_SHMEM_WRAP_ALLOCATOR, GstShMemWrapAllocator))
#define GST_SHMEM_WRAP_ALLOCATOR_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_SHMEM_WRAP_ALLOCATOR, GstShMemWrapAllocatorClass))
#define GST_IS_SHMEM_WRAP_ALLOCATOR(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_SHMEM_WRAP_ALLOCATOR))
#define GST_IS_SHMEM_WRAP_ALLOCATOR_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_SHMEM_WRAP_ALLOCATOR))
#define GST_SHMEM_WRAP_ALLOCATOR_CAST(obj) ((GstShMemWrapAllocator*)(obj))

using GstShMemWrapAllocator = struct _GstShMemWrapAllocator;
using GstShMemWrapAllocatorClass = struct _GstShMemWrapAllocatorClass;

struct _GstShMemWrapAllocator {
    GstAllocator parent;
};

struct _GstShMemWrapAllocatorClass {
    GstAllocatorClass parent;
};

GType gst_shmem_wrap_allocator_get_type(void);

__attribute__((visibility("default"))) GstShMemWrapAllocator *gst_shmem_wrap_allocator_new(void);
__attribute__((visibility("default"))) GstMemory *gst_shmem_wrap(GstAllocator *allocator,
    std::shared_ptr<OHOS::Media::AVSharedMemory> shmem);

G_END_DECLS

#endif // GST_SHMEM_WRAP_ALLOCATOR_H