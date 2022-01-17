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
#include "gst_audio_server_sink.h"
#include <cinttypes>
#include <gst/gst.h>
#include "gst/audio/audio.h"
#include "media_errors.h"
#include "audio_sink_factory.h"

static GstStaticPadTemplate g_sinktemplate = GST_STATIC_PAD_TEMPLATE("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS("audio/x-raw, "
        "format = (string) S16LE, "
        "layout = (string) interleaved, "
        "rate = (int) [ 1, MAX ], "
        "channels = (int) [ 1, MAX ]"));

using namespace OHOS::Media;
namespace {
    constexpr float DEFAULT_VOLUME = 1.0f;
    constexpr uint32_t DEFAULT_BITS_PER_SAMPLE = 16;
}

enum {
    PROP_0,
    PROP_BITS_PER_SAMPLE,
    PROP_CHANNELS,
    PROP_SAMPLE_RATE,
    PROP_VOLUME,
    PROP_MAX_VOLUME,
    PROP_MIN_VOLUME,
    PROP_AUDIO_RENDERER_DESC
};

#define gst_audio_server_sink_parent_class parent_class
G_DEFINE_TYPE(GstAudioServerSink, gst_audio_server_sink, GST_TYPE_BASE_SINK);

static void gst_audio_server_sink_finalize(GObject *object);
static void gst_audio_server_sink_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_audio_server_sink_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static GstStateChangeReturn gst_audio_server_sink_change_state(GstElement *element, GstStateChange transition);
static GstCaps *gst_audio_server_sink_get_caps(GstBaseSink *basesink, GstCaps *caps);
static gboolean gst_audio_server_sink_set_caps(GstBaseSink *basesink, GstCaps *caps);
static gboolean gst_audio_server_sink_event(GstBaseSink *basesink, GstEvent *event);
static gboolean gst_audio_server_sink_start(GstBaseSink *basesink);
static gboolean gst_audio_server_sink_stop(GstBaseSink *basesink);
static GstFlowReturn gst_audio_server_sink_render(GstBaseSink *basesink, GstBuffer *buffer);
static void gst_audio_server_sink_clear_cache_buffer(GstAudioServerSink *sink);

static void gst_audio_server_sink_class_init(GstAudioServerSinkClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstElementClass *gstelement_class = GST_ELEMENT_CLASS(klass);
    GstBaseSinkClass *gstbasesink_class = GST_BASE_SINK_CLASS(klass);
    g_return_if_fail((gobject_class != nullptr) && (gstelement_class != nullptr) && (gstbasesink_class != nullptr));

    gobject_class->finalize = gst_audio_server_sink_finalize;
    gobject_class->set_property = gst_audio_server_sink_set_property;
    gobject_class->get_property = gst_audio_server_sink_get_property;

    g_object_class_install_property(gobject_class, PROP_BITS_PER_SAMPLE,
        g_param_spec_uint("bps", "Bits Per Sample",
            "Audio Format", 0, G_MAXINT32, 0,
            (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_CHANNELS,
        g_param_spec_uint("channels", "Channels",
            "Channels", 0, G_MAXINT32, 0,
            (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_SAMPLE_RATE,
        g_param_spec_uint("sample-rate", "Sample Rate",
            "Sample Rate", 0, G_MAXINT32, 0,
            (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_VOLUME,
        g_param_spec_float("volume", "Volume",
            "Volume", 0, G_MAXFLOAT, 0,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_MAX_VOLUME,
        g_param_spec_float("max-volume", "Maximum Volume",
            "Maximum Volume", 0, G_MAXFLOAT, G_MAXFLOAT,
            (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_MIN_VOLUME,
        g_param_spec_float("min-volume", "Minimum Volume",
            "Minimum Volume", 0, G_MAXFLOAT, 0,
            (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_AUDIO_RENDERER_DESC,
        g_param_spec_int("audio-renderer-desc", "Audio Renderer Desc",
            "Audio Renderer Desc", 0, G_MAXINT32, 0,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    gst_element_class_set_static_metadata(gstelement_class,
        "Audio server sink", "Sink/Audio",
        "Push pcm data to Audio server", "OpenHarmony");

    gst_element_class_add_static_pad_template(gstelement_class, &g_sinktemplate);

    gstelement_class->change_state = gst_audio_server_sink_change_state;

    gstbasesink_class->get_caps = gst_audio_server_sink_get_caps;
    gstbasesink_class->set_caps = gst_audio_server_sink_set_caps;
    gstbasesink_class->event = gst_audio_server_sink_event;
    gstbasesink_class->start = gst_audio_server_sink_start;
    gstbasesink_class->stop = gst_audio_server_sink_stop;
    gstbasesink_class->render = gst_audio_server_sink_render;
}

static void gst_audio_server_sink_init(GstAudioServerSink *sink)
{
    g_return_if_fail(sink != nullptr);
    sink->audio_sink = nullptr;
    sink->bits_per_sample = DEFAULT_BITS_PER_SAMPLE;
    sink->channels = 0;
    sink->sample_rate = 0;
    sink->volume = DEFAULT_VOLUME;
    sink->max_volume = G_MAXFLOAT;
    sink->min_volume = 0;
    sink->min_buffer_size = 0;
    sink->min_frame_count = 0;
    sink->cache_buffer = nullptr;
    sink->pause_cache_buffer = nullptr;
    sink->cache_size = 0;
    sink->enable_cache = FALSE;
    sink->frame_after_segment = FALSE;
    sink->is_start = FALSE;
    g_mutex_init(&sink->render_lock);
}

static void gst_audio_server_sink_finalize(GObject *object)
{
    g_return_if_fail(object != nullptr);
    GstAudioServerSink *sink = GST_AUDIO_SERVER_SINK(object);
    g_return_if_fail(sink != nullptr);
    GST_INFO_OBJECT(sink, "gst_audio_server_sink_finalize in");

    g_mutex_clear(&sink->render_lock);
    if (sink->audio_sink != nullptr) {
        (void)sink->audio_sink->Release();
        sink->audio_sink = nullptr;
    }
    gst_audio_server_sink_clear_cache_buffer(sink);
}

static gboolean gst_audio_server_sink_set_volume(GstAudioServerSink *sink, gfloat volume)
{
    gboolean ret = FALSE;
    g_return_val_if_fail(sink != nullptr, FALSE);
    g_return_val_if_fail(sink->audio_sink != nullptr, FALSE);
    g_return_val_if_fail(volume <= sink->max_volume, FALSE);
    g_return_val_if_fail(volume >= sink->min_volume, FALSE);

    if (sink->audio_sink->SetVolume(volume) == MSERR_OK) {
        sink->volume = volume;
        ret = TRUE;
    }

    GST_INFO_OBJECT(sink, "set volume(%f) finish, ret=%d", volume, ret);
    return ret;
}

static gboolean gst_audio_server_sink_set_audio_renderer_desc(GstAudioServerSink *sink, gint param)
{
    gboolean ret = FALSE;
    g_return_val_if_fail(sink != nullptr, FALSE);
    g_return_val_if_fail(sink->audio_sink != nullptr, FALSE);

    if (sink->audio_sink->SetParameter(param) == MSERR_OK) {
        ret = TRUE;
    }

    GST_INFO_OBJECT(sink, "set renderer descriptor finish, ret=%d", ret);
    return ret;
}

static void gst_audio_server_sink_set_property(GObject *object, guint prop_id,
    const GValue *value, GParamSpec *pspec)
{
    g_return_if_fail(object != nullptr);
    g_return_if_fail(value != nullptr);
    (void)pspec;
    GstAudioServerSink *sink = GST_AUDIO_SERVER_SINK(object);
    g_return_if_fail(sink != nullptr);
    switch (prop_id) {
        case PROP_VOLUME:
            if (gst_audio_server_sink_set_volume(sink, g_value_get_float(value))) {
                GST_INFO_OBJECT(sink, "set volume success!");
                g_object_notify(G_OBJECT(sink), "volume");
            }
            break;
        case PROP_AUDIO_RENDERER_DESC:
            if (gst_audio_server_sink_set_audio_renderer_desc(sink, g_value_get_int(value))) {
                GST_INFO_OBJECT(sink, "set renderer descriptor success!");
                g_object_notify(G_OBJECT(sink), "audio-renderer-desc");
            }
            break;
        default:
            break;
    }
}

static void gst_audio_server_sink_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    g_return_if_fail(object != nullptr);
    g_return_if_fail(value != nullptr);
    (void)pspec;
    GstAudioServerSink *sink = GST_AUDIO_SERVER_SINK(object);
    g_return_if_fail(sink != nullptr);
    switch (prop_id) {
        case PROP_BITS_PER_SAMPLE:
            g_value_set_uint(value, sink->bits_per_sample);
            break;
        case PROP_CHANNELS:
            g_value_set_uint(value, sink->channels);
            break;
        case PROP_SAMPLE_RATE:
            g_value_set_uint(value, sink->sample_rate);
            break;
        case PROP_VOLUME:
            if (sink->audio_sink != nullptr) {
                (void)sink->audio_sink->GetVolume(sink->volume);
            }
            g_value_set_float(value, sink->volume);
            break;
        case PROP_MAX_VOLUME:
            g_value_set_float(value, sink->max_volume);
            break;
        case PROP_MIN_VOLUME:
            g_value_set_float(value, sink->min_volume);
            break;
        default:
            break;
    }
}

static GstCaps *gst_audio_server_sink_get_caps(GstBaseSink *basesink, GstCaps *caps)
{
    (void)caps;
    GstAudioServerSink *sink = GST_AUDIO_SERVER_SINK(basesink);
    g_return_val_if_fail(sink != nullptr, FALSE);
    g_return_val_if_fail(sink->audio_sink != nullptr, FALSE);
    return sink->audio_sink->GetCaps();
}

static gboolean gst_audio_server_sink_set_caps(GstBaseSink *basesink, GstCaps *caps)
{
    g_return_val_if_fail(basesink != nullptr, FALSE);
    g_return_val_if_fail(caps != nullptr, FALSE);
    GstAudioServerSink *sink = GST_AUDIO_SERVER_SINK(basesink);
    g_return_val_if_fail(sink != nullptr, FALSE);
    g_return_val_if_fail(sink->audio_sink != nullptr, FALSE);

    gchar *caps_str = gst_caps_to_string(caps);
    GST_INFO_OBJECT(basesink, "caps=%s", caps_str);
    GstStructure *structure = gst_caps_get_structure(caps, 0);
    g_return_val_if_fail(structure != nullptr, FALSE);
    gint channels = 0;
    gint rate = 0;
    if (!gst_structure_get_int(structure, "rate", &rate) || !gst_structure_get_int(structure, "channels", &channels)) {
        GST_ERROR_OBJECT(basesink, "Incomplete caps");
        return FALSE;
    }
    g_return_val_if_fail(channels > 0 && rate > 0, FALSE);
    sink->sample_rate = static_cast<uint32_t>(rate);
    sink->channels = static_cast<uint32_t>(channels);
    g_return_val_if_fail(sink->audio_sink->SetParameters(sink->bits_per_sample, sink->channels,
        sink->sample_rate) == MSERR_OK, FALSE);
    g_return_val_if_fail(sink->audio_sink->SetVolume(sink->volume) == MSERR_OK, FALSE);
    g_return_val_if_fail(sink->audio_sink->Start() == MSERR_OK, FALSE);
    g_return_val_if_fail(sink->audio_sink->GetParameters(sink->bits_per_sample,
        sink->channels, sink->sample_rate) == MSERR_OK, FALSE);
    g_return_val_if_fail(sink->audio_sink->GetMinimumBufferSize(sink->min_buffer_size) == MSERR_OK, FALSE);
    g_return_val_if_fail(sink->audio_sink->GetMinimumFrameCount(sink->min_frame_count) == MSERR_OK, FALSE);

    return TRUE;
}

static gboolean gst_audio_server_sink_event(GstBaseSink *basesink, GstEvent *event)
{
    g_return_val_if_fail(basesink != nullptr, FALSE);
    g_return_val_if_fail(event != nullptr, FALSE);
    GstAudioServerSink *sink = GST_AUDIO_SERVER_SINK(basesink);
    g_return_val_if_fail(sink != nullptr, FALSE);
    GstEventType type = GST_EVENT_TYPE(event);
    switch (type) {
        case GST_EVENT_EOS:
            if (sink->audio_sink == nullptr) {
                break;
            }
            if (sink->audio_sink->Drain() != MSERR_OK) {
                GST_ERROR_OBJECT(basesink, "fail to call Drain when handling EOS event");
            }
            break;
        case GST_EVENT_SEGMENT:
            g_mutex_lock(&sink->render_lock);
            sink->frame_after_segment = TRUE;
            g_mutex_unlock(&sink->render_lock);
            break;
        case GST_EVENT_FLUSH_START:
            gst_audio_server_sink_clear_cache_buffer(sink);
            if (sink->audio_sink == nullptr) {
                break;
            }
            if (sink->audio_sink->Flush() != MSERR_OK) {
                GST_ERROR_OBJECT(basesink, "fail to call Flush when handling SEEK event");
            }
            GST_DEBUG_OBJECT(basesink, "received FLUSH_START");
            break;
        case GST_EVENT_FLUSH_STOP:
            GST_DEBUG_OBJECT(basesink, "received FLUSH_STOP");
            break;
        default:
            break;
    }
    return GST_BASE_SINK_CLASS(parent_class)->event(basesink, event);
}

static gboolean gst_audio_server_sink_start(GstBaseSink *basesink)
{
    g_return_val_if_fail(basesink != nullptr, FALSE);
    GstAudioServerSink *sink = GST_AUDIO_SERVER_SINK(basesink);
    g_return_val_if_fail(sink != nullptr, FALSE);
    sink->audio_sink = OHOS::Media::AudioSinkFactory::CreateAudioSink();
    g_return_val_if_fail(sink->audio_sink != nullptr, FALSE);
    g_return_val_if_fail(sink->audio_sink->Prepare() == MSERR_OK, FALSE);
    g_return_val_if_fail(sink->audio_sink->GetMaxVolume(sink->max_volume) == MSERR_OK, FALSE);
    g_return_val_if_fail(sink->audio_sink->GetMinVolume(sink->min_volume) == MSERR_OK, FALSE);

    return TRUE;
}

static void gst_audio_server_sink_clear_cache_buffer(GstAudioServerSink *sink)
{
    std::unique_lock<std::mutex> lock(sink->mutex_);
    if (sink->pause_cache_buffer != nullptr) {
        gst_buffer_unref(sink->pause_cache_buffer);
        sink->pause_cache_buffer = nullptr;
    }
}

static gboolean gst_audio_server_sink_stop(GstBaseSink *basesink)
{
    g_return_val_if_fail(basesink != nullptr, FALSE);
    GstAudioServerSink *sink = GST_AUDIO_SERVER_SINK(basesink);
    g_return_val_if_fail(sink != nullptr, FALSE);
    g_return_val_if_fail(sink->audio_sink != nullptr, FALSE);
    g_return_val_if_fail(sink->audio_sink->Stop() == MSERR_OK, FALSE);
    g_return_val_if_fail(sink->audio_sink->Release() == MSERR_OK, FALSE);
    sink->audio_sink = nullptr;

    return TRUE;
}

static GstFlowReturn gst_audio_server_sink_cache_render(GstAudioServerSink *sink, GstBuffer *buffer)
{
    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_READ) != TRUE) {
        return GST_FLOW_ERROR;
    }
    gsize size = map.size;

    if (sink->cache_size == 0 && size >= sink->min_buffer_size) {
        if (sink->audio_sink->Write(map.data, size) != MSERR_OK) {
            gst_buffer_unmap(buffer, &map);
            return GST_FLOW_ERROR;
        }
        gst_buffer_unmap(buffer, &map);
        return GST_FLOW_OK;
    }
    gst_buffer_unmap(buffer, &map);

    if (sink->cache_size == 0 && size < sink->min_buffer_size) {
        sink->cache_size += static_cast<guint>(size);
        sink->cache_buffer = gst_buffer_copy(buffer);
        if (sink->cache_buffer == nullptr) {
            return GST_FLOW_ERROR;
        }
        return GST_FLOW_OK;
    }

    sink->cache_size += static_cast<guint>(size);
    GstBuffer *buf = gst_buffer_copy(sink->cache_buffer);
    if (buf == nullptr) {
        gst_buffer_unref(sink->cache_buffer);
        return GST_FLOW_ERROR;
    }
    (void)gst_buffer_ref(buffer);
    buf = gst_buffer_append(buf, buffer);
    gst_buffer_unref(sink->cache_buffer);
    sink->cache_buffer = buf;

    if (sink->cache_size >= sink->min_buffer_size) {
        if (gst_buffer_map(sink->cache_buffer, &map, GST_MAP_READ) != TRUE) {
            gst_buffer_unref(sink->cache_buffer);
            return GST_FLOW_ERROR;
        }
        int32_t ret = sink->audio_sink->Write(map.data, sink->cache_size);
        gst_buffer_unmap(sink->cache_buffer, &map);
        gst_buffer_unref(sink->cache_buffer);
        sink->cache_size = 0;
        sink->cache_buffer = nullptr;
        if (ret != MSERR_OK) {
            return GST_FLOW_ERROR;
        }
        return GST_FLOW_OK;
    }
    return GST_FLOW_OK;
}

static GstStateChangeReturn gst_audio_server_sink_change_state(GstElement *element, GstStateChange transition)
{
    g_return_val_if_fail(element != nullptr, GST_STATE_CHANGE_FAILURE);
    GstAudioServerSink *sink = GST_AUDIO_SERVER_SINK(element);
    g_return_val_if_fail(sink != nullptr, GST_STATE_CHANGE_FAILURE);
    GstBaseSink *basesink = GST_BASE_SINK(element);
    g_return_val_if_fail(basesink != nullptr, GST_STATE_CHANGE_FAILURE);

    switch (transition) {
        case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
            if (sink->is_start == FALSE) {
                sink->is_start = TRUE;
            } else {
                g_return_val_if_fail(sink->audio_sink != nullptr, GST_STATE_CHANGE_FAILURE);
                g_return_val_if_fail(sink->audio_sink->Start() == MSERR_OK, GST_STATE_CHANGE_FAILURE);
            }
            if (sink->pause_cache_buffer != nullptr) {
                GST_INFO_OBJECT(basesink, "pause to play");
                g_return_val_if_fail(gst_audio_server_sink_render(basesink,
                    sink->pause_cache_buffer) == GST_FLOW_OK, GST_STATE_CHANGE_FAILURE);
                gst_buffer_unref(sink->pause_cache_buffer);
                sink->pause_cache_buffer = nullptr;
            }
            break;
        default:
            break;
    }
    GstStateChangeReturn ret = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);

    switch (transition) {
        case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
            {
                std::unique_lock<std::mutex> lock(sink->mutex_);
                g_return_val_if_fail(sink->audio_sink != nullptr, GST_STATE_CHANGE_FAILURE);
                g_return_val_if_fail(sink->audio_sink->Pause() == MSERR_OK, GST_STATE_CHANGE_FAILURE);
            }
            break;
        case GST_STATE_CHANGE_PAUSED_TO_READY:
            gst_audio_server_sink_clear_cache_buffer(sink);
            sink->is_start = FALSE;
            break;
        default:
            break;
    }

    return ret;
}

static GstFlowReturn gst_audio_server_sink_render(GstBaseSink *basesink, GstBuffer *buffer)
{
    g_return_val_if_fail(basesink != nullptr, GST_FLOW_ERROR);
    g_return_val_if_fail(buffer != nullptr, GST_FLOW_ERROR);
    g_return_val_if_fail(gst_buffer_get_size(buffer) != 0, GST_FLOW_OK);
    GstAudioServerSink *sink = GST_AUDIO_SERVER_SINK(basesink);
    g_return_val_if_fail(sink != nullptr, GST_FLOW_ERROR);
    g_return_val_if_fail(sink->audio_sink != nullptr, GST_FLOW_ERROR);

    if (sink->enable_cache) {
        return gst_audio_server_sink_cache_render(sink, buffer);
    }

    {
        std::unique_lock<std::mutex> lock(sink->mutex_);
        if (!sink->audio_sink->Writeable()) {
            if (sink->pause_cache_buffer == nullptr) {
                sink->pause_cache_buffer = gst_buffer_ref(buffer);
                g_return_val_if_fail(sink->pause_cache_buffer != nullptr, GST_FLOW_ERROR);
                GST_INFO_OBJECT(basesink, "cache buffer for pause state");
                return GST_FLOW_OK;
            } else {
                GST_ERROR_OBJECT(basesink, "cache buffer is not null");
            }
        }

        GstMapInfo map;
        if (gst_buffer_map(buffer, &map, GST_MAP_READ) != TRUE) {
            GST_ERROR_OBJECT(basesink, "unknown error happened during gst_buffer_map");
            return GST_FLOW_ERROR;
        }
        if (sink->audio_sink->Write(map.data, map.size) != MSERR_OK) {
            GST_ERROR_OBJECT(basesink, "unknown error happened during Write");
            gst_buffer_unmap(buffer, &map);
            return GST_FLOW_ERROR;
        }
        gst_buffer_unmap(buffer, &map);
    }

    g_mutex_lock(&sink->render_lock);
    if (sink->frame_after_segment) {
        sink->frame_after_segment = FALSE;
        uint64_t latency = 0;
        GST_INFO_OBJECT(basesink, "the first audio frame after segment has been sent to audio server");
        if (sink->audio_sink->GetLatency(latency) != MSERR_OK) {
            GST_INFO_OBJECT(basesink, "fail to get latency");
        } else {
            GST_INFO_OBJECT(basesink, "frame render latency is (%" PRIu64 ")", latency);
        }
    }
    g_mutex_unlock(&sink->render_lock);

    return GST_FLOW_OK;
}

static gboolean plugin_init(GstPlugin *plugin)
{
    g_return_val_if_fail(plugin != nullptr, FALSE);
    gboolean ret = gst_element_register(plugin, "audioserversink", GST_RANK_PRIMARY, GST_TYPE_AUDIO_SERVER_SINK);
    return ret;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    _audio_server_sink,
    "GStreamer Audio Server Sink",
    plugin_init,
    PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
