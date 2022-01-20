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

#ifndef GST_SHMEM_ALLOCATOR_OLD_H
#define GST_SHMEM_ALLOCATOR_OLD_H

#include <gst/gst.h>

#ifndef GST_API_EXPORT
#define GST_API_EXPORT __attribute__((visibility("default")))
#endif

G_BEGIN_DECLS

#define GST_TYPE_SHMEM_ALLOCATOR_OLD (gst_shmem_allocator_old_get_type())
#define GST_SHMEM_ALLOCATOR_OLD(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_SHMEM_ALLOCATOR_OLD, GstShMemAllocatorOld))
#define GST_SHMEM_ALLOCATOR_OLD_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_SHMEM_ALLOCATOR_OLD, GstShMemAllocatorOldClass))
#define GST_IS_SHMEM_ALLOCATOR_OLD(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_SHMEM_ALLOCATOR_OLD))
#define GST_IS_SHMEM_ALLOCATOR_OLD_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_SHMEM_ALLOCATOR_OLD))
#define GST_SHMEM_ALLOCATOR_OLD_CAST(obj) ((GstShMemAllocatorOld*)(obj))

typedef struct _GstShMemAllocatorOld GstShMemAllocatorOld;
typedef struct _GstShMemAllocatorOldClass GstShMemAllocatorOldClass;

struct _GstShMemAllocatorOld {
    GstAllocator parent;
};

struct _GstShMemAllocatorOldClass {
    GstAllocatorClass parent;
};

GST_API_EXPORT
GType gst_shmem_allocator_old_get_type(void);

GST_API_EXPORT
GstAllocator *gst_shmem_allocator_old_new();

G_END_DECLS

#endif
