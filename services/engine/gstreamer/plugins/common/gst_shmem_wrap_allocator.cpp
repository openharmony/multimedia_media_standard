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

#include "gst_shmem_wrap_allocator.h"
#include "media_log.h"

#define gst_shmem_wrap_allocator_parent_class parent_class
G_DEFINE_TYPE(GstShMemWrapAllocator, gst_shmem_wrap_allocator, GST_TYPE_ALLOCATOR);

GstShMemWrapAllocator *gst_shmem_wrap_allocator_new(void)
{
    GstShMemWrapAllocator *alloc = GST_SHMEM_WRAP_ALLOCATOR_CAST(g_object_new(
        GST_TYPE_SHMEM_WRAP_ALLOCATOR, "name", "ShMemWrapAllocator", nullptr));
    (void)gst_object_ref_sink(alloc);

    return alloc;
}

GstMemory *gst_shmem_wrap(GstAllocator *allocator, std::shared_ptr<OHOS::Media::AVSharedMemory> shmem)
{
    g_return_val_if_fail(allocator != nullptr, nullptr);
    g_return_val_if_fail(shmem != nullptr, nullptr);

    GstShMemMemory *memory = reinterpret_cast<GstShMemMemory *>(g_slice_alloc0(sizeof(GstShMemMemory)));
    g_return_val_if_fail(memory != nullptr, nullptr);

    gst_memory_init(GST_MEMORY_CAST(memory), (GstMemoryFlags)0, allocator,
        nullptr, shmem->GetSize(), 0, 0, shmem->GetSize());

    memory->mem = shmem;
    GST_DEBUG("alloc memory for size: %" PRIu64 ", addr: 0x%06" PRIXPTR "",
        static_cast<uint64_t>(shmem->GetSize()), FAKE_POINTER(shmem->GetBase()));

    return GST_MEMORY_CAST(memory);
}

static GstMemory *gst_shmem_wrap_allocator_alloc(GstAllocator *allocator, gsize size, GstAllocationParams *params)
{
    (void)allocator;
    (void)size;
    (void)params;
    return nullptr;
}

static void gst_shmem_wrap_allocator_free(GstAllocator *allocator, GstMemory *memory)
{
    g_return_if_fail(memory != nullptr && allocator != nullptr);
    g_return_if_fail(gst_is_shmem_memory(memory));

    GstShMemMemory *avSharedMem = reinterpret_cast<GstShMemMemory *>(memory);
    GST_DEBUG("free memory for size: %" G_GSIZE_FORMAT ", addr: 0x%06" PRIXPTR "",
        memory->maxsize, FAKE_POINTER(avSharedMem->mem->GetBase()));

    avSharedMem->mem = nullptr;
    g_slice_free(GstShMemMemory, avSharedMem);
}

static gpointer gst_shmem_wrap_allocator_mem_map(GstMemory *mem, gsize maxsize, GstMapFlags flags)
{
    (void)maxsize;
    (void)flags;
    g_return_val_if_fail(mem != nullptr, nullptr);
    g_return_val_if_fail(gst_is_shmem_memory(mem), nullptr);

    GstShMemMemory *avSharedMem = reinterpret_cast<GstShMemMemory *>(mem);
    g_return_val_if_fail(avSharedMem->mem != nullptr, nullptr);

    GST_INFO("mem_map, maxsize: %" G_GSIZE_FORMAT ", size: %" G_GSIZE_FORMAT, mem->maxsize, mem->size);
    
    // Only need return GetBase, not GetBase + offset
    return avSharedMem->mem->GetBase();
}

static void gst_shmem_wrap_allocator_mem_unmap(GstMemory *mem)
{
    (void)mem;
}

static GstMemory *gst_shmem_wrap_allocator_mem_share(GstMemory *mem, gssize offset, gssize size)
{
    g_return_val_if_fail(mem != nullptr, nullptr);
    g_return_val_if_fail(offset >= 0 && static_cast<gsize>(offset) < mem->size, nullptr);
    GstShMemMemory *sub = nullptr;
    GstMemory *parent = nullptr;

    /* find the real parent */
    if ((parent = mem->parent) == NULL) {
        parent = (GstMemory *)mem;
    }
    if (size == -1) {
        size = mem->size - offset;
    }

    sub = g_slice_new0(GstShMemMemory);
    g_return_val_if_fail(sub != nullptr, nullptr);
    /* the shared memory is always readonly */
    gst_memory_init(
        GST_MEMORY_CAST(sub),
        (GstMemoryFlags)(GST_MINI_OBJECT_FLAGS(parent) | GST_MINI_OBJECT_FLAG_LOCK_READONLY),
        mem->allocator,
        GST_MEMORY_CAST(parent),
        mem->maxsize,
        mem->align,
        mem->offset + offset,
        size);
    
    sub->mem = reinterpret_cast<GstShMemMemory *>(mem)->mem;
    return GST_MEMORY_CAST(sub);
}

static void gst_shmem_wrap_allocator_init(GstShMemWrapAllocator *allocator)
{
    GstAllocator *bAllocator = GST_ALLOCATOR_CAST(allocator);
    g_return_if_fail(bAllocator != nullptr);

    GST_DEBUG_OBJECT(allocator, "init allocator 0x%06" PRIXPTR "", FAKE_POINTER(allocator));

    bAllocator->mem_type = GST_SHMEM_MEMORY_TYPE;
    bAllocator->mem_map = (GstMemoryMapFunction)gst_shmem_wrap_allocator_mem_map;
    bAllocator->mem_unmap = (GstMemoryUnmapFunction)gst_shmem_wrap_allocator_mem_unmap;
    bAllocator->mem_share = (GstMemoryShareFunction)gst_shmem_wrap_allocator_mem_share;
}

static void gst_shmem_wrap_allocator_finalize(GObject *obj)
{
    GstShMemWrapAllocator *allocator = GST_SHMEM_WRAP_ALLOCATOR_CAST(obj);
    g_return_if_fail(allocator != nullptr);

    GST_DEBUG_OBJECT(allocator, "finalize allocator 0x%06" PRIXPTR "", FAKE_POINTER(allocator));
    G_OBJECT_CLASS(parent_class)->finalize(obj);
}

static void gst_shmem_wrap_allocator_class_init(GstShMemWrapAllocatorClass *klass)
{
    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);
    g_return_if_fail(gobjectClass != nullptr);

    gobjectClass->finalize = gst_shmem_wrap_allocator_finalize;

    GstAllocatorClass *allocatorClass = GST_ALLOCATOR_CLASS(klass);
    g_return_if_fail(allocatorClass != nullptr);

    allocatorClass->alloc = gst_shmem_wrap_allocator_alloc;
    allocatorClass->free = gst_shmem_wrap_allocator_free;
}