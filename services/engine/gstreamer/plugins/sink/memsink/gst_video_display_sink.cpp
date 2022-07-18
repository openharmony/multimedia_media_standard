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

#include "gst_video_display_sink.h"

enum {
    PROP_0,
    PROP_AUDIO_SINK,
    PROP_ENABLE_KPI_AVSYNC_LOG,
};

struct _GstVideoDisplaySinkPrivate {
    GstElement *audio_sink;
    gboolean enable_kpi_avsync_log;
    GMutex mutex;
};

#define gst_video_display_sink_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstVideoDisplaySink, gst_video_display_sink,
                        GST_TYPE_SURFACE_MEM_SINK, G_ADD_PRIVATE(GstVideoDisplaySink));

GST_DEBUG_CATEGORY_STATIC(gst_video_display_sink_debug_category);
#define GST_CAT_DEFAULT gst_video_display_sink_debug_category

static void gst_video_display_sink_dispose(GObject *obj);
static void gst_video_display_sink_finalize(GObject *obj);
static void gst_video_display_sink_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static GstFlowReturn gst_video_display_sink_do_app_render(GstSurfaceMemSink *surface_sink,
    GstBuffer *buffer, bool is_preroll);

static void gst_video_display_sink_class_init(GstVideoDisplaySinkClass *klass)
{
    g_return_if_fail(klass != nullptr);

    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstSurfaceMemSinkClass *surface_sink_class = GST_SURFACE_MEM_SINK_CLASS(klass);
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

    gst_element_class_set_static_metadata(element_class,
        "VideoDisplaySink", "Sink/Video", " Video Display sink", "OpenHarmony");

    gobject_class->dispose = gst_video_display_sink_dispose;
    gobject_class->finalize = gst_video_display_sink_finalize;
    gobject_class->set_property = gst_video_display_sink_set_property;

    g_object_class_install_property(gobject_class, PROP_AUDIO_SINK,
        g_param_spec_pointer("audio-sink", "audio sink", "audio sink",
            (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_ENABLE_KPI_AVSYNC_LOG,
        g_param_spec_boolean("enable-kpi-avsync-log", "Enable KPI AV sync log", "Enable KPI AV sync log", FALSE,
            (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    surface_sink_class->do_app_render = gst_video_display_sink_do_app_render;
    GST_DEBUG_CATEGORY_INIT(gst_video_display_sink_debug_category, "videodisplaysink", 0, "videodisplaysink class");
}

static void gst_video_display_sink_init(GstVideoDisplaySink *sink)
{
    g_return_if_fail(sink != nullptr);

    auto priv = reinterpret_cast<GstVideoDisplaySinkPrivate *>(gst_video_display_sink_get_instance_private(sink));
    g_return_if_fail(priv != nullptr);

    sink->priv = priv;
    priv->audio_sink = nullptr;
    priv->enable_kpi_avsync_log = FALSE;
    g_mutex_init(&priv->mutex);
}

static void gst_video_display_sink_dispose(GObject *obj)
{
    g_return_if_fail(obj != nullptr);
    GstVideoDisplaySink *video_display_sink = GST_VIDEO_DISPLAY_SINK_CAST(obj);
    GstVideoDisplaySinkPrivate *priv = video_display_sink->priv;

    g_mutex_lock(&priv->mutex);
    if (priv->audio_sink != nullptr) {
        gst_object_unref(priv->audio_sink);
        priv->audio_sink = nullptr;
    }
    g_mutex_unlock(&priv->mutex);
    G_OBJECT_CLASS(parent_class)->dispose(obj);
}

static void gst_video_display_sink_finalize(GObject *obj)
{
    g_return_if_fail(obj != nullptr);
    GstVideoDisplaySink *video_display_sink = GST_VIDEO_DISPLAY_SINK_CAST(obj);
    GstVideoDisplaySinkPrivate *priv = video_display_sink->priv;
    g_return_if_fail(priv != nullptr);

    g_mutex_clear(&priv->mutex);
    G_OBJECT_CLASS(parent_class)->finalize(obj);
}

static void gst_video_display_sink_set_audio_sink(GstVideoDisplaySink *video_display_sink, gpointer audio_sink)
{
    g_return_if_fail(audio_sink != nullptr);

    GstVideoDisplaySinkPrivate *priv = video_display_sink->priv;
    g_mutex_lock(&priv->mutex);
    if (priv->audio_sink != nullptr) {
        GST_INFO_OBJECT(video_display_sink, "has audio sink: %s, unref it", GST_ELEMENT_NAME(priv->audio_sink));
        gst_object_unref(priv->audio_sink);
    }
    priv->audio_sink = GST_ELEMENT_CAST(gst_object_ref(audio_sink));
    GST_INFO_OBJECT(video_display_sink, "get audio sink: %s", GST_ELEMENT_NAME(priv->audio_sink));
    g_mutex_unlock(&priv->mutex);
}

static void gst_video_display_sink_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    g_return_if_fail(object != nullptr);
    g_return_if_fail(value != nullptr);
    g_return_if_fail(pspec != nullptr);

    GstVideoDisplaySink *video_display_sink = GST_VIDEO_DISPLAY_SINK_CAST(object);
    GstVideoDisplaySinkPrivate *priv = video_display_sink->priv;
    g_return_if_fail(priv != nullptr);

    switch (prop_id) {
        case PROP_AUDIO_SINK:
            gst_video_display_sink_set_audio_sink(video_display_sink, g_value_get_pointer(value));
            break;
        case PROP_ENABLE_KPI_AVSYNC_LOG:
            g_mutex_lock(&priv->mutex);
            priv->enable_kpi_avsync_log = g_value_get_boolean(value);
            g_mutex_unlock(&priv->mutex);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void kpi_log_avsync_diff(GstVideoDisplaySink *video_display_sink, guint64 last_render_pts)
{
    GstVideoDisplaySinkPrivate *priv = video_display_sink->priv;
    guint64 audio_last_render_pts = 0;

    // get av sync diff time
    g_mutex_lock(&priv->mutex);
    if (priv->enable_kpi_avsync_log && priv->audio_sink != nullptr) {
    g_object_get(priv->audio_sink, "last-render-pts", &audio_last_render_pts, nullptr);
    GST_WARNING_OBJECT(video_display_sink, "KPI-TRACE: audio_last_render_pts=%" G_GUINT64_FORMAT
        ", video_last_render_pts=%" G_GUINT64_FORMAT ", diff=%" G_GINT64_FORMAT " ms",
        audio_last_render_pts, last_render_pts,
        ((gint64)audio_last_render_pts - (gint64)last_render_pts) / GST_MSECOND);
    }
    g_mutex_unlock(&priv->mutex);
}

static GstFlowReturn gst_video_display_sink_do_app_render(GstSurfaceMemSink *surface_sink,
    GstBuffer *buffer, bool is_preroll)
{
    (void)is_preroll;
    g_return_val_if_fail(surface_sink != nullptr && buffer != nullptr, GST_FLOW_ERROR);
    GstVideoDisplaySink *video_display_sink = GST_VIDEO_DISPLAY_SINK_CAST(surface_sink);

    kpi_log_avsync_diff(video_display_sink, GST_BUFFER_PTS(buffer));
    return GST_FLOW_OK;
}
