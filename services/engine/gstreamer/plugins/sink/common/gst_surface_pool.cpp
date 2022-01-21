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

#include "gst_surface_pool.h"
#include <unordered_map>
#include "media_log.h"
#include "display_type.h"
#include "surface_buffer.h"
#include "buffer_type_meta.h"
#include "gst_surface_allocator.h"
#include "gst/video/gstvideometa.h"

namespace {
    const std::unordered_map<GstVideoFormat, PixelFormat> FORMAT_MAPPING = {
        { GST_VIDEO_FORMAT_RGBA, PIXEL_FMT_RGBA_8888 },
        { GST_VIDEO_FORMAT_NV21, PIXEL_FMT_YCRCB_420_SP },
    };
}

#define GST_BUFFER_POOL_LOCK(pool)   (g_mutex_lock(&pool->lock))
#define GST_BUFFER_POOL_UNLOCK(pool) (g_mutex_unlock(&pool->lock))
#define GST_BUFFER_POOL_WAIT(pool, endtime) (g_cond_wait_until(&pool->cond, &pool->lock, endtime))
#define GST_BUFFER_POOL_NOTIFY(pool) (g_cond_signal(&pool->cond))

#define gst_surface_pool_parent_class parent_class
G_DEFINE_TYPE (GstSurfacePool, gst_surface_pool, GST_TYPE_BUFFER_POOL);

static void gst_surface_pool_finalize(GObject *obj);
static const gchar **gst_surface_pool_get_options (GstBufferPool *pool);
static gboolean gst_surface_pool_set_config(GstBufferPool *pool, GstStructure *config);
static gboolean gst_surface_pool_start(GstBufferPool *pool);
static gboolean gst_surface_pool_stop(GstBufferPool *pool);
static GstFlowReturn gst_surface_pool_alloc_buffer(GstBufferPool *pool,
    GstBuffer **buffer, GstBufferPoolAcquireParams *params);
static void gst_surface_pool_free_buffer(GstBufferPool *pool, GstBuffer *buffer);
static GstFlowReturn gst_surface_pool_acquire_buffer(GstBufferPool *pool,
    GstBuffer **buffer, GstBufferPoolAcquireParams *params);
static void gst_surface_pool_release_buffer(GstBufferPool *pool, GstBuffer *buffer);

static void gst_surface_pool_class_init (GstSurfacePoolClass *klass)
{
    g_return_if_fail(klass != nullptr);
    GstBufferPoolClass *poolClass = GST_BUFFER_POOL_CLASS (klass);
    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);

    gobjectClass->finalize = gst_surface_pool_finalize;
    poolClass->get_options = gst_surface_pool_get_options;
    poolClass->set_config = gst_surface_pool_set_config;
    poolClass->start = gst_surface_pool_start;
    poolClass->stop = gst_surface_pool_stop;
    poolClass->acquire_buffer = gst_surface_pool_acquire_buffer;
    poolClass->alloc_buffer = gst_surface_pool_alloc_buffer;
    poolClass->release_buffer = gst_surface_pool_release_buffer;
    poolClass->free_buffer = gst_surface_pool_free_buffer;
}

static void gst_surface_pool_init (GstSurfacePool *pool)
{
    g_return_if_fail(pool != nullptr);

    pool->surface = nullptr;
    pool->started = FALSE;
    g_mutex_init(&pool->lock);
    g_cond_init(&pool->cond);
    pool->allocator = nullptr;
    pool->preAllocated = nullptr;
    pool->minBuffers = 0;
    pool->maxBuffers = 0;
    pool->waittime = 0;
    pool->format = PixelFormat::PIXEL_FMT_BUTT;
    pool->usage = 0;
    gst_video_info_init(&pool->info);
    gst_allocation_params_init(&pool->params);
}

static void gst_surface_pool_finalize(GObject *obj)
{
    g_return_if_fail(obj != nullptr);
    GstSurfacePool *spool = GST_SURFACE_POOL_CAST(obj);
    g_return_if_fail(spool != nullptr);
    GstBufferPool *pool = GST_BUFFER_POOL_CAST(spool);

    while (spool->preAllocated != nullptr) {
        if (spool->preAllocated->data != nullptr) {
            GstBuffer *buffer = GST_BUFFER_CAST(spool->preAllocated->data);
            gst_surface_pool_free_buffer(pool, buffer);
        }
        spool->preAllocated = g_list_delete_link(spool->preAllocated, spool->preAllocated);
    }

    spool->surface = nullptr;
    gst_object_unref(spool->allocator);
    spool->allocator = nullptr;
    g_mutex_clear(&spool->lock);
    g_cond_clear(&spool->cond);

    G_OBJECT_CLASS(parent_class)->finalize(obj);
}

GstSurfacePool *gst_surface_pool_new()
{
    GstSurfacePool *pool = GST_SURFACE_POOL_CAST(g_object_new(
        GST_TYPE_SURFACE_POOL, "name", "SurfacePool", nullptr));
    (void)gst_object_ref_sink(pool);

    return pool;
}

static const gchar **gst_surface_pool_get_options (GstBufferPool *pool)
{
    // add buffer type meta option at here
    static const gchar *options[] = { GST_BUFFER_POOL_OPTION_VIDEO_META, nullptr };
    return options;
}

static gboolean parse_caps_info(GstCaps *caps, GstVideoInfo *info, PixelFormat *format)
{
    if (!gst_video_info_from_caps(info, caps)) {
        GST_ERROR("wrong caps, %" GST_PTR_FORMAT, caps);
        return FALSE;
    }

    if (FORMAT_MAPPING.count(GST_VIDEO_INFO_FORMAT(info)) == 0) {
        GST_ERROR("unsupported format, %s", GST_VIDEO_INFO_NAME(info));
        return FALSE;
    }

    *format = FORMAT_MAPPING.at(GST_VIDEO_INFO_FORMAT(info));
    return TRUE;
}

static gboolean parse_config_usage(GstSurfacePool *spool, GstStructure *config)
{
    g_return_val_if_fail(spool != nullptr, FALSE);
    g_return_val_if_fail(config != nullptr, FALSE);
    if (!gst_structure_get_int(config, "usage", &spool->usage)) {
        GST_INFO("no usage available");
        spool->usage = 0;
    }
    return TRUE;
}

static gboolean gst_surface_pool_set_config(GstBufferPool *pool, GstStructure *config)
{
    GstSurfacePool *spool = GST_SURFACE_POOL_CAST(pool);
    g_return_val_if_fail(spool != nullptr, FALSE);
    g_return_val_if_fail(config != nullptr, FALSE);

    GstCaps *caps = nullptr;
    guint size; // ignore the size
    guint minBuffers;
    guint maxBuffers;
    if (!gst_buffer_pool_config_get_params(config, &caps, &size, &minBuffers, &maxBuffers)) {
        GST_ERROR("wrong config");
        return FALSE;
    }
    g_return_val_if_fail(minBuffers <= maxBuffers, FALSE);

    GstVideoInfo info;
    PixelFormat format;
    if (!parse_caps_info(caps, &info, &format)) {
        return FALSE;
    }
    g_return_val_if_fail(parse_config_usage(spool, config), FALSE);
    GST_DEBUG_OBJECT(pool, "get usage %u", spool->usage);

    GST_BUFFER_POOL_LOCK(spool);

    spool->info = info;
    spool->format = format;
    spool->minBuffers = minBuffers;
    spool->maxBuffers = maxBuffers;

    GST_INFO("set config, width: %d, height: %d, format: %d, min_bufs: %u, max_bufs: %u",
        GST_VIDEO_INFO_WIDTH(&info), GST_VIDEO_INFO_HEIGHT(&info), format, minBuffers, maxBuffers);

    GstAllocator *allocator = nullptr;
    GstAllocationParams params;
    (void)gst_buffer_pool_config_get_allocator(config, &allocator, &params);
    if (!(allocator && GST_IS_SURFACE_ALLOCATOR(allocator))) {
        GST_BUFFER_POOL_UNLOCK(spool);
        GST_WARNING_OBJECT(pool, "no valid allocator in pool");
        return TRUE;
    }
    spool->allocator = GST_SURFACE_ALLOCATOR_CAST(allocator);
    spool->params = params;

    GST_INFO("update allocator and params");

    GST_BUFFER_POOL_UNLOCK(spool);
    return TRUE;
}

gboolean gst_surface_pool_set_surface(GstSurfacePool *pool, OHOS::sptr<OHOS::Surface> surface, guint waittime)
{
    g_return_val_if_fail(surface != nullptr, FALSE);
    g_return_val_if_fail(pool != nullptr, FALSE);

    GST_BUFFER_POOL_LOCK(pool);

    if (pool->started) {
        GST_INFO("started already, reject to set surface");
        GST_BUFFER_POOL_UNLOCK(pool);
        return FALSE;
    }

    if (pool->surface != nullptr) {
        GST_INFO("surface has already been set");
        GST_BUFFER_POOL_UNLOCK(pool);
        return FALSE;
    }

    static const guint maxWaitTime = 5000; // microsecond
    pool->waittime = (waittime == 0) ? maxWaitTime : waittime;
    pool->surface = surface;
    GST_DEBUG("set waittime: %u", pool->waittime);

    GST_BUFFER_POOL_UNLOCK(pool);
    return TRUE;
}

static gboolean gst_surface_pool_start(GstBufferPool *pool)
{
    g_return_val_if_fail(pool != nullptr, FALSE);
    GstSurfacePool *spool = GST_SURFACE_POOL_CAST(pool);
    g_return_val_if_fail(spool != nullptr, FALSE);

    GST_DEBUG("pool start");

    GST_BUFFER_POOL_LOCK(spool);
    if (spool->surface == nullptr) {
        GST_BUFFER_POOL_UNLOCK(spool);
        GST_ERROR("not set surface");
        return FALSE;
    }

    OHOS::SurfaceError err = spool->surface->SetQueueSize(spool->maxBuffers);
    if (err != OHOS::SurfaceError::SURFACE_ERROR_OK) {
        GST_BUFFER_POOL_UNLOCK(spool);
        GST_ERROR("set queue size to %u failed", spool->maxBuffers);
        return FALSE;
    }

    gst_surface_allocator_set_surface(spool->allocator, spool->surface);
    spool->started = TRUE;
    GST_INFO("Set pool minbuf %d maxbuf %d", spool->minBuffers, spool->maxBuffers);
    spool->freeBufCnt = spool->maxBuffers;
    GstFlowReturn ret = GST_FLOW_OK;
    for (guint i = 0; i < spool->minBuffers; i++) {
        GstBuffer *buffer = nullptr;
        ret = gst_surface_pool_alloc_buffer(pool, &buffer, nullptr);
        if (ret != GST_FLOW_OK) {
            GST_BUFFER_POOL_UNLOCK(spool);
            GST_ERROR("alloc buffer failed");
            return FALSE;
        }
        spool->preAllocated = g_list_append(spool->preAllocated, buffer);
    }

    GST_BUFFER_POOL_UNLOCK(spool);
    return TRUE;
}

static gboolean gst_surface_pool_stop(GstBufferPool *pool)
{
    g_return_val_if_fail(pool != nullptr, FALSE);
    GstSurfacePool *spool = GST_SURFACE_POOL_CAST(pool);
    g_return_val_if_fail(spool != nullptr, FALSE);

    GST_DEBUG("pool stop");

    GST_BUFFER_POOL_LOCK(spool);
    spool->started = FALSE;

    for (GList *node = g_list_first(spool->preAllocated); node != nullptr; node = g_list_next(node)) {
        GstBuffer *buffer = GST_BUFFER_CAST(node->data);
        if (buffer == nullptr) {
            continue;
        }
        gst_surface_pool_free_buffer(pool, buffer);
    }
    g_list_free(spool->preAllocated);
    spool->preAllocated = nullptr;

    GST_BUFFER_POOL_NOTIFY(spool); // wakeup immediately

    // leave all configuration unchanged.
    gboolean ret = (spool->freeBufCnt == spool->maxBuffers);
    GST_BUFFER_POOL_UNLOCK(spool);

    return ret;
}

static GstFlowReturn do_alloc_memory_locked(GstSurfacePool *spool,
    GstBufferPoolAcquireParams *params, GstSurfaceMemory **memory)
{
    GstVideoInfo *info = &spool->info;
    *memory = nullptr;

    if (spool->freeBufCnt > 0) {
        GST_DEBUG("do_alloc_memory_locked");
        *memory = gst_surface_allocator_alloc(spool->allocator, GST_VIDEO_INFO_WIDTH(info),
            GST_VIDEO_INFO_HEIGHT(info), spool->format, spool->usage);
    }

    return GST_FLOW_OK;
}

static GstFlowReturn gst_surface_pool_alloc_buffer(GstBufferPool *pool,
    GstBuffer **buffer, GstBufferPoolAcquireParams *params)
{
    g_return_val_if_fail(pool != nullptr && buffer != nullptr, GST_FLOW_ERROR);
    GstSurfacePool *spool = GST_SURFACE_POOL_CAST(pool);
    g_return_val_if_fail(spool != nullptr, GST_FLOW_ERROR);

    GST_DEBUG_OBJECT(spool, "alloc surface buffer");

    GstSurfaceMemory *memory = nullptr;
    GstFlowReturn ret = do_alloc_memory_locked(spool, params, &memory);
    g_return_val_if_fail(memory != nullptr, ret);

    *buffer = gst_buffer_new();
    if (*buffer == nullptr) {
        GST_ERROR("alloc gst buffer failed");
        gst_allocator_free(reinterpret_cast<GstAllocator*>(spool->allocator), reinterpret_cast<GstMemory*>(memory));
        return GST_FLOW_ERROR;
    }

    gst_buffer_append_memory(*buffer, reinterpret_cast<GstMemory *>(memory));

    GstVideoInfo *info = &spool->info;
    gst_buffer_add_video_meta(*buffer, GST_VIDEO_FRAME_FLAG_NONE, GST_VIDEO_INFO_FORMAT(info),
        GST_VIDEO_INFO_WIDTH(info), GST_VIDEO_INFO_HEIGHT(info));
    // add buffer type meta at here.
    OHOS::sptr<OHOS::SurfaceBuffer> buf = memory->buf;
    intptr_t buffer_handle = reinterpret_cast<intptr_t>(buf->GetBufferHandle());
    gst_buffer_add_buffer_handle_meta(*buffer, buffer_handle, memory->fence, 0);

    return GST_FLOW_OK;
}

static void gst_surface_pool_free_buffer(GstBufferPool *pool, GstBuffer *buffer)
{
    (void)pool;
    g_return_if_fail(buffer != nullptr);
    gst_buffer_unref(buffer);
}

static GstFlowReturn gst_surface_pool_acquire_buffer(GstBufferPool *pool,
    GstBuffer **buffer, GstBufferPoolAcquireParams *params)
{
    g_return_val_if_fail(pool != nullptr && buffer != nullptr, GST_FLOW_ERROR);
    GstSurfacePool *spool = GST_SURFACE_POOL_CAST(pool);
    g_return_val_if_fail(spool != nullptr, GST_FLOW_ERROR);
    GstFlowReturn ret = GST_FLOW_OK;
    gint64 retries = 0;

    GST_DEBUG("acquire buffer");
    GST_BUFFER_POOL_LOCK(spool);

    GList *node = g_list_first(spool->preAllocated);
    if (node != nullptr) {
        *buffer = GST_BUFFER_CAST(node->data);
        g_return_val_if_fail(*buffer != nullptr, GST_FLOW_ERROR);
        spool->preAllocated = g_list_delete_link(spool->preAllocated, node);
        GST_DEBUG("acquire buffer from preallocated buffers");

        spool->freeBufCnt -= 1;
        GST_BUFFER_POOL_UNLOCK(spool);
        return GST_FLOW_OK;
    }
    if (spool->freeBufCnt == 0 && (params != nullptr) && (params->flags & GST_BUFFER_POOL_ACQUIRE_FLAG_DONTWAIT)) {
        GST_BUFFER_POOL_WAIT(spool, 0);
    }
    while (TRUE) {
        // when pool is set flusing or set inactive, the flusing state is true, refer to GstBufferPool
        if (GST_BUFFER_POOL_IS_FLUSHING(pool)) {
            ret = GST_FLOW_FLUSHING;
            GST_INFO("pool is flushing");
            break;
        }

        /**
         * Although we maybe able to request buffer from Surface if the surface buffer is flushed
         * back to Surface, but we still require the GstBuffer to be released back to the pool
         * before we allow to acquire a GstBuffer again, for avoiding the user to hold two different
         * GstBuffer which pointer to same SurfaceBuffer.
         */
        if (spool->freeBufCnt > 0) {
            GST_DEBUG("Gst_surface_allocator_alloc");
            // Rather than keeping a idlelist for surface buffer by ourself, we request buffer
            // from the Surface directly.
            GstFlowReturn ret = gst_surface_pool_alloc_buffer(pool, buffer, params);
            if (ret == GST_FLOW_OK && *buffer != nullptr) {
                break;
            }
        }

        if ((params != nullptr) && (params->flags & GST_BUFFER_POOL_ACQUIRE_FLAG_DONTWAIT)) {
            ret = GST_FLOW_EOS;
            break;
        }

        retries += 1;
        GST_DEBUG("already try %" G_GINT64_FORMAT " times", retries);

        if (!GST_BUFFER_POOL_IS_FLUSHING(pool)) {
            GST_DEBUG("before sleep currtime: %" G_GINT64_FORMAT, g_get_monotonic_time());
            gint64 endTime = g_get_monotonic_time() + static_cast<gint64>(spool->waittime);
            GST_BUFFER_POOL_WAIT(spool, endTime);
            GST_DEBUG("after sleep currtime: %" G_GINT64_FORMAT, g_get_monotonic_time());
        }
    }
    if (ret == GST_FLOW_OK) {
        spool->freeBufCnt -= 1;
    }

    if (ret == GST_FLOW_EOS) {
        GST_DEBUG("no more buffers");
    }

    // The GstBufferPool will add the GstBuffer's pool ref to this pool.
    GST_BUFFER_POOL_UNLOCK(spool);
    return ret;
}

static void gst_surface_pool_release_buffer(GstBufferPool *pool, GstBuffer *buffer)
{
    // The GstBufferPool has already cleared the GstBuffer's pool ref to this pool.

    GstSurfacePool *spool = GST_SURFACE_POOL_CAST(pool);
    g_return_if_fail(spool != nullptr);
    GST_DEBUG("release buffer 0x%06" PRIXPTR "", FAKE_POINTER(buffer));

    if (G_UNLIKELY(!gst_buffer_is_all_memory_writable(buffer))) {
        GST_WARNING("buffer is not writable, 0x%06" PRIXPTR, FAKE_POINTER(buffer));
    }

    gboolean needNotify = FALSE;
    for (guint i = 0; i < gst_buffer_n_memory(buffer); i++) {
        GstMemory *memory = gst_buffer_peek_memory(buffer, i);
        if (gst_is_surface_memory(memory)) {
            GstSurfaceMemory *surfaceMem = reinterpret_cast<GstSurfaceMemory *>(memory);
            // the needRender is set by the surface sink
            if (!surfaceMem->needRender) {
                needNotify = TRUE;
            }
        }
    }

    // we dont queue the buffer to the idlelist. the memory rotation reuse feature
    // provided by the Surface itself.
    gst_surface_pool_free_buffer(pool, buffer);

    GST_BUFFER_POOL_LOCK(spool);
    spool->freeBufCnt += 1;
    GST_BUFFER_POOL_UNLOCK(spool);

    // if there is GstSurfaceMemory that does not need to be rendered, we can
    // notify the waiters to wake up and retry to acquire buffer.
    if (needNotify) {
        GST_BUFFER_POOL_NOTIFY(spool);
    }
}
