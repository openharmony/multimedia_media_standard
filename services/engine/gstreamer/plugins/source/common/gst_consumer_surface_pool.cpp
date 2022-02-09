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

#include "gst_consumer_surface_pool.h"
#include "gst_consumer_surface_allocator.h"
#include "gst_consumer_surface_memory.h"
#include "buffer_type_meta.h"
#include "scope_guard.h"
using namespace OHOS;

#define gst_consumer_surface_pool_parent_class parent_class

GST_DEBUG_CATEGORY_STATIC(gst_consumer_surface_pool_debug_category);
#define GST_CAT_DEFAULT gst_consumer_surface_pool_debug_category

struct _GstConsumerSurfacePoolPrivate {
    sptr<Surface> consumer_surface;
    guint available_buf_count;
    GMutex pool_lock;
    GCond buffer_available_con;
    gboolean flushing;
    gboolean start;
    gboolean suspend;
    guint32 repeat_interval;
    guint32 max_frame_rate;
    guint64 pre_timestamp;
    GstBuffer *cache_buffer;
};

enum {
    PROP_0,
    PROP_SUSPEND,
    PROP_REPEAT,
    PROP_MAX_FRAME_RATE,
};

G_DEFINE_TYPE_WITH_PRIVATE(GstConsumerSurfacePool, gst_consumer_surface_pool, GST_TYPE_VIDEO_BUFFER_POOL);

class ConsumerListenerProxy : public IBufferConsumerListener {
public:
    explicit ConsumerListenerProxy(GstConsumerSurfacePool &owner) : owner_(owner) {}
    ~ConsumerListenerProxy() = default;
    DISALLOW_COPY_AND_MOVE(ConsumerListenerProxy);
    void OnBufferAvailable() override;
private:
    GstConsumerSurfacePool &owner_;
};

static void gst_consumer_surface_pool_set_property(GObject *object, guint id, const GValue *value, GParamSpec *pspec);
static void gst_consumer_surface_pool_init(GstConsumerSurfacePool *pool);
static void gst_consumer_surface_pool_buffer_available(GstConsumerSurfacePool *pool);
static GstFlowReturn gst_consumer_surface_pool_acquire_buffer(GstBufferPool *pool, GstBuffer **buffer,
    GstBufferPoolAcquireParams *params);
static void gst_consumer_surface_pool_release_buffer(GstBufferPool *pool, GstBuffer *buffer);
static gboolean gst_consumer_surface_pool_stop(GstBufferPool *pool);
static gboolean gst_consumer_surface_pool_start(GstBufferPool *pool);
static void gst_consumer_surface_pool_flush_start(GstBufferPool *pool);
static void gst_consumer_surface_pool_flush_stop(GstBufferPool *pool);
static gboolean drop_this_fame(GstConsumerSurfacePool *pool, guint64 new_timestamp,
    guint64 old_timestamp, guint32 frame_rate);

void ConsumerListenerProxy::OnBufferAvailable()
{
    gst_consumer_surface_pool_buffer_available(&owner_);
}

static const gchar **gst_consumer_surface_pool_get_options(GstBufferPool *pool)
{
    static const gchar *options[] = { GST_BUFFER_POOL_OPTION_VIDEO_META, nullptr };
    return options;
}

static gboolean gst_consumer_surface_pool_set_config(GstBufferPool *pool, GstStructure *config)
{
    g_return_val_if_fail(pool != nullptr, FALSE);
    g_return_val_if_fail(config != nullptr, FALSE);

    GstAllocator *allocator = nullptr;
    (void)gst_buffer_pool_config_get_allocator(config, &allocator, nullptr);
    if (!(allocator && GST_IS_CONSUMER_SURFACE_ALLOCATOR(allocator))) {
        GST_WARNING_OBJECT(pool, "no valid allocator in pool");
        return FALSE;
    }

    return GST_BUFFER_POOL_CLASS(parent_class)->set_config(pool, config);
}

// before unref must stop(deactive)
static void gst_consumer_surface_pool_finalize(GObject *obj)
{
    g_return_if_fail(obj != nullptr);
    GstConsumerSurfacePool *surfacepool = GST_CONSUMER_SURFACE_POOL_CAST(obj);
    g_return_if_fail(surfacepool != nullptr && surfacepool->priv != nullptr);
    auto priv = surfacepool->priv;
    if (priv->consumer_surface != nullptr) {
        if (priv->consumer_surface->UnregisterConsumerListener() != SURFACE_ERROR_OK) {
            GST_WARNING_OBJECT(surfacepool, "deregister consumer listener fail");
        }
        priv->consumer_surface = nullptr;
    }
    g_mutex_clear(&priv->pool_lock);
    g_cond_clear(&priv->buffer_available_con);
    G_OBJECT_CLASS(parent_class)->finalize(obj);
}

static void gst_consumer_surface_pool_class_init(GstConsumerSurfacePoolClass *klass)
{
    g_return_if_fail(klass != nullptr);
    GstBufferPoolClass *poolClass = GST_BUFFER_POOL_CLASS (klass);
    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);
    GST_DEBUG_CATEGORY_INIT(gst_consumer_surface_pool_debug_category, "surfacepool", 0, "surface pool");
    gobjectClass->set_property = gst_consumer_surface_pool_set_property;
    gobjectClass->finalize = gst_consumer_surface_pool_finalize;

    g_object_class_install_property(gobjectClass, PROP_SUSPEND,
        g_param_spec_boolean("suspend", "Suspend surface", "Suspend surface",
            FALSE, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobjectClass, PROP_REPEAT,
        g_param_spec_uint("repeat", "Repeat frame", "Repeat previous frame after given milliseconds",
            0, G_MAXUINT32, 0, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobjectClass, PROP_MAX_FRAME_RATE,
        g_param_spec_uint("max-framerate", "Max frame rate", "Max frame rate",
            0, G_MAXUINT32, 0, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    poolClass->get_options = gst_consumer_surface_pool_get_options;
    poolClass->set_config = gst_consumer_surface_pool_set_config;
    poolClass->release_buffer = gst_consumer_surface_pool_release_buffer;
    poolClass->acquire_buffer = gst_consumer_surface_pool_acquire_buffer;
    poolClass->start = gst_consumer_surface_pool_start;
    poolClass->stop = gst_consumer_surface_pool_stop;
    poolClass->flush_start = gst_consumer_surface_pool_flush_start;
    poolClass->flush_stop = gst_consumer_surface_pool_flush_stop;
}

static void gst_consumer_surface_pool_set_property(GObject *object, guint id, const GValue *value, GParamSpec *pspec)
{
    (void)pspec;
    GstConsumerSurfacePool *surfacepool = GST_CONSUMER_SURFACE_POOL(object);
    g_return_if_fail(surfacepool != nullptr && value != nullptr);
    auto priv = surfacepool->priv;

    g_mutex_lock(&priv->pool_lock);
    ON_SCOPE_EXIT(0) { g_mutex_unlock(&priv->pool_lock); };

    switch (id) {
        case PROP_SUSPEND:
            priv->suspend = g_value_get_boolean(value);
            break;
        case PROP_REPEAT:
            if (g_value_get_uint(value) == 0 && priv->cache_buffer != nullptr) {
                gst_buffer_unref(priv->cache_buffer);
                priv->cache_buffer = nullptr;
            }
            priv->repeat_interval = g_value_get_uint(value) * 1000; // ms to us
            break;
        case PROP_MAX_FRAME_RATE:
            priv->max_frame_rate = g_value_get_uint(value);
            break;
        default:
            break;
    }
}

static void gst_consumer_surface_pool_flush_start(GstBufferPool *pool)
{
    GstConsumerSurfacePool *surfacepool = GST_CONSUMER_SURFACE_POOL(pool);
    g_return_if_fail(surfacepool != nullptr && surfacepool->priv != nullptr);
    auto priv = surfacepool->priv;
    g_mutex_lock(&priv->pool_lock);
    if (priv->cache_buffer != nullptr) {
        gst_buffer_unref(priv->cache_buffer);
        priv->cache_buffer = nullptr;
    }
    surfacepool->priv->flushing = TRUE;
    g_cond_signal(&priv->buffer_available_con);
    g_mutex_unlock(&priv->pool_lock);
}

static void gst_consumer_surface_pool_flush_stop(GstBufferPool *pool)
{
    GstConsumerSurfacePool *surfacepool = GST_CONSUMER_SURFACE_POOL(pool);
    g_return_if_fail(surfacepool != nullptr && surfacepool->priv != nullptr);
    auto priv = surfacepool->priv;
    g_mutex_lock(&priv->pool_lock);
    surfacepool->priv->flushing = FALSE;
    g_mutex_unlock(&priv->pool_lock);
}

// Disable pre-caching
static gboolean gst_consumer_surface_pool_start(GstBufferPool *pool)
{
    GstConsumerSurfacePool *surfacepool = GST_CONSUMER_SURFACE_POOL(pool);
    g_return_val_if_fail(surfacepool != nullptr && surfacepool->priv != nullptr, FALSE);
    auto priv = surfacepool->priv;
    g_mutex_lock(&priv->pool_lock);
    surfacepool->priv->start = TRUE;
    g_mutex_unlock(&priv->pool_lock);
    return TRUE;
}

// Disable release buffers
static gboolean gst_consumer_surface_pool_stop(GstBufferPool *pool)
{
    GstConsumerSurfacePool *surfacepool = GST_CONSUMER_SURFACE_POOL(pool);
    g_return_val_if_fail(surfacepool != nullptr && surfacepool->priv != nullptr, FALSE);
    auto priv = surfacepool->priv;
    g_mutex_lock(&priv->pool_lock);
    surfacepool->priv->start = FALSE;
    g_cond_signal(&priv->buffer_available_con);
    g_mutex_unlock(&priv->pool_lock);
    return TRUE;
}

static void gst_consumer_surface_pool_release_buffer(GstBufferPool *pool, GstBuffer *buffer)
{
    g_return_if_fail(pool != nullptr && buffer != nullptr);
    GstMemory *mem = gst_buffer_peek_memory(buffer, 0);
    if (gst_is_consumer_surface_memory(mem)) {
        GstBufferTypeMeta *meta = gst_buffer_get_buffer_type_meta(buffer);
        if (meta != nullptr) {
            GstConsumerSurfaceMemory *surfacemem = reinterpret_cast<GstConsumerSurfaceMemory *>(mem);
            surfacemem->fencefd = meta->fenceFd;
        }
    }
    // the buffer's pool is remove, the buffer will free by allocator.
    gst_buffer_unref(buffer);
}

static GstFlowReturn gst_consumer_surface_pool_acquire_buffer(GstBufferPool *pool, GstBuffer **buffer,
    GstBufferPoolAcquireParams *params)
{
    GstConsumerSurfacePool *surfacepool = GST_CONSUMER_SURFACE_POOL(pool);
    g_return_val_if_fail(surfacepool != nullptr && surfacepool->priv != nullptr, GST_FLOW_ERROR);
    GstBufferPoolClass *pclass = GST_BUFFER_POOL_GET_CLASS(pool);
    g_return_val_if_fail(pclass != nullptr, GST_FLOW_ERROR);
    if (!pclass->alloc_buffer) {
        return GST_FLOW_NOT_SUPPORTED;
    }
    auto priv = surfacepool->priv;
    g_mutex_lock(&priv->pool_lock);
    ON_SCOPE_EXIT(0) { g_mutex_unlock(&priv->pool_lock); };

    do {
        gboolean repeat = FALSE;
        while (priv->available_buf_count == 0 && !priv->flushing && priv->start) {
            if (priv->repeat_interval == 0 || priv->cache_buffer == nullptr) {
                g_cond_wait(&priv->buffer_available_con, &priv->pool_lock);
            } else if (g_cond_wait_until(&priv->buffer_available_con, &priv->pool_lock, priv->repeat_interval)) {
                GST_INFO_OBJECT(surfacepool, "Repeat previous frame after waiting given microseconds");
                repeat = TRUE;
            }
        }
        if (priv->flushing || !priv->start) {
            return GST_FLOW_FLUSHING;
        }

        if (repeat && priv->cache_buffer != nullptr) {
            *buffer = priv->cache_buffer;
            gst_buffer_ref(priv->cache_buffer);
            GST_BUFFER_PTS(*buffer) = priv->pre_timestamp + priv->repeat_interval;
            priv->pre_timestamp = GST_BUFFER_PTS(*buffer);
            break;
        }

        GstFlowReturn result = pclass->alloc_buffer(pool, buffer, params);
        g_return_val_if_fail(result == GST_FLOW_OK && *buffer != nullptr, GST_FLOW_ERROR);
        GstMemory *mem = gst_buffer_peek_memory(*buffer, 0);
        GstConsumerSurfaceMemory *surfacemem = nullptr;
        if (gst_is_consumer_surface_memory(mem)) {
            surfacemem = reinterpret_cast<GstConsumerSurfaceMemory*>(mem);
            uint32_t bufferFlag = 0;
            if (surfacemem->is_eos_frame) {
                bufferFlag = BUFFER_FLAG_EOS;
            }
            gst_buffer_add_buffer_handle_meta(*buffer, surfacemem->buffer_handle, surfacemem->fencefd, bufferFlag);
            GST_BUFFER_PTS(*buffer) = surfacemem->timestamp;
        }
        priv->available_buf_count--;

        // check whether needs to dropp frame to ensure the maximum frame rate
        if (surfacemem != nullptr && priv->max_frame_rate > 0 &&
            drop_this_fame(surfacepool, surfacemem->timestamp, priv->pre_timestamp, priv->max_frame_rate)) {
            (void)priv->consumer_surface->ReleaseBuffer(surfacemem->surface_buffer, surfacemem->fencefd);
            if (!priv->flushing && priv->start) {
                continue;
            }
        }
        priv->pre_timestamp = surfacemem->timestamp;
        if (priv->repeat_interval > 0) {
            if (priv->cache_buffer != nullptr) {
                gst_buffer_unref(priv->cache_buffer);
            }
            priv->cache_buffer = *buffer;
            gst_buffer_ref(priv->cache_buffer);
        }
    } while (0);

    return GST_FLOW_OK;
}

static void gst_consumer_surface_pool_init(GstConsumerSurfacePool *pool)
{
    g_return_if_fail(pool != nullptr);
    auto priv = reinterpret_cast<GstConsumerSurfacePoolPrivate *>
        (gst_consumer_surface_pool_get_instance_private(pool));
    g_return_if_fail(priv != nullptr);
    pool->priv = priv;
    priv->available_buf_count = 0;
    priv->flushing = FALSE;
    priv->start = FALSE;
    priv->suspend = FALSE;
    priv->repeat_interval = 0;
    priv->max_frame_rate = 0;
    priv->pre_timestamp = 0;
    priv->cache_buffer = nullptr;
    g_mutex_init(&priv->pool_lock);
    g_cond_init(&priv->buffer_available_con);
}

static void gst_consumer_surface_pool_buffer_available(GstConsumerSurfacePool *pool)
{
    g_return_if_fail(pool != nullptr && pool->priv != nullptr);
    auto priv = pool->priv;
    g_mutex_lock(&priv->pool_lock);
    ON_SCOPE_EXIT(0) { g_mutex_unlock(&priv->pool_lock); };

    if (priv->suspend) {
        sptr<SurfaceBuffer> buffer = nullptr;
        gint32 fencefd = -1;
        gint64 timestamp = 0;
        Rect damage = {0, 0, 0, 0};
        if (priv->consumer_surface->AcquireBuffer(buffer, fencefd, timestamp, damage) == SURFACE_ERROR_OK) {
            GST_INFO_OBJECT(pool, "Surface is suspended, release buffer");
            (void)priv->consumer_surface->ReleaseBuffer(buffer, fencefd);
            return;
        }
    }

    if (priv->available_buf_count == 0) {
        g_cond_signal(&priv->buffer_available_con);
    }
    pool->priv->available_buf_count++;
    GST_DEBUG_OBJECT(pool, "Available buffer count %u", pool->priv->available_buf_count);
}

void gst_consumer_surface_pool_set_surface(GstBufferPool *pool, sptr<Surface> &consumer_surface)
{
    GstConsumerSurfacePool *surfacepool = GST_CONSUMER_SURFACE_POOL(pool);
    g_return_if_fail(surfacepool != nullptr && surfacepool->priv != nullptr);
    g_return_if_fail(consumer_surface != nullptr && surfacepool->priv->consumer_surface == nullptr);
    surfacepool->priv->consumer_surface = consumer_surface;
    sptr<IBufferConsumerListener> listenerProxy = new (std::nothrow) ConsumerListenerProxy(*surfacepool);
    g_return_if_fail(listenerProxy != nullptr);

    if (consumer_surface->RegisterConsumerListener(listenerProxy) != SURFACE_ERROR_OK) {
        GST_WARNING_OBJECT(surfacepool, "register consumer listener fail");
    }
}

static gboolean drop_this_fame(GstConsumerSurfacePool *pool, guint64 new_timestamp,
    guint64 old_timestamp, guint32 frame_rate)
{
    if (new_timestamp <= old_timestamp) {
        GST_WARNING_OBJECT(pool, "Invalid timestamp: not increased");
        return TRUE;
    }

    if (frame_rate == 0) {
        GST_WARNING_OBJECT(pool, "Invalid frame rate: 0");
        return FALSE;
    }
    guint64 min_interval = 1000000000 / frame_rate; // 1s = 1000000000ns
    if ((UINT64_MAX - min_interval) < old_timestamp) {
        GST_WARNING_OBJECT(pool, "Invalid timestamp: too big");
        return TRUE;
    }

    const guint64 deviations = 3000000; // 3ms
    if (new_timestamp < (old_timestamp - deviations + min_interval)) {
        GST_INFO_OBJECT(pool, "Drop this frame to make sure maximum frame rate");
        return TRUE;
    }
    return FALSE;
}

GstBufferPool *gst_consumer_surface_pool_new()
{
    GstBufferPool *pool = GST_BUFFER_POOL_CAST(g_object_new(
        GST_TYPE_CONSUMER_SURFACE_POOL, "name", "consumer_surfacepool", nullptr));
    (void)gst_object_ref_sink(pool);

    return pool;
}
