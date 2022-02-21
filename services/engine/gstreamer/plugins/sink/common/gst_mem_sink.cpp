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
#include "gst_mem_sink.h"
#include <cinttypes>
#include "gst_surface_mem_sink.h"
#include "gst_shared_mem_sink.h"

#define POINTER_MASK 0x00FFFFFF
#define FAKE_POINTER(addr) (POINTER_MASK & reinterpret_cast<uintptr_t>(addr))

namespace {
    constexpr guint32 DEFAULT_PROP_MAX_POOL_CAPACITY = 5; // 0 is meanlessly
    constexpr guint32 DEFAULT_PROP_WAIT_TIME = 5000; // us, 0 is meanlessly
}

struct _GstMemSinkPrivate {
    GstCaps *caps;
    GMutex mutex;
    gboolean started;
    GstMemSinkCallbacks callbacks;
    gpointer userdata;
    GDestroyNotify notify;
};

enum {
    PROP_0,
    PROP_CAPS,
    PROP_MAX_POOL_CAPACITY,
    PROP_WAIT_TIME,
};

static GstStaticPadTemplate g_sinktemplate = GST_STATIC_PAD_TEMPLATE("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static void gst_mem_sink_dispose(GObject *obj);
static void gst_mem_sink_finalize(GObject *object);
static void gst_mem_sink_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_mem_sink_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static GstCaps *gst_mem_sink_get_caps(GstBaseSink *basesink, GstCaps *filter);
static gboolean gst_mem_sink_set_caps(GstBaseSink *basesink, GstCaps *caps);
static GstCaps *gst_mem_sink_getcaps(GstMemSink *mem_sink);
static void gst_mem_sink_setcaps(GstMemSink *mem_sink, const GstCaps *caps);
static gboolean gst_mem_sink_event(GstBaseSink *basesink, GstEvent *event);
static gboolean gst_mem_sink_query(GstBaseSink *basesink, GstQuery *query);
static gboolean gst_mem_sink_start(GstBaseSink *basesink);
static gboolean gst_mem_sink_stop(GstBaseSink *basesink);
static GstFlowReturn gst_mem_sink_preroll(GstBaseSink *basesink, GstBuffer *buffer);
static GstFlowReturn gst_mem_sink_stream_render(GstBaseSink *basesink, GstBuffer *buffer);
static gboolean gst_mem_sink_propose_allocation(GstBaseSink *bsink, GstQuery *query);

#define gst_mem_sink_parent_class parent_class
G_DEFINE_ABSTRACT_TYPE_WITH_CODE(GstMemSink, gst_mem_sink, GST_TYPE_BASE_SINK, G_ADD_PRIVATE(GstMemSink));

GST_DEBUG_CATEGORY_STATIC(gst_mem_sink_debug_category);
#define GST_CAT_DEFAULT gst_mem_sink_debug_category

static void gst_mem_sink_class_init(GstMemSinkClass *klass)
{
    g_return_if_fail(klass != nullptr);

    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstBaseSinkClass *base_sink_class = GST_BASE_SINK_CLASS(klass);
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

    gst_element_class_add_static_pad_template (element_class, &g_sinktemplate);

    gst_element_class_set_static_metadata(element_class,
        "MemSink", "Generic/Sink",
        "Output to memory and allow the application to get access to the memory",
        "OpenHarmony");

    gobject_class->dispose = gst_mem_sink_dispose;
    gobject_class->finalize = gst_mem_sink_finalize;
    gobject_class->set_property = gst_mem_sink_set_property;
    gobject_class->get_property = gst_mem_sink_get_property;

    g_object_class_install_property(gobject_class, PROP_CAPS,
        g_param_spec_boxed ("caps", "Caps",
            "The allowed caps for the sink pad", GST_TYPE_CAPS,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_MAX_POOL_CAPACITY,
        g_param_spec_uint("max-pool-capacity", "Max Pool Capacity",
            "The maximum capacity of the buffer pool (0 == meanlessly)",
            0, G_MAXUINT, DEFAULT_PROP_MAX_POOL_CAPACITY,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_WAIT_TIME,
        g_param_spec_uint("wait-time", "Wait Time",
            "The longest waiting time for single try to acquire buffer from buffer pool (0 == meanlessly)",
            0, G_MAXUINT, DEFAULT_PROP_MAX_POOL_CAPACITY,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    base_sink_class->start = gst_mem_sink_start;
    base_sink_class->stop = gst_mem_sink_stop;
    base_sink_class->preroll = gst_mem_sink_preroll;
    base_sink_class->render = gst_mem_sink_stream_render;
    base_sink_class->get_caps = gst_mem_sink_get_caps;
    base_sink_class->set_caps = gst_mem_sink_set_caps;
    base_sink_class->query = gst_mem_sink_query;
    base_sink_class->event = gst_mem_sink_event;
    base_sink_class->propose_allocation = gst_mem_sink_propose_allocation;

    GST_DEBUG_CATEGORY_INIT(gst_mem_sink_debug_category, "memsink", 0, "memsink class");
}

static void gst_mem_sink_init(GstMemSink *mem_sink)
{
    g_return_if_fail(mem_sink != nullptr);

    auto priv = reinterpret_cast<GstMemSinkPrivate *>(gst_mem_sink_get_instance_private(mem_sink));
    g_return_if_fail(priv != nullptr);
    mem_sink->priv = priv;

    g_mutex_init(&priv->mutex);

    mem_sink->max_pool_capacity = DEFAULT_PROP_MAX_POOL_CAPACITY;
    mem_sink->wait_time = DEFAULT_PROP_WAIT_TIME;
    priv->started = FALSE;
    priv->caps = nullptr;
    priv->callbacks.eos = nullptr;
    priv->callbacks.new_preroll = nullptr;
    priv->callbacks.new_sample = nullptr;
    priv->userdata = nullptr;
    priv->notify = nullptr;
}

static void gst_mem_sink_dispose(GObject *obj)
{
    g_return_if_fail(obj != nullptr);

    GstMemSink *mem_sink = GST_MEM_SINK_CAST(obj);
    GstMemSinkPrivate *priv = mem_sink->priv;
    g_return_if_fail(priv != nullptr);

    GST_OBJECT_LOCK(mem_sink);
    if (priv->caps != nullptr) {
        gst_caps_unref(priv->caps);
        priv->caps = nullptr;
    }
    if (priv->notify != nullptr) {
        priv->notify(priv->userdata);
        priv->userdata = nullptr;
        priv->notify = nullptr;
    }
    GST_OBJECT_UNLOCK(mem_sink);

    G_OBJECT_CLASS(parent_class)->dispose(obj);
}

static void gst_mem_sink_finalize(GObject *obj)
{
    g_return_if_fail(obj != nullptr);

    GstMemSink *mem_sink = GST_MEM_SINK_CAST(obj);
    GstMemSinkPrivate *priv = mem_sink->priv;
    g_return_if_fail(priv != nullptr);

    g_mutex_clear (&priv->mutex);

    G_OBJECT_CLASS(parent_class)->finalize(obj);
}

static void gst_mem_sink_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    g_return_if_fail(object != nullptr);

    GstMemSink *mem_sink = GST_MEM_SINK_CAST(object);
    GstMemSinkPrivate *priv = mem_sink->priv;
    g_return_if_fail(priv != nullptr);

    switch (prop_id) {
        case PROP_CAPS:
            gst_mem_sink_setcaps(mem_sink, gst_value_get_caps(value));
            break;
        case PROP_MAX_POOL_CAPACITY: {
            GST_OBJECT_LOCK(mem_sink);
            guint max_pool_capacity = g_value_get_uint(value);
            if (max_pool_capacity != 0) {
                mem_sink->max_pool_capacity = max_pool_capacity;
            }
            GST_DEBUG_OBJECT(mem_sink, "set max pool capacity: %u", mem_sink->max_pool_capacity);
            GST_OBJECT_UNLOCK(mem_sink);
            break;
        }
        case PROP_WAIT_TIME: {
            GST_OBJECT_LOCK(mem_sink);
            mem_sink->wait_time = g_value_get_uint(value);
            GST_DEBUG_OBJECT(mem_sink, "set wait time: %d", mem_sink->wait_time);
            GST_OBJECT_UNLOCK(mem_sink);
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void gst_mem_sink_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    g_return_if_fail(object != nullptr);

    GstMemSink *mem_sink = GST_MEM_SINK_CAST(object);
    GstMemSinkPrivate *priv = mem_sink->priv;
    g_return_if_fail(priv != nullptr);

    switch (prop_id) {
        case PROP_CAPS: {
            GstCaps *caps = gst_mem_sink_getcaps(mem_sink);
            gst_value_set_caps(value, caps);
            if (caps != nullptr) {
                gst_caps_unref(caps);
            }
            break;
        }
        case PROP_MAX_POOL_CAPACITY: {
            GST_OBJECT_LOCK(mem_sink);
            g_value_set_uint(value, mem_sink->max_pool_capacity);
            GST_OBJECT_UNLOCK(mem_sink);
            break;
        }
        case PROP_WAIT_TIME: {
            GST_OBJECT_LOCK(mem_sink);
            g_value_set_uint(value, mem_sink->wait_time);
            GST_OBJECT_UNLOCK(mem_sink);
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

void gst_mem_sink_set_callback(GstMemSink *mem_sink, GstMemSinkCallbacks *callbacks,
                               gpointer userdata, GDestroyNotify notify)
{
    g_return_if_fail(GST_IS_MEM_SINK(mem_sink));
    g_return_if_fail(callbacks != nullptr);
    GstMemSinkPrivate *priv = mem_sink->priv;
    g_return_if_fail(priv != nullptr);

    GST_OBJECT_LOCK(mem_sink);
    GDestroyNotify old_notify = priv->notify;
    if (old_notify) {
        gpointer old_data = priv->userdata;
        priv->userdata = nullptr;
        priv->notify = nullptr;

        GST_OBJECT_UNLOCK(mem_sink);
        old_notify(old_data);
        GST_OBJECT_LOCK(mem_sink);
    }

    priv->callbacks = *callbacks;
    priv->userdata = userdata;
    priv->notify = notify;
    GST_OBJECT_UNLOCK(mem_sink);
}

static void gst_mem_sink_setcaps(GstMemSink *mem_sink, const GstCaps *caps)
{
    GstMemSinkPrivate *priv = mem_sink->priv;
    g_return_if_fail(priv != nullptr);

    GST_OBJECT_LOCK(mem_sink);
    GST_INFO_OBJECT(mem_sink, "setting caps to %s", gst_caps_to_string(caps));

    GstCaps *old = priv->caps;
    if (old != caps) {
        if (caps != nullptr) {
            priv->caps = gst_caps_copy(caps);
        } else {
            priv->caps = nullptr;
        }
        if (old != nullptr) {
            gst_caps_unref(old);
        }
    }

    GST_OBJECT_UNLOCK(mem_sink);
}

static GstCaps *gst_mem_sink_getcaps(GstMemSink *mem_sink)
{
    GstMemSinkPrivate *priv = mem_sink->priv;
    g_return_val_if_fail(priv != nullptr, nullptr);

    GST_OBJECT_LOCK(mem_sink);
    GstCaps *caps = priv->caps;
    if (caps != nullptr) {
        gst_caps_ref(caps);
        GST_INFO_OBJECT(mem_sink, "getting caps of %s", gst_caps_to_string(caps));
    }
    GST_OBJECT_UNLOCK(mem_sink);

    return caps;
}

static gboolean gst_mem_sink_start(GstBaseSink *bsink)
{
    GstMemSink *mem_sink = GST_MEM_SINK(bsink);
    g_return_val_if_fail(mem_sink != nullptr, FALSE);
    GstMemSinkPrivate *priv = mem_sink->priv;
    g_return_val_if_fail(priv != nullptr, FALSE);

    g_mutex_lock(&priv->mutex);
    GST_INFO_OBJECT(mem_sink, "starting");
    priv->started = TRUE;
    g_mutex_unlock(&priv->mutex);

    return TRUE;
}

static gboolean gst_mem_sink_stop(GstBaseSink *bsink)
{
    GstMemSink *mem_sink = GST_MEM_SINK(bsink);
    g_return_val_if_fail(mem_sink != nullptr, FALSE);
    GstMemSinkPrivate *priv = mem_sink->priv;
    g_return_val_if_fail(priv != nullptr, FALSE);

    g_mutex_lock(&priv->mutex);
    GST_INFO_OBJECT(mem_sink, "stopping");
    priv->started = FALSE;
    g_mutex_unlock(&priv->mutex);

    return TRUE;
}

static gboolean gst_mem_sink_event(GstBaseSink *bsink, GstEvent *event)
{
    GstMemSink *mem_sink = GST_MEM_SINK_CAST(bsink);
    g_return_val_if_fail(mem_sink != nullptr, FALSE);
    GstMemSinkPrivate *priv = mem_sink->priv;
    g_return_val_if_fail(priv != nullptr, FALSE);

    switch (event->type) {
        case GST_EVENT_EOS: {
            GST_INFO_OBJECT(mem_sink, "receiving EOS");
            if (priv->callbacks.eos != nullptr) {
                priv->callbacks.eos(mem_sink, priv->userdata);
            }
            break;
        }
        default:
            break;
    }
    return GST_BASE_SINK_CLASS(parent_class)->event(bsink, event);
}

static gboolean gst_mem_sink_query(GstBaseSink *bsink, GstQuery *query)
{
    GstMemSink *mem_sink = GST_MEM_SINK_CAST(bsink);
    g_return_val_if_fail(mem_sink != nullptr, FALSE);
    GstMemSinkPrivate *priv = mem_sink->priv;
    g_return_val_if_fail(priv != nullptr, FALSE);

    gboolean ret;
    switch (GST_QUERY_TYPE(query)) {
        case GST_QUERY_SEEKING: {
            GstFormat fmt;
            gst_query_parse_seeking(query, &fmt, nullptr, nullptr, nullptr);
            gst_query_set_seeking(query, fmt, FALSE, 0, -1);
            ret = TRUE;
            break;
        }
        default:
            ret = GST_BASE_SINK_CLASS(parent_class)->query(bsink, query);
            break;
    }

    return ret;
}

static gboolean gst_mem_sink_set_caps(GstBaseSink *sink, GstCaps *caps)
{
    GstMemSink *mem_sink = GST_MEM_SINK_CAST(sink);
    g_return_val_if_fail(mem_sink != nullptr, FALSE);
    GstMemSinkPrivate *priv = mem_sink->priv;
    g_return_val_if_fail(priv != nullptr, FALSE);

    GST_OBJECT_LOCK(mem_sink);

    gboolean ret = FALSE;
    GST_INFO_OBJECT(mem_sink, "receiving CAPS %s", gst_caps_to_string(caps));
    if (caps != nullptr && priv->caps != nullptr) {
        caps = gst_caps_intersect_full(priv->caps, caps, GST_CAPS_INTERSECT_FIRST);
        if (caps != nullptr) {
            GST_INFO_OBJECT(mem_sink, "received caps");
            gst_caps_unref(caps);
            ret = TRUE;
        }
    }

    GST_OBJECT_UNLOCK(mem_sink);
    return ret;
}

static GstCaps *gst_mem_sink_get_caps(GstBaseSink *sink, GstCaps *filter)
{
    GstMemSink *mem_sink = GST_MEM_SINK_CAST(sink);
    g_return_val_if_fail(mem_sink != nullptr, nullptr);
    GstMemSinkPrivate *priv = mem_sink->priv;
    g_return_val_if_fail(priv != nullptr, nullptr);

    GST_OBJECT_LOCK(mem_sink);
    GstCaps *caps = priv->caps;
    if (caps != nullptr) {
        if (filter != nullptr) {
            caps = gst_caps_intersect_full(filter, caps, GST_CAPS_INTERSECT_FIRST);
        } else {
            gst_caps_ref(caps);
        }
        GST_INFO_OBJECT(mem_sink, "got caps %s", gst_caps_to_string(caps));
    }
    GST_OBJECT_UNLOCK(mem_sink);

    return caps;
}

static GstFlowReturn gst_mem_sink_preroll(GstBaseSink *bsink, GstBuffer *buffer)
{
    GstMemSink *mem_sink = GST_MEM_SINK_CAST(bsink);
    g_return_val_if_fail(mem_sink != nullptr, GST_FLOW_ERROR);
    GstMemSinkPrivate *priv = mem_sink->priv;
    g_return_val_if_fail(priv != nullptr, GST_FLOW_ERROR);

    g_mutex_lock(&priv->mutex);
    if (!priv->started) {
        GST_INFO_OBJECT(mem_sink, "we are not started");
        g_mutex_unlock(&priv->mutex);
        return GST_FLOW_FLUSHING;
    }
    g_mutex_unlock(&priv->mutex);

    GST_INFO_OBJECT(mem_sink, "preroll buffer 0x%06" PRIXPTR "", FAKE_POINTER(buffer));

    GstFlowReturn ret = GST_FLOW_OK;
    if (priv->callbacks.new_preroll != nullptr) {
        ret = priv->callbacks.new_preroll(mem_sink, buffer, priv->userdata);
    }

    return ret;
}

static GstFlowReturn gst_mem_sink_stream_render(GstBaseSink *bsink, GstBuffer *buffer)
{
    GstMemSink *mem_sink = GST_MEM_SINK_CAST(bsink);
    g_return_val_if_fail(mem_sink != nullptr && buffer != nullptr, GST_FLOW_ERROR);
    GstMemSinkPrivate *priv = mem_sink->priv;
    g_return_val_if_fail(priv != nullptr, GST_FLOW_ERROR);
    GstMemSinkClass *mem_sink_class = GST_MEM_SINK_GET_CLASS(mem_sink);
    g_return_val_if_fail(mem_sink_class != nullptr, GST_FLOW_ERROR);

    g_mutex_lock(&priv->mutex);
    if (!priv->started) {
        GST_INFO_OBJECT(mem_sink, "we are not started");
        g_mutex_unlock(&priv->mutex);
        return GST_FLOW_FLUSHING;
    }
    g_mutex_unlock(&priv->mutex);

    GST_INFO_OBJECT(mem_sink, "stream render buffer 0x%06" PRIXPTR "", FAKE_POINTER(buffer));

    GstFlowReturn ret = GST_FLOW_OK;
    if (mem_sink_class->do_stream_render != nullptr) {
        ret = mem_sink_class->do_stream_render(mem_sink, &buffer);
        g_return_val_if_fail(ret == GST_FLOW_OK, ret);
    }

    if (priv->callbacks.new_sample != nullptr) {
        ret = priv->callbacks.new_sample(mem_sink, buffer, priv->userdata);
    }

    // the basesink will unref the buffer.
    return ret;
}

static gboolean gst_mem_sink_propose_allocation(GstBaseSink *bsink, GstQuery *query)
{
    GstMemSink *mem_sink = GST_MEM_SINK_CAST(bsink);
    g_return_val_if_fail(mem_sink != nullptr && query != nullptr, FALSE);
    GstMemSinkClass *mem_sink_class = GST_MEM_SINK_GET_CLASS(mem_sink);
    g_return_val_if_fail(mem_sink_class != nullptr, FALSE);

    if (mem_sink_class->do_propose_allocation != nullptr) {
        return mem_sink_class->do_propose_allocation(mem_sink, query);
    }

    return FALSE;
}

GstFlowReturn gst_mem_sink_app_render(GstMemSink *mem_sink, GstBuffer *buffer)
{
    g_return_val_if_fail(mem_sink != nullptr, GST_FLOW_ERROR);
    GstMemSinkPrivate *priv = mem_sink->priv;
    g_return_val_if_fail(priv != nullptr, GST_FLOW_ERROR);
    GstMemSinkClass *mem_sink_class = GST_MEM_SINK_GET_CLASS(mem_sink);
    g_return_val_if_fail(mem_sink_class != nullptr, GST_FLOW_ERROR);

    g_mutex_lock(&priv->mutex);
    if (!priv->started) {
        GST_INFO_OBJECT(mem_sink, "we are not started");
        g_mutex_unlock(&priv->mutex);
        return GST_FLOW_FLUSHING;
    }
    g_mutex_unlock(&priv->mutex);

    GST_INFO_OBJECT(mem_sink, "app render buffer 0x%06" PRIXPTR "", FAKE_POINTER(buffer));

    GstFlowReturn ret = GST_FLOW_OK;
    if (mem_sink_class->do_app_render != nullptr) {
        ret = mem_sink_class->do_app_render(mem_sink, buffer);
    }

    return ret;
}
