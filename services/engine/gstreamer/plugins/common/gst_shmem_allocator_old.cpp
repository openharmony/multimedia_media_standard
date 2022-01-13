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

#include "config.h"
#include "gst_shmem_allocator_old.h"
#include "gst_shmem_memory.h"
#include "media_log.h"
#include "media_errors.h"
#include "avsharedmemorybase.h"

#define gst_shmem_allocator_old_parent_class parent_class
G_DEFINE_TYPE(GstShMemAllocatorOld, gst_shmem_allocator_old, GST_TYPE_ALLOCATOR);

static const uint32_t ALIGN_BYTES = 4;

static GstMemory *gst_shmem_allocator_old_alloc(GstAllocator *allocator, gsize size, GstAllocationParams *params)
{
    g_return_val_if_fail(params != nullptr, nullptr);
    g_return_val_if_fail(allocator != nullptr, nullptr);

    /**
     * currently, only concern about the prefix.
     */
    g_return_val_if_fail((params->prefix & (ALIGN_BYTES - 1)) == 0, nullptr);
    g_return_val_if_fail((UINT64_MAX - params->prefix) >= size, nullptr);

    uint64_t allocSize = size + params->prefix;
    GST_INFO_OBJECT(allocator, "alloc memory, prefix: %" G_GSIZE_FORMAT, params->prefix);

    g_return_val_if_fail(allocSize < INT32_MAX, nullptr);

    auto avSharedMem = std::make_shared<OHOS::Media::AVSharedMemoryBase>(
        static_cast<int32_t>(allocSize), OHOS::Media::AVSharedMemory::FLAGS_READ_ONLY, "GstShMemAllocatorOld");
    g_return_val_if_fail(avSharedMem->Init() == OHOS::Media::MSERR_OK, nullptr);

    GstShMemMemory *mem = reinterpret_cast<GstShMemMemory *>(g_slice_alloc0(sizeof(GstShMemMemory)));
    g_return_val_if_fail(mem != nullptr, nullptr);
    gst_memory_init(GST_MEMORY_CAST(mem), GST_MEMORY_FLAG_NO_SHARE,
        allocator, nullptr, allocSize, 0, 0, size);

    mem->mem = avSharedMem;
    GST_INFO_OBJECT(allocator, "alloc memory for size: %" PRIu64 ", addr: 0x%06" PRIXPTR "",
        allocSize, FAKE_POINTER(avSharedMem->GetBase()));

    return GST_MEMORY_CAST(mem);
}

static void gst_shmem_allocator_old_free(GstAllocator *allocator, GstMemory *mem)
{
    g_return_if_fail(mem != nullptr && allocator != nullptr);
    g_return_if_fail(gst_is_shmem_memory(mem));

    GstShMemMemory *avSharedMem = reinterpret_cast<GstShMemMemory *>(mem);
    GST_INFO_OBJECT(allocator, "free memory for size: %" G_GSIZE_FORMAT ", addr: 0x%06" PRIXPTR "",
        mem->maxsize, FAKE_POINTER(avSharedMem->mem->GetBase()));
    avSharedMem->mem = nullptr;
    g_slice_free(GstShMemMemory, avSharedMem);
}

static gpointer gst_shmem_allocator_old_mem_map(GstMemory *mem, gsize maxsize, GstMapFlags flags)
{
    g_return_val_if_fail(mem != nullptr, nullptr);
    g_return_val_if_fail(gst_is_shmem_memory(mem), nullptr);

    GstShMemMemory *avSharedMem = reinterpret_cast<GstShMemMemory *>(mem);
    g_return_val_if_fail(avSharedMem->mem != nullptr, nullptr);

    GST_INFO("mem_map, maxsize: %" G_GSIZE_FORMAT ", size: %" G_GSIZE_FORMAT, mem->maxsize, mem->size);
    return avSharedMem->mem->GetBase() + (mem->maxsize - mem->size);
}

static void gst_shmem_allocator_old_mem_unmap(GstMemory *mem)
{
    (void)mem;
}

static void gst_shmem_allocator_old_init(GstShMemAllocatorOld *allocator)
{
    GstAllocator *bAllocator = GST_ALLOCATOR_CAST(allocator);
    g_return_if_fail(bAllocator != nullptr);

    GST_DEBUG_OBJECT(allocator, "init allocator 0x%06" PRIXPTR "", FAKE_POINTER(allocator));

    bAllocator->mem_type = gst_shmem_memory_type();
    bAllocator->mem_map = (GstMemoryMapFunction)gst_shmem_allocator_old_mem_map;
    bAllocator->mem_unmap = (GstMemoryUnmapFunction)gst_shmem_allocator_old_mem_unmap;
}

static void gst_shmem_allocator_old_finalize(GObject *obj)
{
    GstShMemAllocatorOld *allocator = GST_SHMEM_ALLOCATOR_OLD_CAST(obj);
    g_return_if_fail(allocator != nullptr);

    GST_DEBUG_OBJECT(allocator, "finalize allocator 0x%06" PRIXPTR "", FAKE_POINTER(allocator));
    G_OBJECT_CLASS(parent_class)->finalize(obj);
}

static void gst_shmem_allocator_old_class_init(GstShMemAllocatorOldClass *klass)
{
    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);
    g_return_if_fail(gobjectClass != nullptr);

    gobjectClass->finalize = gst_shmem_allocator_old_finalize;

    GstAllocatorClass *allocatorClass = GST_ALLOCATOR_CLASS(klass);
    g_return_if_fail(allocatorClass != nullptr);

    allocatorClass->alloc = gst_shmem_allocator_old_alloc;
    allocatorClass->free = gst_shmem_allocator_old_free;
}

GstAllocator *gst_shmem_allocator_old_new()
{
    GstAllocator *alloc = GST_ALLOCATOR_CAST(g_object_new(
        GST_TYPE_SHMEM_ALLOCATOR_OLD, "name", "AVShareMem::Allocator", nullptr));
    (void)gst_object_ref_sink(alloc);

    return alloc;
}
