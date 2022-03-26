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

#include "gst_shared_mem_sink.h"
#include <cinttypes>
#include "gst_shmem_pool.h"
#include "avsharedmemorypool.h"
#include "media_log.h"
#include "media_errors.h"
#include "buffer_type_meta.h"

namespace {
    constexpr guint32 DEFAULT_PROP_MEM_SIZE = 0; // 0 is meanless
    constexpr guint32 DEFAULT_PROP_MEM_PREFIX_SIZE = 0;
    constexpr gboolean DEFAULT_PROP_REMOTE_REFCOUNT = FALSE;
}

struct _GstSharedMemSinkPrivate {
    guint mem_size;
    guint mem_prefix_size;
    gboolean enable_remote_ref_count;
    GstShMemPool *pool;
    GstShMemAllocator *allocator;
    GstAllocationParams alloc_params;
    std::shared_ptr<OHOS::Media::AVSharedMemoryPool> av_shmem_pool;
    gboolean set_pool_for_allocator;
    GMutex mutex;
    GCond cond;
    gboolean unlock;
    gboolean flushing;
};

enum {
    PROP_0,
    PROP_MEM_SIZE,
    PROP_MEM_PREFIX_SIZE,
    PROP_ENABLE_REMOTE_REFCOUNT,
};

static void gst_shared_mem_sink_dispose(GObject *obj);
static void gst_shared_mem_sink_finalize(GObject *object);
static void gst_shared_mem_sink_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_shared_mem_sink_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static gboolean gst_shared_mem_sink_do_propose_allocation(GstMemSink *memsink, GstQuery *query);
static GstFlowReturn gst_shared_mem_sink_do_stream_render(GstMemSink *memsink, GstBuffer **buffer);
static gboolean gst_shared_mem_sink_unlock_start(GstBaseSink *bsink);
static gboolean gst_shared_mem_sink_unlock_stop(GstBaseSink *bsink);
static gboolean gst_shared_mem_sink_start(GstBaseSink *bsink);
static gboolean gst_shared_mem_sink_stop(GstBaseSink *bsink);

#define gst_shared_mem_sink_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstSharedMemSink, gst_shared_mem_sink,
                        GST_TYPE_MEM_SINK, G_ADD_PRIVATE(GstSharedMemSink));

GST_DEBUG_CATEGORY_STATIC(gst_shmem_sink_debug_category);
#define GST_CAT_DEFAULT gst_shmem_sink_debug_category

static void gst_shared_mem_sink_class_init(GstSharedMemSinkClass *klass)
{
    g_return_if_fail(klass != nullptr);

    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstMemSinkClass *mem_sink_class = GST_MEM_SINK_CLASS(klass);
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
    GstBaseSinkClass *base_sink_class = GST_BASE_SINK_CLASS(klass);

    gobject_class->dispose = gst_shared_mem_sink_dispose;
    gobject_class->finalize = gst_shared_mem_sink_finalize;
    gobject_class->set_property = gst_shared_mem_sink_set_property;
    gobject_class->get_property = gst_shared_mem_sink_get_property;

    g_object_class_install_property(gobject_class, PROP_MEM_SIZE,
        g_param_spec_uint("mem-size", "Memory Size",
            "Allocate the memory with required length (in bytes)",
            0, G_MAXUINT, DEFAULT_PROP_MEM_SIZE,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_MEM_PREFIX_SIZE,
        g_param_spec_uint("mem-prefix-size", "Memory Prefix Size",
            "Allocate the memory with required length's prefix (in bytes)",
            0, G_MAXUINT, DEFAULT_PROP_MEM_PREFIX_SIZE,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_ENABLE_REMOTE_REFCOUNT,
        g_param_spec_boolean("enable-remote-refcount", "Enable Remote RefCount",
            "Enable the remote refcount at the allocated memory", DEFAULT_PROP_REMOTE_REFCOUNT,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    gst_element_class_set_static_metadata(element_class,
        "ShMemSink", "Sink/Generic",
        "Output to multi-process shared memory and allow the application to get access to the shared memory",
        "OpenHarmony");

    base_sink_class->unlock = gst_shared_mem_sink_unlock_start;
    base_sink_class->unlock_stop = gst_shared_mem_sink_unlock_stop;
    base_sink_class->start = gst_shared_mem_sink_start;
    base_sink_class->stop = gst_shared_mem_sink_stop;

    mem_sink_class->do_propose_allocation = gst_shared_mem_sink_do_propose_allocation;
    mem_sink_class->do_stream_render = gst_shared_mem_sink_do_stream_render;

    GST_DEBUG_CATEGORY_INIT(gst_shmem_sink_debug_category, "shmemsink", 0, "shmemsink class");
}

static void gst_shared_mem_sink_init(GstSharedMemSink *sink)
{
    g_return_if_fail(sink != nullptr);

    auto priv = reinterpret_cast<GstSharedMemSinkPrivate *>(gst_shared_mem_sink_get_instance_private(sink));
    g_return_if_fail(priv != nullptr);
    sink->priv = priv;
    priv->mem_size = DEFAULT_PROP_MEM_SIZE;
    priv->mem_prefix_size = DEFAULT_PROP_MEM_PREFIX_SIZE;
    priv->enable_remote_ref_count = DEFAULT_PROP_REMOTE_REFCOUNT;
    gst_allocation_params_init(&priv->alloc_params);
    priv->allocator = gst_shmem_allocator_new();
    priv->set_pool_for_allocator = FALSE;
    g_mutex_init(&priv->mutex);
    g_cond_init(&priv->cond);
    priv->unlock = FALSE;
    priv->flushing = FALSE;
}

static void gst_shared_mem_sink_dispose(GObject *obj)
{
    g_return_if_fail(obj != nullptr);

    GstSharedMemSink *shmem_sink = GST_SHARED_MEM_SINK_CAST(obj);
    GstSharedMemSinkPrivate *priv = shmem_sink->priv;
    g_return_if_fail(priv != nullptr);

    GST_OBJECT_LOCK(shmem_sink);
    if (priv->allocator) {
        gst_object_unref(priv->allocator);
        priv->allocator = nullptr;
    }
    if (priv->pool != nullptr) {
        gst_object_unref(priv->pool);
        priv->pool = nullptr;
    }
    priv->av_shmem_pool = nullptr;
    GST_OBJECT_UNLOCK(shmem_sink);

    G_OBJECT_CLASS(parent_class)->dispose(obj);
}

static void gst_shared_mem_sink_finalize(GObject *obj)
{
    g_return_if_fail(obj != nullptr);
    GstSharedMemSink *shmem_sink = GST_SHARED_MEM_SINK_CAST(obj);
    GstSharedMemSinkPrivate *priv = shmem_sink->priv;
    g_return_if_fail(priv != nullptr);

    g_mutex_clear(&priv->mutex);
    g_cond_clear(&priv->cond);

    G_OBJECT_CLASS(parent_class)->finalize(obj);
}

static void gst_shared_mem_sink_set_property(GObject *object, guint propId, const GValue *value, GParamSpec *pspec)
{
    g_return_if_fail(object != nullptr && value != nullptr);

    GstSharedMemSink *shmem_sink = GST_SHARED_MEM_SINK_CAST(object);
    GstSharedMemSinkPrivate *priv = shmem_sink->priv;
    g_return_if_fail(priv != nullptr);

    switch (propId) {
        case PROP_MEM_SIZE: {
            GST_OBJECT_LOCK(shmem_sink);
            priv->mem_size = g_value_get_uint(value);
            GST_DEBUG_OBJECT(shmem_sink, "memory size: %u", priv->mem_size);
            GST_OBJECT_UNLOCK(shmem_sink);
            break;
        }
        case PROP_MEM_PREFIX_SIZE: {
            GST_OBJECT_LOCK(shmem_sink);
            priv->mem_prefix_size = g_value_get_uint(value);
            priv->alloc_params.prefix = priv->mem_prefix_size;
            GST_DEBUG_OBJECT(shmem_sink, "memory prefix size: %u", priv->mem_prefix_size);
            GST_OBJECT_UNLOCK(shmem_sink);
            break;
        }
        case PROP_ENABLE_REMOTE_REFCOUNT: {
            GST_OBJECT_LOCK(shmem_sink);
            priv->enable_remote_ref_count = g_value_get_boolean(value);
            GST_DEBUG_OBJECT(shmem_sink, "enable remote refcount: %d", priv->enable_remote_ref_count);
            GST_OBJECT_UNLOCK(shmem_sink);
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
            break;
    }
}

static void gst_shared_mem_sink_get_property(GObject *object, guint propId, GValue *value, GParamSpec *pspec)
{
    g_return_if_fail(object != nullptr);

    GstSharedMemSink *shmem_sink = GST_SHARED_MEM_SINK_CAST(object);
    GstSharedMemSinkPrivate *priv = shmem_sink->priv;
    g_return_if_fail(priv != nullptr);

    switch (propId) {
        case PROP_MEM_SIZE: {
            GST_OBJECT_LOCK(shmem_sink);
            g_value_set_uint(value, priv->mem_size);
            GST_OBJECT_UNLOCK(shmem_sink);
            break;
        }
        case PROP_MEM_PREFIX_SIZE: {
            GST_OBJECT_LOCK(shmem_sink);
            g_value_set_uint(value, priv->mem_prefix_size);
            GST_OBJECT_UNLOCK(shmem_sink);
            break;
        }
        case PROP_ENABLE_REMOTE_REFCOUNT: {
            GST_OBJECT_LOCK(shmem_sink);
            g_value_set_boolean(value, priv->enable_remote_ref_count);
            GST_OBJECT_UNLOCK(shmem_sink);
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
            break;
    }
}

static gboolean gst_shared_mem_sink_unlock_start(GstBaseSink *bsink)
{
    g_return_val_if_fail(bsink != nullptr, FALSE);
    GstSharedMemSink *shmem_sink = GST_SHARED_MEM_SINK_CAST(bsink);
    GstSharedMemSinkPrivate *priv = shmem_sink->priv;
    g_return_val_if_fail(priv != nullptr, FALSE);

    GST_INFO_OBJECT(shmem_sink, "we are unlock start");

    g_mutex_lock(&priv->mutex);
    priv->unlock = TRUE;
    g_cond_signal(&priv->cond);
    g_mutex_unlock(&priv->mutex);

    return TRUE;
}

static gboolean gst_shared_mem_sink_unlock_stop(GstBaseSink *bsink)
{
    g_return_val_if_fail(bsink != nullptr, FALSE);
    GstSharedMemSink *shmem_sink = GST_SHARED_MEM_SINK_CAST(bsink);
    GstSharedMemSinkPrivate *priv = shmem_sink->priv;
    g_return_val_if_fail(priv != nullptr, FALSE);

    GST_INFO_OBJECT(shmem_sink, "we are unlock stop");

    g_mutex_lock(&priv->mutex);
    priv->unlock = FALSE;
    g_cond_signal(&priv->cond);
    g_mutex_unlock(&priv->mutex);

    return TRUE;
}

static gboolean gst_shared_mem_sink_start(GstBaseSink *bsink)
{
    g_return_val_if_fail(bsink != nullptr, FALSE);
    GstSharedMemSink *shmem_sink = GST_SHARED_MEM_SINK_CAST(bsink);
    GstSharedMemSinkPrivate *priv = shmem_sink->priv;
    g_return_val_if_fail(priv != nullptr, FALSE);

    GST_INFO_OBJECT(shmem_sink, "we are start");

    g_mutex_lock(&priv->mutex);
    priv->flushing = FALSE;
    g_mutex_unlock(&priv->mutex);

    return GST_BASE_SINK_CLASS(parent_class)->start(bsink);
}

static gboolean gst_shared_mem_sink_stop(GstBaseSink *bsink)
{
    g_return_val_if_fail(bsink != nullptr, FALSE);
    GstSharedMemSink *shmem_sink = GST_SHARED_MEM_SINK_CAST(bsink);
    GstSharedMemSinkPrivate *priv = shmem_sink->priv;
    g_return_val_if_fail(priv != nullptr, FALSE);

    GST_INFO_OBJECT(shmem_sink, "we are stop");

    g_mutex_lock(&priv->mutex);
    priv->flushing = TRUE;
    g_cond_signal(&priv->cond);
    g_mutex_unlock(&priv->mutex);

    return GST_BASE_SINK_CLASS(parent_class)->stop(bsink);
}

static void notify_memory_available(GstSharedMemSink *shmem_sink)
{
    g_return_if_fail(shmem_sink != nullptr);
    GstSharedMemSinkPrivate *priv = shmem_sink->priv;
    g_return_if_fail(priv != nullptr);

    g_mutex_lock(&priv->mutex);
    g_cond_signal(&priv->cond);
    g_mutex_unlock(&priv->mutex);
}

static gboolean set_pool_for_allocator(GstSharedMemSink *shmem_sink, guint min_bufs, guint max_bufs, guint size)
{
    GstSharedMemSinkPrivate *priv = shmem_sink->priv;

    if (priv->allocator == nullptr) {
        return FALSE;
    }

    if (priv->set_pool_for_allocator) {
        return TRUE;
    }

    if (size == 0 || max_bufs == 0) {
        GST_ERROR_OBJECT(shmem_sink, "need copy buffer, but the dest buf's size or pool capacity is not provided");
        return FALSE;
    }

    GST_DEBUG_OBJECT(shmem_sink, "min_bufs: %u, max_bufs: %u, size: %u", min_bufs, max_bufs, size);

    auto notifier = [shmem_sink]() {
        notify_memory_available(shmem_sink);
    };

    priv->av_shmem_pool = std::make_shared<OHOS::Media::AVSharedMemoryPool>("shmemsink");
    OHOS::Media::AVSharedMemoryPool::InitializeOption option = {
        .preAllocMemCnt = min_bufs,
        .memSize = size,
        .maxMemCnt = max_bufs,
        .notifier = notifier,
    };
    int32_t ret = priv->av_shmem_pool->Init(option);
    g_return_val_if_fail(ret == OHOS::Media::MSERR_OK, GST_FLOW_ERROR);

    priv->av_shmem_pool->SetNonBlocking(true);
    gst_shmem_allocator_set_pool(priv->allocator, priv->av_shmem_pool);
    priv->set_pool_for_allocator = TRUE;
    return TRUE;
}

static GstFlowReturn do_allocate_buffer(GstSharedMemSink *shmem_sink, GstBuffer **buffer)
{
    GstSharedMemSinkPrivate *priv = shmem_sink->priv;
    g_mutex_lock(&priv->mutex);

    while (!priv->flushing) {
        GstMemory *memory = gst_allocator_alloc(GST_ALLOCATOR_CAST(priv->allocator),
            priv->mem_size, &priv->alloc_params);
        if (memory != nullptr) {
            g_mutex_unlock(&priv->mutex);
            *buffer = gst_buffer_new();
            if (*buffer == nullptr) {
                GST_ERROR_OBJECT(shmem_sink, "buffer new failed");
                gst_allocator_free(GST_ALLOCATOR_CAST(priv->allocator), memory);
                return GST_FLOW_ERROR;
            }
            gst_buffer_append_memory(*buffer, memory);
            return GST_FLOW_OK;
        }

        if (priv->unlock) {
            g_mutex_unlock(&priv->mutex);
            GstFlowReturn ret = gst_base_sink_wait_preroll(GST_BASE_SINK(shmem_sink));
            if (ret != GST_FLOW_OK) {
                GST_INFO_OBJECT(shmem_sink, "we are stopping");
                return ret;
            }
            g_mutex_lock(&priv->mutex);
            continue;
        }

        g_cond_wait(&priv->cond, &priv->mutex);
    };

    g_mutex_unlock(&priv->mutex);
    GST_INFO_OBJECT(shmem_sink, "we are flushing");
    return GST_FLOW_FLUSHING;
}

static GstFlowReturn do_copy_buffer(GstSharedMemSink *shmem_sink, GstBuffer *in_buf, GstBuffer **out_buf)
{
    GstSharedMemSinkPrivate *priv = shmem_sink->priv;
    GstMemSink *memsink = GST_MEM_SINK_CAST(shmem_sink);

    gboolean ret = set_pool_for_allocator(shmem_sink, 1, memsink->max_pool_capacity, priv->mem_size);
    g_return_val_if_fail(ret, GST_FLOW_ERROR);

    GstFlowReturn flow_ret = do_allocate_buffer(shmem_sink, out_buf);
    g_return_val_if_fail(flow_ret == GST_FLOW_OK, flow_ret);
    g_return_val_if_fail(*out_buf != nullptr, GST_FLOW_ERROR);

    do {
        ret = gst_buffer_copy_into(*out_buf, in_buf, GST_BUFFER_COPY_METADATA, 0, -1);
        if (!ret) {
            GST_ERROR_OBJECT(shmem_sink, "copy metadata from in_buf failed");
            break;
        }

        // add buffer type meta here.

        GstMapInfo info;
        if (!gst_buffer_map(*out_buf, &info, GST_MAP_WRITE)) {
            GST_ERROR_OBJECT(shmem_sink, "map buffer failed");
            ret = FALSE;
            break;
        }

        gsize size = gst_buffer_get_size(in_buf);
        g_return_val_if_fail(info.size >= size, GST_FLOW_ERROR);
        gsize writesize = gst_buffer_extract(in_buf, 0, info.data, size);
        gst_buffer_unmap(*out_buf, &info);
        if (writesize != size) {
            GST_ERROR_OBJECT(shmem_sink, "extract buffer failed");
            ret = FALSE;
            break;
        }
        gst_buffer_resize(*out_buf, 0, size);
    } while (0);

    if (!ret) {
        gst_buffer_unref(*out_buf);
        *out_buf = nullptr;
        return GST_FLOW_ERROR;
    }

    return GST_FLOW_OK;
}

static gboolean check_need_copy(GstSharedMemSink *shmem_sink, GstBuffer *buffer)
{
    GstSharedMemSinkPrivate *priv = shmem_sink->priv;

    if (gst_buffer_n_memory(buffer) != 1) {
        GST_ERROR_OBJECT(shmem_sink, "buffer's memory chunks is not 1 !");
        return FALSE;
    }

    if (buffer->pool != nullptr) {
        if (buffer->pool != GST_BUFFER_POOL_CAST(priv->pool)) {
            return TRUE;
        }
        return FALSE;
    }

    if (priv->allocator != nullptr) {
        GstMemory *memory = gst_buffer_peek_memory(buffer, 0);
        if (memory == nullptr) {
            return FALSE;
        }
        if (memory->allocator == GST_ALLOCATOR_CAST(priv->allocator)) {
            return FALSE;
        }
    }

    return TRUE;
}

static GstFlowReturn gst_shared_mem_sink_do_stream_render(GstMemSink *memsink, GstBuffer **buffer)
{
    g_return_val_if_fail(memsink != nullptr && buffer != nullptr, GST_FLOW_ERROR);
    GstSharedMemSink *shmem_sink = GST_SHARED_MEM_SINK_CAST(memsink);
    GstSharedMemSinkPrivate *priv = shmem_sink->priv;
    g_return_val_if_fail(priv != nullptr, GST_FLOW_ERROR);
    GstBuffer *orig_buf = *buffer;
    GstBuffer *out_buf = nullptr;

    if (!check_need_copy(shmem_sink, orig_buf)) {
        // To keep the user interface consistent with the scenario where the output
        // buffer needs to be copied, reference counting needs to be added.
        gst_buffer_ref(orig_buf);
        return GST_FLOW_OK;
    }

    GstFlowReturn ret = do_copy_buffer(shmem_sink, orig_buf, &out_buf);
    g_return_val_if_fail(ret == GST_FLOW_OK, ret);

    *buffer = out_buf;
    return GST_FLOW_OK;
}

static gboolean set_pool_for_propose_allocation(GstSharedMemSink *shmem_sink, GstQuery *query, GstCaps *caps)
{
    GstMemSink *memsink = GST_MEM_SINK_CAST(shmem_sink);
    GstSharedMemSinkPrivate *priv = shmem_sink->priv;

    guint size = 0;
    guint min_buffers = 0;
    guint max_buffers = 0;
    gst_query_parse_nth_allocation_pool(query, 0, nullptr, &size, &min_buffers, &max_buffers);
    if (max_buffers == 0) {
        GST_INFO_OBJECT(shmem_sink, "correct the maxbuffer from %u to %u", max_buffers, memsink->max_pool_capacity);
        max_buffers = memsink->max_pool_capacity;
    }
    if (size == 0) {
        GST_INFO_OBJECT(shmem_sink, "correct the size from %u to %u", size, priv->mem_size);
        size = priv->mem_size;
    }

    if (priv->pool != nullptr)  {
        gst_object_unref(priv->pool);
    }
    priv->pool = gst_shmem_pool_new();
    g_return_val_if_fail(priv->pool != nullptr, FALSE);
    GstShMemPool *pool = priv->pool;
    GstStructure *params = gst_structure_new("mem", "memtype", G_TYPE_STRING, "avshmem", nullptr);
    gst_query_add_allocation_pool(query, GST_BUFFER_POOL_CAST(pool), size, min_buffers, max_buffers);
    gst_query_add_allocation_meta(query, GST_BUFFER_TYPE_META_API_TYPE, params);
    gst_structure_free(params);

    // the gstbufferpool will reconfig the av_shmem_pool when the gstbufferpool is activated.
    (void)gst_shmem_pool_set_avshmempool(priv->pool, priv->av_shmem_pool);

    GstStructure *config = gst_buffer_pool_get_config(GST_BUFFER_POOL_CAST(pool));
    g_return_val_if_fail(config != nullptr, FALSE);

    gst_buffer_pool_config_set_params(config, caps, size, min_buffers, max_buffers);
    gst_buffer_pool_config_set_allocator(config, GST_ALLOCATOR_CAST(priv->allocator), &priv->alloc_params);

    return gst_buffer_pool_set_config(GST_BUFFER_POOL_CAST(pool), config);
}

static gboolean gst_shared_mem_sink_do_propose_allocation(GstMemSink *memsink, GstQuery *query)
{
    g_return_val_if_fail(memsink != nullptr && query != nullptr, FALSE);
    GstSharedMemSink *shmem_sink = GST_SHARED_MEM_SINK_CAST(memsink);
    GstSharedMemSinkPrivate *priv = shmem_sink->priv;
    g_return_val_if_fail(priv != nullptr, FALSE);
    g_return_val_if_fail(priv->allocator != nullptr, FALSE);

    GstCaps *caps = nullptr;
    gboolean need_pool = FALSE;
    priv->set_pool_for_allocator = FALSE;
    gst_query_parse_allocation(query, &caps, &need_pool);
    GST_INFO_OBJECT(shmem_sink, "allocation query, caps: %s, need pool: %d", gst_caps_to_string(caps), need_pool);

    GST_OBJECT_LOCK(shmem_sink);
    gst_query_add_allocation_param(query, GST_ALLOCATOR_CAST(priv->allocator), &priv->alloc_params);

    // always set av_shmem_pool for allocator, in case that the upstream only use the
    // gstallocator while the needpool is set.
    gboolean ret = set_pool_for_allocator(shmem_sink, 1, memsink->max_pool_capacity, priv->mem_size);
    if (!ret) {
        GST_ERROR_OBJECT(shmem_sink, "set pool for allocator failed");
        GST_OBJECT_UNLOCK(shmem_sink);
        return ret;
    }

    if (need_pool) {
        ret = set_pool_for_propose_allocation(shmem_sink, query, caps);
        if (!ret) {
            GST_ERROR_OBJECT(shmem_sink, "config pool failed");
        }
    }

    GST_OBJECT_UNLOCK(shmem_sink);
    return ret;
}
