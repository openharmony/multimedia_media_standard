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

#include "gst_surface_mem_sink.h"
#include <cinttypes>
#include "surface.h"
#include "gst_surface_pool.h"
#include "buffer_type_meta.h"
#include "media_log.h"

namespace {
    constexpr guint32 DEFAULT_SURFACE_MAX_POOL_CAPACITY = 10; // 10 is surface queue max size
}

struct _GstSurfaceMemSinkPrivate {
    OHOS::sptr<OHOS::Surface> surface;
    GstSurfacePool *pool;
};

enum {
    PROP_0,
    PROP_SURFACE,
};

static GstStaticPadTemplate g_sinktemplate = GST_STATIC_PAD_TEMPLATE("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static void gst_surface_mem_sink_dispose(GObject *obj);
static void gst_surface_mem_sink_finalize(GObject *object);
static void gst_surface_mem_sink_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_surface_mem_sink_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static gboolean gst_surface_mem_sink_do_propose_allocation(GstMemSink *memsink, GstQuery *query);
static GstFlowReturn gst_surface_mem_sink_do_app_render(GstMemSink *memsink, GstBuffer *buffer);

#define gst_surface_mem_sink_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstSurfaceMemSink, gst_surface_mem_sink,
                        GST_TYPE_MEM_SINK, G_ADD_PRIVATE(GstSurfaceMemSink));

GST_DEBUG_CATEGORY_STATIC(gst_surface_mem_sink_debug_category);
#define GST_CAT_DEFAULT gst_surface_mem_sink_debug_category

static void gst_surface_mem_sink_class_init(GstSurfaceMemSinkClass *klass)
{
    g_return_if_fail(klass != nullptr);

    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstMemSinkClass *mem_sink_class = GST_MEM_SINK_CLASS(klass);
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

    gst_element_class_add_static_pad_template (element_class, &g_sinktemplate);

    gobject_class->dispose = gst_surface_mem_sink_dispose;
    gobject_class->finalize = gst_surface_mem_sink_finalize;
    gobject_class->set_property = gst_surface_mem_sink_set_property;
    gobject_class->get_property = gst_surface_mem_sink_get_property;

    gst_element_class_set_static_metadata(element_class,
        "SurfaceMemSink", "Sink/Video",
        "Output to surface buffer and allow the application to get access to the surface buffer",
        "OpenHarmony");

    g_object_class_install_property(gobject_class, PROP_SURFACE,
        g_param_spec_pointer("surface", "Surface",
            "Surface for rendering output",
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    mem_sink_class->do_propose_allocation = gst_surface_mem_sink_do_propose_allocation;
    mem_sink_class->do_app_render = gst_surface_mem_sink_do_app_render;

    GST_DEBUG_CATEGORY_INIT(gst_surface_mem_sink_debug_category, "surfacesink", 0, "surfacesink class");
}

static void gst_surface_mem_sink_init(GstSurfaceMemSink *sink)
{
    g_return_if_fail(sink != nullptr);

    auto priv = reinterpret_cast<GstSurfaceMemSinkPrivate *>(gst_surface_mem_sink_get_instance_private(sink));
    g_return_if_fail(priv != nullptr);
    sink->priv = priv;
    sink->priv->surface = nullptr;
    sink->priv->pool = GST_SURFACE_POOL_CAST(gst_surface_pool_new());
    GstMemSink *memSink = GST_MEM_SINK_CAST(sink);
    memSink->max_pool_capacity = DEFAULT_SURFACE_MAX_POOL_CAPACITY;
}

static void gst_surface_mem_sink_dispose(GObject *obj)
{
    g_return_if_fail(obj != nullptr);

    GstSurfaceMemSink *surface_sink = GST_SURFACE_MEM_SINK_CAST(obj);
    GstSurfaceMemSinkPrivate *priv = surface_sink->priv;
    g_return_if_fail(priv != nullptr);

    GST_OBJECT_LOCK(surface_sink);
    priv->surface = nullptr;
    if (priv->pool != nullptr) {
        gst_object_unref(priv->pool);
        priv->pool = nullptr;
    }
    GST_OBJECT_UNLOCK(surface_sink);

    G_OBJECT_CLASS(parent_class)->dispose(obj);
}

static void gst_surface_mem_sink_finalize(GObject *obj)
{
    g_return_if_fail(obj != nullptr);
    G_OBJECT_CLASS(parent_class)->finalize(obj);
}

static void gst_surface_mem_sink_set_property(GObject *object, guint propId, const GValue *value, GParamSpec *pspec)
{
    g_return_if_fail(object != nullptr && value != nullptr);

    GstSurfaceMemSink *surface_sink = GST_SURFACE_MEM_SINK_CAST(object);
    GstSurfaceMemSinkPrivate *priv = surface_sink->priv;
    g_return_if_fail(priv != nullptr);

    switch (propId) {
        case PROP_SURFACE: {
            gpointer surface = g_value_get_pointer(value);
            g_return_if_fail(surface != nullptr);
            OHOS::sptr<OHOS::Surface> surface_ref = reinterpret_cast<OHOS::Surface *>(surface);
            GST_OBJECT_LOCK(surface_sink);
            priv->surface = surface_ref;
            GST_OBJECT_UNLOCK(surface_sink);
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
            break;
    }
}

static void gst_surface_mem_sink_get_property(GObject *object, guint propId, GValue *value, GParamSpec *pspec)
{
    g_return_if_fail(object != nullptr);

    GstSurfaceMemSink *surface_sink = GST_SURFACE_MEM_SINK_CAST(object);
    GstSurfaceMemSinkPrivate *priv = surface_sink->priv;
    g_return_if_fail(priv != nullptr);

    switch (propId) {
        case PROP_SURFACE: {
            GST_OBJECT_LOCK(surface_sink);
            g_return_if_fail(priv->surface != nullptr);
            g_value_set_pointer(value, priv->surface.GetRefPtr());
            GST_OBJECT_UNLOCK(surface_sink);
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
            break;
    }
}

static GstFlowReturn gst_surface_mem_sink_do_app_render(GstMemSink *memsink, GstBuffer *buffer)
{
    g_return_val_if_fail(memsink != nullptr && buffer != nullptr, GST_FLOW_ERROR);
    GstSurfaceMemSink *surface_sink = GST_SURFACE_MEM_SINK_CAST(memsink);
    g_return_val_if_fail(surface_sink != nullptr, GST_FLOW_ERROR);
    GstSurfaceMemSinkPrivate *priv = surface_sink->priv;
    GST_OBJECT_LOCK(surface_sink);
    for (guint i = 0; i < gst_buffer_n_memory(buffer); i++) {
        GstMemory *memory = gst_buffer_peek_memory(buffer, i);
        if (!gst_is_surface_memory(memory)) {
            GST_WARNING_OBJECT(surface_sink, "not surface buffer !, 0x%06" PRIXPTR, FAKE_POINTER(memory));
            continue;
        }

        GstSurfaceMemory *surface_mem = reinterpret_cast<GstSurfaceMemory *>(memory);
        surface_mem->needRender = TRUE;

        OHOS::BufferFlushConfig flushConfig = {
            { 0, 0, surface_mem->buf->GetWidth(), surface_mem->buf->GetHeight() },
        };
        OHOS::SurfaceError ret = priv->surface->FlushBuffer(surface_mem->buf, surface_mem->fence, flushConfig);
        if (ret != OHOS::SurfaceError::SURFACE_ERROR_OK) {
            // if it's paused, then play, this buffer is render by preroll, so it's ok
            GST_ERROR_OBJECT(surface_sink, "flush buffer to surface failed, %d", ret);
        }
    }

    GST_OBJECT_UNLOCK(surface_sink);
    GST_DEBUG_OBJECT(surface_sink, "End gst_surface_mem_sink_do_app_render");
    return GST_FLOW_OK;
}

static gboolean gst_surface_mem_sink_do_propose_allocation(GstMemSink *memsink, GstQuery *query)
{
    g_return_val_if_fail(memsink != nullptr && query != nullptr, FALSE);
    GstSurfaceMemSink *surface_sink = GST_SURFACE_MEM_SINK_CAST(memsink);
    g_return_val_if_fail(surface_sink != nullptr, FALSE);

    GstCaps *caps = nullptr;
    gboolean needPool = FALSE;
    gst_query_parse_allocation(query, &caps, &needPool);
    GST_DEBUG_OBJECT(surface_sink, "process allocation query, caps: %s", gst_caps_to_string(caps));

    if (!needPool) {
        GST_ERROR_OBJECT(surface_sink, "no need buffer pool, unexpected!");
        return FALSE;
    }

    GST_OBJECT_LOCK(surface_sink);

    guint size = 0;
    guint minBuffers = 0;
    guint maxBuffers = 0;
    gst_query_parse_nth_allocation_pool(query, 0, nullptr, &size, &minBuffers, &maxBuffers);
    if (maxBuffers == 0) {
        GST_INFO_OBJECT(surface_sink, "correct the maxbuffer from %u to %u", maxBuffers, memsink->max_pool_capacity);
        maxBuffers = memsink->max_pool_capacity;
    }
    GST_DEBUG("maxBuffers is: %u", maxBuffers);

    GstSurfacePool *pool = surface_sink->priv->pool;
    g_return_val_if_fail(pool != nullptr, FALSE);
    g_return_val_if_fail(gst_buffer_pool_set_active(GST_BUFFER_POOL(pool), FALSE), FALSE);
    (void)gst_surface_pool_set_surface(pool, surface_sink->priv->surface);

    GstVideoInfo info;
    GST_DEBUG("begin gst_video_info_from_caps");
    gboolean ret = gst_video_info_from_caps(&info, caps);
    g_return_val_if_fail(ret, FALSE);
    gst_query_add_allocation_pool(query, GST_BUFFER_POOL_CAST(pool), info.size, minBuffers, maxBuffers);

    GstSurfaceAllocator *allocator = gst_surface_allocator_new();
    g_return_val_if_fail(allocator != nullptr, FALSE);
    GstStructure *params = gst_structure_new("mem", "memtype", G_TYPE_STRING, "surface", nullptr);
    gst_query_add_allocation_param(query, GST_ALLOCATOR_CAST(allocator), nullptr);
    gst_query_add_allocation_meta(query, GST_VIDEO_META_API_TYPE, nullptr);
    gst_query_add_allocation_meta(query, GST_BUFFER_TYPE_META_API_TYPE, params);
    gst_structure_free(params);

    GstStructure *config = gst_buffer_pool_get_config(GST_BUFFER_POOL_CAST(pool));
    if (config == nullptr) {
        GST_OBJECT_UNLOCK(surface_sink);
        gst_object_unref(allocator);
        return FALSE;
    }

    gst_buffer_pool_config_set_params(config, caps, info.size, minBuffers, maxBuffers);
    gst_buffer_pool_config_set_allocator(config, GST_ALLOCATOR_CAST(allocator), nullptr);
    // set config will take ownership of the config, we dont need to free it.
    ret = gst_buffer_pool_set_config(GST_BUFFER_POOL_CAST(pool), config);

    gst_object_unref(allocator);
    GST_OBJECT_UNLOCK(surface_sink);
    return ret;
}
