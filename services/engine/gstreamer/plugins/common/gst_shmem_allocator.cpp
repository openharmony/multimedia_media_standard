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

#include "gst_shmem_allocator.h"
#include "media_log.h"
#include "media_errors.h"
#include "securec.h"

#define gst_shmem_allocator_parent_class parent_class
G_DEFINE_TYPE(GstShMemAllocator, gst_shmem_allocator, GST_TYPE_ALLOCATOR);

GST_DEBUG_CATEGORY_STATIC(gst_shmem_allocator_debug_category);
#define GST_CAT_DEFAULT gst_shmem_allocator_debug_category

static constexpr uint32_t ALIGN_BYTES = 4;

GstShMemAllocator *gst_shmem_allocator_new()
{
    GstShMemAllocator *alloc = GST_SHMEM_ALLOCATOR_CAST(g_object_new(
        GST_TYPE_SHMEM_ALLOCATOR, "name", "ShMemAllocator", nullptr));
    (void)gst_object_ref_sink(alloc);

    return alloc;
}

void gst_shmem_allocator_set_pool(GstShMemAllocator *allocator,
                                  std::shared_ptr<OHOS::Media::AVSharedMemoryPool> pool)
{
    g_return_if_fail(allocator != nullptr && pool != nullptr);
    allocator->avShmemPool = pool;
}

GstMemory *gst_shmem_allocator_alloc(GstAllocator *allocator, gsize size, GstAllocationParams *params)
{
    g_return_val_if_fail(allocator != nullptr && params != nullptr, nullptr);
    GstShMemAllocator *sAlloctor = GST_SHMEM_ALLOCATOR_CAST(allocator);
    g_return_val_if_fail(sAlloctor != nullptr && sAlloctor->avShmemPool != nullptr, nullptr);
    g_return_val_if_fail((params->prefix & (ALIGN_BYTES - 1)) == 0, nullptr);
    g_return_val_if_fail((UINT64_MAX - params->prefix) >= size, nullptr);

    uint64_t allocSize = size + params->prefix;
    g_return_val_if_fail(allocSize < INT32_MAX, nullptr);

    std::shared_ptr<OHOS::Media::AVSharedMemory> shmem = sAlloctor->avShmemPool->AcquireMemory(allocSize);
    if (shmem == nullptr) {
        GST_LOG("no memory");
        return nullptr;
    }

    GstShMemMemory *memory = reinterpret_cast<GstShMemMemory *>(g_slice_alloc0(sizeof(GstShMemMemory)));
    g_return_val_if_fail(memory != nullptr, nullptr);

    gst_memory_init(GST_MEMORY_CAST(memory), (GstMemoryFlags)0, allocator,
        nullptr, allocSize, 0, 0, size);

    memory->mem = shmem;
    GST_LOG("alloc memory from %s for size: %" PRIu64 ", gstmemory: 0x%06" PRIXPTR "",
        sAlloctor->avShmemPool->GetName().c_str(), allocSize, FAKE_POINTER(memory));

    return GST_MEMORY_CAST(memory);
}

void gst_shmem_allocator_free(GstAllocator *allocator, GstMemory *memory)
{
    g_return_if_fail(memory != nullptr && allocator != nullptr);
    g_return_if_fail(gst_is_shmem_memory(memory));

    GstShMemAllocator *sAlloctor = GST_SHMEM_ALLOCATOR_CAST(allocator);
    g_return_if_fail(sAlloctor != nullptr && sAlloctor->avShmemPool != nullptr);

    GstShMemMemory *avSharedMem = reinterpret_cast<GstShMemMemory *>(memory);
    GST_LOG("free memory to %s for size: %" G_GSIZE_FORMAT ", gstmemory: 0x%06" PRIXPTR ", recount: %ld",
        sAlloctor->avShmemPool->GetName().c_str(), memory->size, FAKE_POINTER(avSharedMem),
        avSharedMem->mem.use_count());

    // assign the nullptr will decrease the refcount, if the refcount is zero,
    // the memory will be released back to pool.
    avSharedMem->mem = nullptr;
    g_slice_free(GstShMemMemory, avSharedMem);
}

static gpointer gst_shmem_allocator_mem_map(GstMemory *mem, gsize maxsize, GstMapFlags flags)
{
    g_return_val_if_fail(mem != nullptr, nullptr);
    g_return_val_if_fail(gst_is_shmem_memory(mem), nullptr);

    GstShMemMemory *avSharedMem = reinterpret_cast<GstShMemMemory *>(mem);
    g_return_val_if_fail(avSharedMem->mem != nullptr, nullptr);

    return avSharedMem->mem->GetBase();
}

static void gst_shmem_allocator_mem_unmap(GstMemory *mem)
{
    (void)mem;
}

static GstMemory *gst_shmem_allocator_mem_copy(GstShMemMemory *mem, gssize offset, gssize size)
{
    g_return_val_if_fail(mem != nullptr && mem->mem != nullptr, nullptr);
    g_return_val_if_fail(mem->mem->GetBase() != nullptr, nullptr);

    gssize realOffset = 0;
    if (((gint64)mem->parent.offset + offset) > INT32_MAX) {
        GST_ERROR("invalid offset");
        return nullptr;
    } else {
        realOffset = mem->parent.offset + offset;
    }
    g_return_val_if_fail(realOffset >= 0, nullptr);

    if (size == -1) {
        if (((gint64)mem->parent.size - offset) > INT32_MAX) {
            GST_ERROR("invalid size");
            return nullptr;
        } else {
            size = mem->parent.size - offset;
        }
    }
    g_return_val_if_fail(size > 0, nullptr);

    if ((UINT32_MAX - size) <= realOffset) {
        GST_ERROR("invalid limit");
        return nullptr;
    }
    gsize realLimit = static_cast<gsize>(size) + static_cast<gsize>(realOffset);
    g_return_val_if_fail(realLimit <= static_cast<gsize>(mem->mem->GetSize()), nullptr);

    GstMemory *copy = gst_allocator_alloc(nullptr, static_cast<gsize>(size), nullptr);
    g_return_val_if_fail(copy != nullptr, nullptr);

    GstMapInfo info = GST_MAP_INFO_INIT;
    if (!gst_memory_map(copy, &info, GST_MAP_READ)) {
        gst_memory_unref(copy);
        GST_ERROR("map failed");
        return nullptr;
    }

    uint8_t *src = mem->mem->GetBase() + realOffset;
    errno_t rc = memcpy_s(info.data, info.size, src, static_cast<size_t>(size));
    if (rc != EOK) {
        GST_ERROR("memcpy failed");
        gst_memory_unmap(copy, &info);
        gst_memory_unref(copy);
        return nullptr;
    }

    gst_memory_unmap(copy, &info);
    GST_LOG("copy memory: 0x%06" PRIXPTR " for size: %" G_GSSIZE_FORMAT ", offset: %" G_GSSIZE_FORMAT
        ", gstmemory: 0x%06" PRIXPTR, FAKE_POINTER(mem), size, offset, FAKE_POINTER(copy));

    return copy;
}

static GstShMemMemory *gst_shmem_allocator_mem_share(GstMemory *mem, gssize offset, gssize size)
{
    g_return_val_if_fail(mem != nullptr, nullptr);
    g_return_val_if_fail(offset >= 0 && static_cast<gsize>(offset) < mem->size, nullptr);
    GstShMemMemory *shmem = reinterpret_cast<GstShMemMemory *>(mem);
    g_return_val_if_fail(shmem->mem != nullptr, nullptr);

    GstMemory *parent = mem->parent;
    if (parent == nullptr) {
        parent = GST_MEMORY_CAST(mem);
    }
    if (size == -1) {
        size = mem->size - offset;
    }

    GstShMemMemory *sub = reinterpret_cast<GstShMemMemory *>(g_slice_alloc0(sizeof(GstShMemMemory)));
    g_return_val_if_fail(sub != nullptr, nullptr);

    GstMemoryFlags flags = static_cast<GstMemoryFlags>(GST_MEMORY_FLAGS(parent) | GST_MEMORY_FLAG_READONLY);
    gst_memory_init(GST_MEMORY_CAST(sub), flags, mem->allocator, parent, mem->maxsize,
        mem->align, mem->offset + offset, size);
    sub->mem = shmem->mem;

    GST_LOG("share memory 0x%06" PRIXPTR " for size: %" G_GSSIZE_FORMAT ", offset: %" G_GSSIZE_FORMAT
            ", addr: 0x%06" PRIXPTR ", refcount: %ld",
            FAKE_POINTER(mem), size, offset, FAKE_POINTER(sub), sub->mem.use_count());

    return sub;
}

static gboolean gst_shmem_allocator_is_span(GstShMemMemory *mem1, GstShMemMemory *mem2, gsize *offset)
{
    g_return_val_if_fail(mem1 != nullptr && mem1->mem != nullptr, FALSE);
    g_return_val_if_fail(mem2 != nullptr && mem2->mem != nullptr, FALSE);

    if (mem1->mem != mem2->mem) {
        return FALSE;
    }

    if (offset != nullptr) {
        GstShMemMemory *parent = reinterpret_cast<GstShMemMemory *>(mem1->parent.parent);
        if (parent != nullptr) {
            *offset = mem1->parent.offset - parent->parent.offset;
        }
    }

    return mem1->mem->GetBase() + mem1->parent.offset + mem1->parent.size ==
        mem2->mem->GetBase() + mem2->parent.offset;
}

static void gst_shmem_allocator_init(GstShMemAllocator *allocator)
{
    GstAllocator *bAllocator = GST_ALLOCATOR_CAST(allocator);
    g_return_if_fail(bAllocator != nullptr);

    GST_DEBUG_OBJECT(allocator, "init allocator 0x%06" PRIXPTR "", FAKE_POINTER(allocator));

    bAllocator->mem_type = GST_SHMEM_MEMORY_TYPE;
    bAllocator->mem_map = (GstMemoryMapFunction)gst_shmem_allocator_mem_map;
    bAllocator->mem_unmap = (GstMemoryUnmapFunction)gst_shmem_allocator_mem_unmap;
    bAllocator->mem_copy = (GstMemoryCopyFunction)gst_shmem_allocator_mem_copy;
    bAllocator->mem_share = (GstMemoryShareFunction)gst_shmem_allocator_mem_share;
    bAllocator->mem_is_span = (GstMemoryIsSpanFunction)gst_shmem_allocator_is_span;
}

static void gst_shmem_allocator_finalize(GObject *obj)
{
    GstShMemAllocator *allocator = GST_SHMEM_ALLOCATOR_CAST(obj);
    g_return_if_fail(allocator != nullptr);

    GST_DEBUG_OBJECT(allocator, "finalize allocator 0x%06" PRIXPTR "", FAKE_POINTER(allocator));
    allocator->avShmemPool = nullptr;
    G_OBJECT_CLASS(parent_class)->finalize(obj);
}

static void gst_shmem_allocator_class_init(GstShMemAllocatorClass *klass)
{
    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);
    g_return_if_fail(gobjectClass != nullptr);

    gobjectClass->finalize = gst_shmem_allocator_finalize;

    GstAllocatorClass *allocatorClass = GST_ALLOCATOR_CLASS(klass);
    g_return_if_fail(allocatorClass != nullptr);

    allocatorClass->alloc = gst_shmem_allocator_alloc;
    allocatorClass->free = gst_shmem_allocator_free;

    GST_DEBUG_CATEGORY_INIT(gst_shmem_allocator_debug_category, "shmemalloc", 0, "shmemalloc class");
}
