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

#include "gst_video_shmem_sink.h"
#include <cinttypes>
#include "gst/base/gstqueuearray.h"
#include "gst/video/video-info.h"
#include "gst_video_shmem_pool.h"
#include "gst_shmem_allocator_old.h"
#include "media_log.h"

namespace {
    constexpr guint32 DEFAULT_INTERNEL_QUEUE_INIT_SIZE = 10;
    constexpr guint32 DEFAULT_PROP_MAX_POOL_CAPACITY = 0; // 0 means no limit
    constexpr guint32 DEFAULT_PROP_MEM_PREFIX = 0;
}

enum WaitStatus : guint32 {
    NO_WAITING = 0,
    USER_WAITING = 1 << 0,
    STREAM_WAITING = 1 << 1,
};

struct _GstVideoShMemSinkPrivate {
    GstCaps *caps;
    guint maxPoolCapacity;
    GstQueueArray *queue;
    GCond cond;
    GMutex mutex;
    GstBuffer *prerollBuffer;
    GstCaps *prerollCaps;
    GstCaps *lastCaps;
    GstSegment prerollSegment;
    GstSegment lastSegment;
    gboolean started;
    GstSample *sample;
    guint32 waitStatus;
    gboolean isEos;
    gboolean flushing;
    guint32 numBuffers;
    gboolean unlock;
    guint32 memPrefix;
};

enum {
    /* signals */
    SIGNAL_NEW_PREROLL,
    SIGNAL_NEW_SAMPLE,

    /* actions */
    SIGNAL_PULL_PREROLL,
    SIGNAL_PULL_SAMPLE,

    LAST_SIGNAL
};

enum {
    PROP_0,
    PROP_CAPS,
    PROP_MAX_POOL_CAPACITY,
    PROP_MEM_PREFIX, // allocate the memory with the required length's prefix.
};

static GstStaticPadTemplate g_sinktemplate = GST_STATIC_PAD_TEMPLATE("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static void gst_video_shmem_sink_dispose(GObject *obj);
static void gst_video_shmem_sink_finalize(GObject *object);
static void gst_video_shmem_sink_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_video_shmem_sink_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static GstCaps *gst_video_shmem_sink_get_caps(GstBaseSink *basesink, GstCaps *filter);
static gboolean gst_video_shmem_sink_set_caps(GstBaseSink *basesink, GstCaps *caps);
static GstCaps *gst_video_shmem_sink_getcaps(GstVideoShMemSink *vidShMemSink);
static void gst_video_shmem_sink_setcaps(GstVideoShMemSink *vidShMemSink, const GstCaps *caps);
static gboolean gst_video_shmem_sink_event(GstBaseSink *basesink, GstEvent *event);
static gboolean gst_video_shmem_sink_query(GstBaseSink *basesink, GstQuery *query);
static gboolean gst_video_shmem_sink_start(GstBaseSink *basesink);
static gboolean gst_video_shmem_sink_stop(GstBaseSink *basesink);
static gboolean gst_video_shmem_sink_unlock_start(GstBaseSink *bsink);
static gboolean gst_video_shmem_sink_unlock_stop(GstBaseSink *bsink);
static GstFlowReturn gst_video_shmem_sink_preroll(GstBaseSink *basesink, GstBuffer *buffer);
static GstFlowReturn gst_video_shmem_sink_render(GstBaseSink *basesink, GstBuffer *buffer);
static gboolean gst_video_shmem_sink_propose_allocation(GstBaseSink *bsink, GstQuery *query);
static GstSample *gst_video_shmem_sink_pull_preroll(GstVideoShMemSink *sink);
static GstSample *gst_video_shmem_sink_pull_sample(GstVideoShMemSink *sink);

static guint gst_video_shmem_sink_signals[LAST_SIGNAL] = { 0 };

#define gst_video_shmem_sink_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstVideoShMemSink, gst_video_shmem_sink,
    GST_TYPE_BASE_SINK, G_ADD_PRIVATE(GstVideoShMemSink));

static void gst_video_shmem_sink_setup_gobject_class(GObjectClass *gobjectClass)
{
    gobjectClass->dispose = gst_video_shmem_sink_dispose;
    gobjectClass->finalize = gst_video_shmem_sink_finalize;
    gobjectClass->set_property = gst_video_shmem_sink_set_property;
    gobjectClass->get_property = gst_video_shmem_sink_get_property;

    g_object_class_install_property(gobjectClass, PROP_CAPS,
        g_param_spec_boxed ("caps", "Caps",
            "The allowed caps for the sink pad", GST_TYPE_CAPS,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobjectClass, PROP_MAX_POOL_CAPACITY,
        g_param_spec_uint("max-pool-capacity", "Max Pool Capacity",
            "The maximum capacity of the buffer pool (0 == unlimited)",
            0, G_MAXUINT, DEFAULT_PROP_MAX_POOL_CAPACITY,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobjectClass, PROP_MEM_PREFIX,
        g_param_spec_uint("mem-prefix", "Memory Prefix",
            "Allocate the memory with required length's prefix (in bytes)",
            0, G_MAXUINT, DEFAULT_PROP_MEM_PREFIX,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
}

static void gst_video_shmem_sink_class_init(GstVideoShMemSinkClass *klass)
{
    g_return_if_fail(klass != nullptr);

    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);
    GstBaseSinkClass *baseSinkClass = GST_BASE_SINK_CLASS(klass);
    GstElementClass *elementClass = GST_ELEMENT_CLASS(klass);

    gst_element_class_add_static_pad_template (elementClass, &g_sinktemplate);

    gst_element_class_set_static_metadata(elementClass,
        "VideoShMemSink", "Sink/Video",
        "Output to ashmem memory and allow the application to get access to the ashmem memory",
        "OpenHarmony");

    gst_video_shmem_sink_setup_gobject_class(gobjectClass);

    gst_video_shmem_sink_signals[SIGNAL_NEW_PREROLL] =
        g_signal_new("new-preroll", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET(GstVideoShMemSinkClass, new_preroll),
            nullptr, nullptr, nullptr, GST_TYPE_FLOW_RETURN, 0, G_TYPE_NONE);

    gst_video_shmem_sink_signals[SIGNAL_NEW_SAMPLE] =
        g_signal_new("new-sample", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET(GstVideoShMemSinkClass, new_preroll),
            nullptr, nullptr, nullptr, GST_TYPE_FLOW_RETURN, 0, G_TYPE_NONE);

    gst_video_shmem_sink_signals[SIGNAL_PULL_PREROLL] =
        g_signal_new("pull-preroll", G_TYPE_FROM_CLASS(klass), (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            G_STRUCT_OFFSET(GstVideoShMemSinkClass, pull_preroll),
            nullptr, nullptr, nullptr, GST_TYPE_SAMPLE, 0, G_TYPE_NONE);

    gst_video_shmem_sink_signals[SIGNAL_PULL_SAMPLE] =
        g_signal_new("pull-sample", G_TYPE_FROM_CLASS(klass), (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            G_STRUCT_OFFSET(GstVideoShMemSinkClass, pull_sample),
            nullptr, nullptr, nullptr, GST_TYPE_SAMPLE, 0, G_TYPE_NONE);

    baseSinkClass->unlock = gst_video_shmem_sink_unlock_start;
    baseSinkClass->unlock_stop = gst_video_shmem_sink_unlock_stop;
    baseSinkClass->start = gst_video_shmem_sink_start;
    baseSinkClass->stop = gst_video_shmem_sink_stop;
    baseSinkClass->preroll = gst_video_shmem_sink_preroll;
    baseSinkClass->render = gst_video_shmem_sink_render;
    baseSinkClass->get_caps = gst_video_shmem_sink_get_caps;
    baseSinkClass->set_caps = gst_video_shmem_sink_set_caps;
    baseSinkClass->query = gst_video_shmem_sink_query;
    baseSinkClass->event = gst_video_shmem_sink_event;
    baseSinkClass->propose_allocation = gst_video_shmem_sink_propose_allocation;

    klass->pull_preroll = gst_video_shmem_sink_pull_preroll;
    klass->pull_sample = gst_video_shmem_sink_pull_sample;
}

static void gst_video_shmem_sink_init(GstVideoShMemSink *vidShmemSink)
{
    g_return_if_fail(vidShmemSink != nullptr);

    auto priv = reinterpret_cast<GstVideoShMemSinkPrivate *>(gst_video_shmem_sink_get_instance_private(vidShmemSink));
    g_return_if_fail(priv != nullptr);
    vidShmemSink->priv = priv;

    g_mutex_init(&priv->mutex);
    g_cond_init(&priv->cond);
    priv->queue = gst_queue_array_new(DEFAULT_INTERNEL_QUEUE_INIT_SIZE);
    priv->sample = gst_sample_new(nullptr, nullptr, nullptr, nullptr);

    priv->maxPoolCapacity = DEFAULT_PROP_MAX_POOL_CAPACITY;
    priv->started = FALSE;
    priv->prerollBuffer = nullptr;
    priv->prerollCaps = nullptr;
    priv->caps = nullptr;
    priv->waitStatus = WaitStatus::NO_WAITING;
    priv->isEos = FALSE;
    priv->flushing = FALSE;
    priv->numBuffers = 0;
    priv->unlock = FALSE;
    priv->memPrefix = DEFAULT_PROP_MEM_PREFIX;
}

static void gst_video_shmem_sink_dispose(GObject *obj)
{
    g_return_if_fail(obj != nullptr);

    GstVideoShMemSink *vidShmemSink = GST_VIDEO_SHMEM_SINK_CAST(obj);
    GstVideoShMemSinkPrivate *priv = vidShmemSink->priv;
    g_return_if_fail(priv != nullptr);

    GST_OBJECT_LOCK(vidShmemSink);
    if (priv->caps != nullptr) {
        gst_caps_unref(priv->caps);
        priv->caps = nullptr;
    }
    GST_OBJECT_UNLOCK(vidShmemSink);

    g_mutex_lock(&priv->mutex);
    GstMiniObject *queueObj = reinterpret_cast<GstMiniObject *>(gst_queue_array_pop_head(priv->queue));
    while (queueObj != nullptr) {
        gst_mini_object_unref(queueObj);
        queueObj = reinterpret_cast<GstMiniObject *>(gst_queue_array_pop_head(priv->queue));
    }
    priv->numBuffers = 0;
    (void)gst_buffer_replace(&priv->prerollBuffer, nullptr);
    (void)gst_caps_replace(&priv->prerollCaps, nullptr);
    (void)gst_caps_replace(&priv->lastCaps, nullptr);
    if (priv->sample != nullptr) {
        gst_sample_unref(priv->sample);
        priv->sample = nullptr;
    }
    g_mutex_unlock(&priv->mutex);

    G_OBJECT_CLASS(parent_class)->dispose(obj);
}

static void gst_video_shmem_sink_finalize(GObject *obj)
{
    g_return_if_fail(obj != nullptr);

    GstVideoShMemSink *vidShmemSink = GST_VIDEO_SHMEM_SINK_CAST(obj);
    GstVideoShMemSinkPrivate *priv = vidShmemSink->priv;
    g_return_if_fail(priv != nullptr);

    g_mutex_clear (&priv->mutex);
    g_cond_clear (&priv->cond);
    gst_queue_array_free(priv->queue);

    G_OBJECT_CLASS(parent_class)->finalize(obj);
}

static void gst_video_shmem_sink_set_property(GObject *object, guint propId, const GValue *value, GParamSpec *pspec)
{
    g_return_if_fail(object != nullptr);

    GstVideoShMemSink *vidShmemSink = GST_VIDEO_SHMEM_SINK_CAST(object);
    GstVideoShMemSinkPrivate *priv = vidShmemSink->priv;
    g_return_if_fail(priv != nullptr);

    switch (propId) {
        case PROP_CAPS:
            gst_video_shmem_sink_setcaps(vidShmemSink, gst_value_get_caps(value));
            break;
        case PROP_MAX_POOL_CAPACITY:
            priv->maxPoolCapacity = g_value_get_uint(value);
            GST_INFO_OBJECT(vidShmemSink, "set max pool capacity: %u", priv->maxPoolCapacity);
            break;
        case PROP_MEM_PREFIX:
            priv->memPrefix = g_value_get_uint(value);
            GST_INFO_OBJECT(vidShmemSink, "set memprefix to %d", priv->memPrefix);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
            break;
    }
}

static void gst_video_shmem_sink_get_property(GObject *object, guint propId, GValue *value, GParamSpec *pspec)
{
    g_return_if_fail(object != nullptr);

    GstVideoShMemSink *vidShmemSink = GST_VIDEO_SHMEM_SINK_CAST(object);
    GstVideoShMemSinkPrivate *priv = vidShmemSink->priv;
    g_return_if_fail(priv != nullptr);

    switch (propId) {
        case PROP_CAPS: {
            GstCaps *caps = gst_video_shmem_sink_getcaps(vidShmemSink);
            gst_value_set_caps(value, caps);
            if (caps != nullptr) {
                gst_caps_unref(caps);
            }
            break;
        }
        case PROP_MAX_POOL_CAPACITY:
            g_value_set_uint(value, priv->maxPoolCapacity);
            break;
        case PROP_MEM_PREFIX:
            g_value_set_uint(value, priv->memPrefix);
            GST_INFO_OBJECT(vidShmemSink, "get memprefix to %d", priv->memPrefix);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
            break;
    }
}

static void gst_video_shmem_sink_setcaps(GstVideoShMemSink *vidShMemSink, const GstCaps *caps)
{
    GstVideoShMemSinkPrivate *priv = vidShMemSink->priv;
    g_return_if_fail(priv != nullptr);

    GST_OBJECT_LOCK(vidShMemSink);
    GST_INFO_OBJECT(vidShMemSink, "setting caps to %" GST_PTR_FORMAT, caps);

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

    GST_OBJECT_UNLOCK(vidShMemSink);
}

static GstCaps *gst_video_shmem_sink_getcaps(GstVideoShMemSink *vidShMemSink)
{
    GstVideoShMemSinkPrivate *priv = vidShMemSink->priv;
    g_return_val_if_fail(priv != nullptr, nullptr);

    GST_OBJECT_LOCK(vidShMemSink);
    GstCaps *caps = priv->caps;
    if (caps != nullptr) {
        (void)gst_caps_ref(caps);
        GST_INFO_OBJECT(vidShMemSink, "getting caps of %" GST_PTR_FORMAT, caps);
    }
    GST_OBJECT_UNLOCK(vidShMemSink);

    return caps;
}

static gboolean gst_video_shmem_sink_unlock_start(GstBaseSink *bsink)
{
    GstVideoShMemSink *vidShMemSink = GST_VIDEO_SHMEM_SINK_CAST(bsink);
    g_return_val_if_fail(vidShMemSink != nullptr, FALSE);
    GstVideoShMemSinkPrivate *priv = vidShMemSink->priv;
    g_return_val_if_fail(priv != nullptr, FALSE);

    g_mutex_lock(&priv->mutex);
    GST_INFO_OBJECT(vidShMemSink, "unlock start");
    priv->unlock = TRUE;
    g_cond_signal(&priv->cond);
    g_mutex_unlock(&priv->mutex);

    return TRUE;
}

static gboolean gst_video_shmem_sink_unlock_stop(GstBaseSink *bsink)
{
    GstVideoShMemSink *vidShMemSink = GST_VIDEO_SHMEM_SINK_CAST(bsink);
    g_return_val_if_fail(vidShMemSink != nullptr, FALSE);
    GstVideoShMemSinkPrivate *priv = vidShMemSink->priv;
    g_return_val_if_fail(priv != nullptr, FALSE);

    g_mutex_lock(&priv->mutex);
    GST_INFO_OBJECT(vidShMemSink, "unlock stop");
    priv->unlock = FALSE;
    g_cond_signal(&priv->cond);
    g_mutex_unlock(&priv->mutex);

    return TRUE;
}

static void gst_video_shmem_sink_flush_unlocked(GstVideoShMemSink *vidShMemSink)
{
    GstVideoShMemSinkPrivate *priv = vidShMemSink->priv;
    g_return_if_fail(priv != nullptr);

    GST_INFO_OBJECT(vidShMemSink, "flush stop video ashmem sink");
    priv->isEos = FALSE;
    (void)gst_buffer_replace(&priv->prerollBuffer, nullptr);
    GstMiniObject *queueObj = reinterpret_cast<GstMiniObject *>(gst_queue_array_pop_head(priv->queue));
    while (queueObj != nullptr) {
        gst_mini_object_unref(queueObj);
        queueObj = reinterpret_cast<GstMiniObject *>(gst_queue_array_pop_head(priv->queue));
    }
    priv->numBuffers = 0;
    g_cond_signal(&priv->cond);
}

static gboolean gst_video_shmem_sink_start(GstBaseSink *bsink)
{
    GstVideoShMemSink *vidShMemSink = GST_VIDEO_SHMEM_SINK_CAST(bsink);
    g_return_val_if_fail(vidShMemSink != nullptr, FALSE);
    GstVideoShMemSinkPrivate *priv = vidShMemSink->priv;
    g_return_val_if_fail(priv != nullptr, FALSE);

    g_mutex_lock(&priv->mutex);
    GST_INFO_OBJECT(vidShMemSink, "starting");
    priv->waitStatus = NO_WAITING;
    priv->flushing = FALSE;
    priv->started = TRUE;
    gst_segment_init(&priv->prerollSegment, GST_FORMAT_TIME);
    gst_segment_init(&priv->lastSegment, GST_FORMAT_TIME);
    priv->sample = gst_sample_make_writable(priv->sample);
    gst_sample_set_buffer(priv->sample, nullptr);
    gst_sample_set_buffer_list(priv->sample, nullptr);
    gst_sample_set_caps(priv->sample, nullptr);
    gst_sample_set_segment(priv->sample, nullptr);
    g_mutex_unlock(&priv->mutex);

    return TRUE;
}

static gboolean gst_video_shmem_sink_stop(GstBaseSink *bsink)
{
    GstVideoShMemSink *vidShMemSink = GST_VIDEO_SHMEM_SINK_CAST(bsink);
    g_return_val_if_fail(vidShMemSink != nullptr, FALSE);
    GstVideoShMemSinkPrivate *priv = vidShMemSink->priv;
    g_return_val_if_fail(priv != nullptr, FALSE);

    g_mutex_lock(&priv->mutex);
    GST_INFO_OBJECT(vidShMemSink, "stopping");
    priv->flushing = TRUE;
    priv->started = FALSE;
    priv->waitStatus = NO_WAITING;
    gst_video_shmem_sink_flush_unlocked(vidShMemSink);
    (void)gst_buffer_replace(&priv->prerollBuffer, nullptr);
    (void)gst_caps_replace(&priv->prerollCaps, nullptr);
    (void)gst_caps_replace(&priv->lastCaps, nullptr);
    gst_segment_init(&priv->prerollSegment, GST_FORMAT_UNDEFINED);
    gst_segment_init(&priv->lastSegment, GST_FORMAT_UNDEFINED);
    g_mutex_unlock(&priv->mutex);

    return TRUE;
}

static gboolean WaitBufferDrained(GstVideoShMemSink *vidShMemSink)
{
    GstVideoShMemSinkPrivate *priv = vidShMemSink->priv;

    g_mutex_lock(&priv->mutex);
    while ((priv->numBuffers > 0) && (priv->flushing == FALSE)) {
        if (priv->unlock) {
            g_mutex_unlock(&priv->mutex);
            if (gst_base_sink_wait_preroll(GST_BASE_SINK_CAST(vidShMemSink)) != GST_FLOW_OK) {
                return FALSE;
            }
            g_mutex_lock(&priv->mutex);
            continue;
        }
        priv->waitStatus |= STREAM_WAITING;
        g_cond_wait(&priv->cond, &priv->mutex);
        priv->waitStatus &= ~STREAM_WAITING;
    }
    g_mutex_unlock(&priv->mutex);

    return TRUE;
}

static gboolean gst_video_shmem_sink_event(GstBaseSink *bsink, GstEvent *event)
{
    GstVideoShMemSink *vidShMemSink = GST_VIDEO_SHMEM_SINK_CAST(bsink);
    g_return_val_if_fail(vidShMemSink != nullptr, FALSE);
    GstVideoShMemSinkPrivate *priv = vidShMemSink->priv;
    g_return_val_if_fail(priv != nullptr, FALSE);

    switch (event->type) {
        case GST_EVENT_SEGMENT:
            g_mutex_lock(&priv->mutex);
            GST_INFO_OBJECT(vidShMemSink, "receiving SGEMENT");
            gst_queue_array_push_tail(priv->queue, gst_event_ref(event));
            if (priv->prerollBuffer == nullptr) {
                gst_event_copy_segment(event, &priv->prerollSegment);
            }
            g_mutex_unlock(&priv->mutex);
            break;
        case GST_EVENT_EOS: {
            g_mutex_lock(&priv->mutex);
            GST_INFO_OBJECT(vidShMemSink, "receiving EOS");
            priv->isEos = TRUE;
            g_cond_signal(&priv->cond);
            g_mutex_unlock(&priv->mutex);

            if (!WaitBufferDrained(vidShMemSink)) {
                gst_event_unref(event);
                return FALSE;
            }
            break;
        }
        case GST_EVENT_FLUSH_START:
            GST_INFO_OBJECT(vidShMemSink, "received FLUSH_START");
            break;
        case GST_EVENT_FLUSH_STOP:
            g_mutex_lock(&priv->mutex);
            GST_INFO_OBJECT(vidShMemSink, "received FLUSH_STOP");
            gst_video_shmem_sink_flush_unlocked(vidShMemSink);
            g_mutex_unlock(&priv->mutex);
            break;
        default:
            break;
    }
    return GST_BASE_SINK_CLASS(parent_class)->event(bsink, event);
}

static gboolean gst_video_shmem_sink_query(GstBaseSink *bsink, GstQuery *query)
{
    GstVideoShMemSink *vidShMemSink = GST_VIDEO_SHMEM_SINK_CAST(bsink);
    g_return_val_if_fail(vidShMemSink != nullptr, FALSE);
    GstVideoShMemSinkPrivate *priv = vidShMemSink->priv;
    g_return_val_if_fail(priv != nullptr, FALSE);

    gboolean ret;
    switch (GST_QUERY_TYPE(query)) {
        case GST_QUERY_DRAIN: {
            if (!WaitBufferDrained(vidShMemSink)) {
                return FALSE;
            }
            ret = GST_BASE_SINK_CLASS(parent_class)->query(bsink, query);
            break;
        }
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

static gboolean gst_video_shmem_sink_set_caps(GstBaseSink *sink, GstCaps *caps)
{
    GstVideoShMemSink *vidShMemSink = GST_VIDEO_SHMEM_SINK_CAST(sink);
    g_return_val_if_fail(vidShMemSink != nullptr, FALSE);
    GstVideoShMemSinkPrivate *priv = vidShMemSink->priv;
    g_return_val_if_fail(priv != nullptr, FALSE);

    g_mutex_lock(&priv->mutex);
    GST_INFO_OBJECT(vidShMemSink, "receiving CAPS");
    gst_queue_array_push_tail(priv->queue, gst_event_new_caps(caps));
    if (priv->prerollBuffer == nullptr) {
        (void)gst_caps_replace(&priv->prerollCaps, caps);
    }
    g_mutex_unlock(&priv->mutex);
    return TRUE;
}

static GstCaps *gst_video_shmem_sink_get_caps(GstBaseSink *sink, GstCaps *filter)
{
    GstVideoShMemSink *vidShMemSink = GST_VIDEO_SHMEM_SINK_CAST(sink);
    g_return_val_if_fail(vidShMemSink != nullptr, nullptr);
    GstVideoShMemSinkPrivate *priv = vidShMemSink->priv;
    g_return_val_if_fail(priv != nullptr, nullptr);

    GST_OBJECT_LOCK(vidShMemSink);
    GstCaps *caps = priv->caps;
    if (caps != nullptr) {
        if (filter != nullptr) {
            caps = gst_caps_intersect_full(filter, caps, GST_CAPS_INTERSECT_FIRST);
        } else {
            (void)gst_caps_ref(caps);
        }
        GST_INFO_OBJECT(vidShMemSink, "got caps %" GST_PTR_FORMAT, caps);
    }
    GST_OBJECT_UNLOCK(vidShMemSink);

    return caps;
}

static GstSample *gst_video_shmem_sink_pull_preroll(GstVideoShMemSink *vidShMemSink)
{
    g_return_val_if_fail(vidShMemSink != nullptr, nullptr);
    GstVideoShMemSinkPrivate *priv = vidShMemSink->priv;
    g_return_val_if_fail(priv != nullptr, nullptr);

    GST_INFO_OBJECT(vidShMemSink, "pull preroll enter");
    g_mutex_lock(&priv->mutex);

    while (TRUE) {
        GST_INFO_OBJECT(vidShMemSink, "trying to grab a buffer");
        if (!priv->started) {
            GST_INFO_OBJECT(vidShMemSink, "we are stopped, return nullptr");
            g_mutex_unlock(&priv->mutex);
            return nullptr;
        }
        if (priv->prerollBuffer != nullptr) {
            break;
        }
        if (priv->isEos) {
            GST_INFO_OBJECT(vidShMemSink, "we are EOS, return nullptr");
            g_mutex_unlock(&priv->mutex);
            return nullptr;
        }
        GST_INFO_OBJECT(vidShMemSink, "waiting for the preroll buffer");
        priv->waitStatus |= USER_WAITING;
        g_cond_wait(&priv->cond, &priv->mutex);
        priv->waitStatus &= ~USER_WAITING;
    }

    GstSample *result = gst_sample_new(priv->prerollBuffer, priv->prerollCaps, &priv->prerollSegment, nullptr);
    (void)gst_buffer_replace(&priv->prerollBuffer, nullptr);
    GST_INFO_OBJECT(vidShMemSink, "we have the preroll sample 0x%06" PRIXPTR "", FAKE_POINTER(result));

    g_mutex_unlock(&priv->mutex);
    return result;
}

static GstBuffer *DequeueBuffer(GstVideoShMemSink *vidShMemSink)
{
    GstVideoShMemSinkPrivate *priv = vidShMemSink->priv;
    g_return_val_if_fail(priv != nullptr, nullptr);

    GstMiniObject *obj = nullptr;
    GstBuffer *buffer = nullptr;
    do {
        obj = GST_MINI_OBJECT_CAST(gst_queue_array_pop_head(priv->queue));
        if (GST_IS_BUFFER(obj)) {
            GST_INFO_OBJECT(vidShMemSink, "dequeued bffer 0x%06" PRIXPTR "", FAKE_POINTER(obj));
            priv->numBuffers--;
            buffer = GST_BUFFER_CAST(obj);
            break;
        }

        if (GST_IS_EVENT(obj)) {
            GstEvent *event = GST_EVENT_CAST(obj);
            switch (GST_EVENT_TYPE(obj)) {
                case GST_EVENT_CAPS: {
                    GstCaps *caps = nullptr;
                    gst_event_parse_caps(event, &caps);
                    GST_INFO_OBJECT(vidShMemSink, "activating caps %" GST_PTR_FORMAT, caps);
                    (void)gst_caps_replace(&priv->lastCaps, caps);
                    priv->sample = gst_sample_make_writable(priv->sample);
                    gst_sample_set_caps(priv->sample, priv->lastCaps);
                    break;
                }
                case GST_EVENT_SEGMENT: {
                    gst_event_copy_segment(event, &priv->lastSegment);
                    priv->sample = gst_sample_make_writable(priv->sample);
                    gst_sample_set_segment(priv->sample, &priv->lastSegment);
                    GST_INFO_OBJECT(vidShMemSink, "activated segment %" GST_SEGMENT_FORMAT, &priv->lastSegment);
                    break;
                }
                default:
                    break;
            }
            gst_mini_object_unref(obj);
        }
    } while (gst_queue_array_is_empty(priv->queue) == FALSE);

    return buffer;
}

static GstSample *gst_video_shmem_sink_pull_sample(GstVideoShMemSink *vidShMemSink)
{
    g_return_val_if_fail(vidShMemSink != nullptr, nullptr);
    GstVideoShMemSinkPrivate *priv = vidShMemSink->priv;
    g_return_val_if_fail(priv != nullptr, nullptr);

    GST_INFO_OBJECT(vidShMemSink, "pull sample enter");
    g_mutex_lock(&priv->mutex);
    (void)gst_buffer_replace(&priv->prerollBuffer, nullptr);

    while (TRUE) {
        GST_INFO_OBJECT(vidShMemSink, "trying to grab a buffer");
        if (!priv->started) {
            GST_INFO_OBJECT(vidShMemSink, "we are stopped, return nullptr");
            g_mutex_unlock(&priv->mutex);
            return nullptr;
        }
        if (priv->numBuffers > 0) {
            break;
        }
        if (priv->isEos) {
            GST_INFO_OBJECT(vidShMemSink, "we are EOS, return nullptr");
            g_mutex_unlock(&priv->mutex);
            return nullptr;
        }
        GST_INFO_OBJECT(vidShMemSink, "waiting for a buffer");
        priv->waitStatus |= USER_WAITING;
        g_cond_wait(&priv->cond, &priv->mutex);
        priv->waitStatus &= ~USER_WAITING;
    }

    GstBuffer *buffer = DequeueBuffer(vidShMemSink);
    GST_INFO_OBJECT(vidShMemSink, "we have a buffer 0x%06" PRIXPTR "", FAKE_POINTER(buffer));
    priv->sample = gst_sample_make_writable(priv->sample);
    gst_sample_set_buffer_list(priv->sample, nullptr);
    gst_sample_set_buffer(priv->sample, buffer);
    GstSample *sample = gst_sample_ref(priv->sample);
    gst_buffer_unref(buffer);

    g_mutex_unlock(&priv->mutex);
    return sample;
}

static GstFlowReturn gst_video_shmem_sink_preroll(GstBaseSink *bsink, GstBuffer *buffer)
{
    GstVideoShMemSink *vidShMemSink = GST_VIDEO_SHMEM_SINK_CAST(bsink);
    g_return_val_if_fail(vidShMemSink != nullptr, GST_FLOW_ERROR);
    GstVideoShMemSinkPrivate *priv = vidShMemSink->priv;
    g_return_val_if_fail(priv != nullptr, GST_FLOW_ERROR);

    g_mutex_lock(&priv->mutex);

    if (priv->flushing) {
        GST_INFO_OBJECT(vidShMemSink, "we are flushing");
        g_mutex_unlock(&priv->mutex);
        return GST_FLOW_FLUSHING;
    }

    GST_INFO_OBJECT(vidShMemSink, "setting preroll buffer 0x%06" PRIXPTR "", FAKE_POINTER(buffer));
    (void)gst_buffer_replace(&priv->prerollBuffer, buffer);

    if (priv->waitStatus & USER_WAITING) {
        g_cond_signal(&priv->cond);
    }

    g_mutex_unlock(&priv->mutex);

    GstFlowReturn ret = GST_FLOW_OK;
    g_signal_emit(vidShMemSink, gst_video_shmem_sink_signals[SIGNAL_NEW_PREROLL], 0, &ret);

    return ret;
}

static GstFlowReturn gst_video_shmem_sink_render(GstBaseSink *bsink, GstBuffer *buffer)
{
    GstVideoShMemSink *vidShMemSink = GST_VIDEO_SHMEM_SINK_CAST(bsink);
    g_return_val_if_fail(vidShMemSink != nullptr, GST_FLOW_ERROR);
    GstVideoShMemSinkPrivate *priv = vidShMemSink->priv;
    g_return_val_if_fail(priv != nullptr, GST_FLOW_ERROR);

    g_mutex_lock(&priv->mutex);

    if (priv->flushing) {
        GST_INFO_OBJECT(vidShMemSink, "we are flushing");
        g_mutex_unlock(&priv->mutex);
        return GST_FLOW_FLUSHING;
    }

    if (G_UNLIKELY(priv->lastCaps == nullptr && gst_pad_has_current_caps(GST_BASE_SINK_PAD(bsink)))) {
        priv->lastCaps = gst_pad_get_current_caps(GST_BASE_SINK_PAD(bsink));
        gst_sample_set_caps(priv->sample, priv->lastCaps);
        GST_INFO_OBJECT(vidShMemSink, "activating pad caps %" GST_PTR_FORMAT, priv->lastCaps);
    }

    GST_INFO_OBJECT(vidShMemSink, "pushing render buffer 0x%06" PRIXPTR " on queue (%d)",
                     FAKE_POINTER(buffer), priv->numBuffers);

    gst_queue_array_push_tail(priv->queue, gst_mini_object_ref(GST_MINI_OBJECT_CAST(buffer)));
    priv->numBuffers++;

    if (priv->waitStatus & USER_WAITING) {
        g_cond_signal(&priv->cond);
    }

    g_mutex_unlock(&priv->mutex);

    GstFlowReturn ret = GST_FLOW_OK;
    g_signal_emit(vidShMemSink, gst_video_shmem_sink_signals[SIGNAL_NEW_SAMPLE], 0, &ret);

    return ret;
}

static gboolean gst_video_shmem_sink_propose_allocation(GstBaseSink *bsink, GstQuery *query)
{
    GstVideoShMemSink *vidShMemSink = GST_VIDEO_SHMEM_SINK_CAST(bsink);
    g_return_val_if_fail(vidShMemSink != nullptr, FALSE);
    g_return_val_if_fail(vidShMemSink->priv != nullptr, FALSE);

    GstCaps *caps = nullptr;
    gboolean needPool = FALSE;
    gst_query_parse_allocation(query, &caps, &needPool);
    GST_DEBUG_OBJECT(bsink, "process allocation query, caps: %" GST_PTR_FORMAT "", caps);

    if (!needPool) {
        GST_ERROR_OBJECT(bsink, "no need buffer pool, unexpected!");
        return FALSE;
    }

    GstBufferPool *pool = gst_video_shmem_pool_new();
    g_return_val_if_fail(pool != nullptr, FALSE);

    GstVideoInfo info;
    GST_DEBUG("begin gst_video_info_from_caps");
    gboolean ret = gst_video_info_from_caps(&info, caps);
    if (!ret) {
        gst_object_unref(pool);
        GST_ERROR_OBJECT(vidShMemSink, "get video info from caps failed");
        return ret;
    }

    gst_query_add_allocation_pool(query, pool, info.size, 0, vidShMemSink->priv->maxPoolCapacity);

    GstAllocator *alloc = gst_shmem_allocator_old_new();
    if (alloc == nullptr) {
        gst_object_unref(pool);
        GST_ERROR_OBJECT(vidShMemSink, "create shmem allocator failed");
        return FALSE;
    }

    GstAllocationParams allocParams {};
    allocParams.prefix = vidShMemSink->priv->memPrefix;

    gst_query_add_allocation_param(query, alloc, &allocParams);
    gst_query_add_allocation_meta(query, GST_VIDEO_META_API_TYPE, nullptr);

    GstStructure *config = gst_buffer_pool_get_config(pool);
    gst_buffer_pool_config_set_params(config, caps, info.size, 0, vidShMemSink->priv->maxPoolCapacity);
    gst_buffer_pool_config_add_option(config, GST_BUFFER_POOL_OPTION_VIDEO_META);
    gst_buffer_pool_config_set_allocator(config, alloc, &allocParams);
    ret = gst_buffer_pool_set_config(pool, config);
    gst_object_unref(pool);
    gst_object_unref(alloc);

    g_return_val_if_fail(ret, FALSE);
    return TRUE;
}

static gboolean plugin_init(GstPlugin *plugin)
{
    g_return_val_if_fail(plugin != nullptr, false);
    return gst_element_register(plugin, "vidshmemsink", GST_RANK_MARGINAL, GST_TYPE_VIDEO_SHMEM_SINK);
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    _video_shmem_sink,
    "GStreamer Video ShMem Sink",
    plugin_init,
    PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
