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

#include "gst_mem_pool_src.h"
#include <gst/video/video.h>
#include "media_errors.h"
#include "scope_guard.h"

using namespace OHOS;
namespace {
    constexpr int32_t DEFAULT_BUFFER_SIZE = 41920;
    constexpr int32_t DEFAULT_BUFFER_NUM = 8;
}

#define gst_mem_pool_src_parent_class parent_class

GST_DEBUG_CATEGORY_STATIC(gst_mem_pool_src_debug_category);
#define GST_CAT_DEFAULT gst_mem_pool_src_debug_category

struct _GstMemPoolSrcPrivate {
    BufferAvailable buffer_available;
    gboolean emit_signals;
    gpointer user_data;
    GDestroyNotify notify;
};

enum {
    /* signals */
    SIGNAL_BUFFER_AVAILABLE,
    /* actions */
    SIGNAL_PULL_BUFFER,
    SIGNAL_PUSH_BUFFER,
    LAST_SIGNAL
};

enum {
    PROP_0,
    PROP_CAPS,
    PROP_EMIT_SIGNALS,
    PROP_BUFFER_NUM,
    PROP_BUFFER_SIZE,
};

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE(GstMemPoolSrc, gst_mem_pool_src, GST_TYPE_BASE_SRC);

static guint gst_mem_pool_src_signals[LAST_SIGNAL] = { 0 };
static void gst_mem_pool_src_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_mem_pool_src_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static gboolean gst_mem_pool_src_query(GstBaseSrc *src, GstQuery *query);
static gboolean gst_mem_pool_src_is_seekable(GstBaseSrc *basesrc);
static gboolean gst_mem_pool_src_negotiate(GstBaseSrc *basesrc);
static void gst_mem_pool_src_dispose(GObject *object);

static void gst_mem_pool_src_class_init(GstMemPoolSrcClass *klass)
{
    g_return_if_fail(klass != nullptr);
    GObjectClass *gobject_class = reinterpret_cast<GObjectClass *>(klass);
    GstBaseSrcClass *gstbasesrc_class = reinterpret_cast<GstBaseSrcClass *>(klass);
    GST_DEBUG_CATEGORY_INIT(gst_mem_pool_src_debug_category, "mempoolsrc", 0, "mem pool src base class");
    gobject_class->set_property = gst_mem_pool_src_set_property;
    gobject_class->get_property = gst_mem_pool_src_get_property;
    gobject_class->dispose = gst_mem_pool_src_dispose;

    g_object_class_install_property(gobject_class, PROP_BUFFER_SIZE,
        g_param_spec_uint("buffer-size", "buffer size",
            "buffer size", 0, G_MAXINT32, 0,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_BUFFER_NUM,
        g_param_spec_uint("buffer-num", "buffer num",
            "buffer num", 0, G_MAXINT32, 0,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_CAPS,
        g_param_spec_boxed("caps", "Caps",
            "The allowed caps for the src pad", GST_TYPE_CAPS,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_EMIT_SIGNALS,
        g_param_spec_boolean("emit-signals", "Emit signals",
            "Emit new-preroll and new-sample signals", FALSE,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    gst_mem_pool_src_signals[SIGNAL_BUFFER_AVAILABLE] =
        g_signal_new("buffer-available", G_TYPE_FROM_CLASS(gobject_class), G_SIGNAL_RUN_LAST,
            0, nullptr, nullptr, nullptr, GST_TYPE_FLOW_RETURN, 0, G_TYPE_NONE);

    gst_mem_pool_src_signals[SIGNAL_PULL_BUFFER] =
        g_signal_new("pull-buffer", G_TYPE_FROM_CLASS(gobject_class),
            static_cast<GSignalFlags>(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            G_STRUCT_OFFSET(GstMemPoolSrcClass, pull_buffer), nullptr, nullptr, nullptr,
            GST_TYPE_BUFFER, 0, G_TYPE_NONE);

    gst_mem_pool_src_signals[SIGNAL_PUSH_BUFFER] =
        g_signal_new("push-buffer", G_TYPE_FROM_CLASS(gobject_class),
            static_cast<GSignalFlags>(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            G_STRUCT_OFFSET(GstMemPoolSrcClass, push_buffer), nullptr, nullptr, nullptr,
            GST_TYPE_FLOW_RETURN, 1, GST_TYPE_BUFFER);

    gstbasesrc_class->is_seekable = gst_mem_pool_src_is_seekable;
    gstbasesrc_class->query = gst_mem_pool_src_query;
    gstbasesrc_class->negotiate = gst_mem_pool_src_negotiate;
}

static void gst_mem_pool_src_init(GstMemPoolSrc *memsrc)
{
    g_return_if_fail(memsrc != nullptr);
    auto priv = reinterpret_cast<GstMemPoolSrcPrivate *>(gst_mem_pool_src_get_instance_private(memsrc));
    g_return_if_fail(priv != nullptr);
    gst_base_src_set_format(GST_BASE_SRC(memsrc), GST_FORMAT_TIME);
    gst_base_src_set_live(GST_BASE_SRC(memsrc), TRUE);
    memsrc->buffer_size = DEFAULT_BUFFER_SIZE;
    memsrc->buffer_num = DEFAULT_BUFFER_NUM;
    memsrc->priv = priv;
    priv->buffer_available = nullptr;
    priv->user_data = nullptr;
    priv->notify = nullptr;
    priv->emit_signals = FALSE;
}

GstFlowReturn gst_mem_pool_src_buffer_available(GstMemPoolSrc *memsrc)
{
    GST_DEBUG_OBJECT(memsrc, "Buffer available");
    g_return_val_if_fail(memsrc != nullptr && memsrc->priv != nullptr, GST_FLOW_ERROR);
    GstFlowReturn ret = GST_FLOW_OK;
    auto priv = memsrc->priv;
    gboolean emit = FALSE;
    if (priv->buffer_available) {
        ret = priv->buffer_available(memsrc, priv->user_data);
    } else {
        GST_OBJECT_LOCK(memsrc);
        emit = priv->emit_signals;
        GST_OBJECT_UNLOCK(memsrc);
        if (emit) {
            g_signal_emit(memsrc, gst_mem_pool_src_signals[SIGNAL_BUFFER_AVAILABLE], 0, &ret);
        }
    }
    return ret;
}

static gboolean gst_mem_pool_src_negotiate(GstBaseSrc *basesrc)
{
    g_return_val_if_fail(basesrc != nullptr, FALSE);
    GstMemPoolSrc *memsrc = GST_MEM_POOL_SRC(basesrc);
    return gst_base_src_set_caps(basesrc, memsrc->caps);
}

void gst_mem_pool_src_set_caps(GstMemPoolSrc *memsrc, const GstCaps *caps)
{
    g_return_if_fail(memsrc != nullptr);
    GST_DEBUG_OBJECT(memsrc, "Setting caps to %s", gst_caps_to_string(caps));
    GST_OBJECT_LOCK(memsrc);
    GstCaps *old_caps = memsrc->caps;
    if (caps != nullptr) {
        memsrc->caps = gst_caps_copy(caps);
    } else {
        memsrc->caps = nullptr;
    }
    if (old_caps != nullptr) {
        gst_caps_unref(old_caps);
    }
    GST_OBJECT_UNLOCK(memsrc);
}

void gst_mem_pool_src_set_emit_signals(GstMemPoolSrc *memsrc, gboolean emit)
{
    g_return_if_fail(memsrc != nullptr && memsrc->priv != nullptr);
    auto priv = memsrc->priv;
    GST_OBJECT_LOCK(memsrc);
    priv->emit_signals = emit;
    GST_OBJECT_UNLOCK(memsrc);
}

void gst_mem_pool_src_set_buffer_size(GstMemPoolSrc *memsrc, guint size)
{
    g_return_if_fail(memsrc != nullptr);
    GST_OBJECT_LOCK(memsrc);
    memsrc->buffer_size = size;
    GST_OBJECT_UNLOCK(memsrc);
}

void gst_mem_pool_src_set_buffer_num(GstMemPoolSrc *memsrc, guint num)
{
    g_return_if_fail(memsrc != nullptr);
    GST_OBJECT_LOCK(memsrc);
    memsrc->buffer_num = num;
    GST_OBJECT_UNLOCK(memsrc);
}

static gboolean gst_mem_pool_src_is_seekable(GstBaseSrc *basesrc)
{
    (void)basesrc;
    return FALSE;
}

static void gst_mem_pool_src_dispose(GObject *object)
{
    GstMemPoolSrc *memsrc = GST_MEM_POOL_SRC(object);
    g_return_if_fail(memsrc != nullptr && memsrc->priv != nullptr);
    auto priv = memsrc->priv;

    GST_OBJECT_LOCK(memsrc);
    if (memsrc->caps) {
        gst_caps_unref(memsrc->caps);
        memsrc->caps = nullptr;
    }
    if (priv->notify) {
        priv->notify(priv->user_data);
    }
    priv->user_data = nullptr;
    priv->notify = nullptr;
    GST_OBJECT_UNLOCK(memsrc);

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void gst_mem_pool_src_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    GstMemPoolSrc *memsrc = GST_MEM_POOL_SRC(object);
    g_return_if_fail(memsrc != nullptr);
    (void)pspec;
    switch (prop_id) {
        case PROP_CAPS:
            gst_mem_pool_src_set_caps(memsrc, gst_value_get_caps(value));
            break;
        case PROP_EMIT_SIGNALS:
            gst_mem_pool_src_set_emit_signals(memsrc, g_value_get_boolean(value));
            break;
        case PROP_BUFFER_SIZE:
            gst_mem_pool_src_set_buffer_size(memsrc, g_value_get_uint(value));
            break;
        case PROP_BUFFER_NUM:
            gst_mem_pool_src_set_buffer_num(memsrc, g_value_get_uint(value));
            break;
        default:
            break;
    }
}

static void gst_mem_pool_src_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GstMemPoolSrc *memsrc = GST_MEM_POOL_SRC(object);
    g_return_if_fail(memsrc != nullptr);
    g_return_if_fail(value != nullptr);
    (void)pspec;
    switch (prop_id) {
        case PROP_BUFFER_SIZE:
            g_value_set_uint(value, memsrc->buffer_size);
            GST_DEBUG_OBJECT(object, "set buffer size: %u", memsrc->buffer_size);
            break;
        case PROP_BUFFER_NUM:
            g_value_set_uint(value, memsrc->buffer_num);
            GST_DEBUG_OBJECT(object, "set buffer num: %u", memsrc->buffer_num);
            break;
        default:
            break;
    }
}

static gboolean gst_mem_pool_src_query(GstBaseSrc *src, GstQuery *query)
{
    g_return_val_if_fail(src != nullptr, FALSE);
    g_return_val_if_fail(query != nullptr, FALSE);
    g_return_val_if_fail(GST_BASE_SRC_CLASS(parent_class) != nullptr, FALSE);
    switch (GST_QUERY_TYPE(query)) {
        case GST_QUERY_SCHEDULING: {
            gst_query_set_scheduling(query, GST_SCHEDULING_FLAG_SEQUENTIAL, 1, -1, 0);
            gst_query_add_scheduling_mode(query, GST_PAD_MODE_PUSH);
            return TRUE;
        }
        default: {
            return GST_BASE_SRC_CLASS(parent_class)->query(src, query);
        }
    }
    return TRUE;
}

void gst_mem_pool_src_set_callback(GstMemPoolSrc *memsrc, BufferAvailable callback,
    gpointer user_data, GDestroyNotify notify)
{
    g_return_if_fail(memsrc != nullptr && memsrc->priv != nullptr);
    auto priv = memsrc->priv;
    GST_OBJECT_LOCK(memsrc);
    priv->user_data = user_data;
    priv->notify = notify;
    priv->buffer_available = callback;
    GST_OBJECT_UNLOCK(memsrc);
}


GstBuffer *gst_mem_pool_src_pull_buffer(GstMemPoolSrc *memsrc)
{
    GST_DEBUG_OBJECT(memsrc, "Pull buffer");
    g_return_val_if_fail(memsrc != nullptr, nullptr);
    GstMemPoolSrcClass *memclass = GST_MEM_POOL_SRC_GET_CLASS(memsrc);

    g_return_val_if_fail(memclass != nullptr, nullptr);
    if (memclass->pull_buffer) {
        return memclass->pull_buffer(memsrc);
    }
    GST_ERROR_OBJECT(memsrc, "there is no pull buffer function");
    return nullptr;
}

GstFlowReturn gst_mem_pool_src_push_buffer(GstMemPoolSrc *memsrc, GstBuffer *buffer)
{
    GST_DEBUG_OBJECT(memsrc, "Push buffer");
    g_return_val_if_fail(memsrc != nullptr, GST_FLOW_ERROR);
    GstMemPoolSrcClass *memclass = GST_MEM_POOL_SRC_GET_CLASS(memsrc);

    g_return_val_if_fail(memclass != nullptr, GST_FLOW_ERROR);
    if (memclass->push_buffer) {
        return memclass->push_buffer(memsrc, buffer);
    }
    GST_ERROR_OBJECT(memsrc, "there is no push buffer function");
    return GST_FLOW_ERROR;
}