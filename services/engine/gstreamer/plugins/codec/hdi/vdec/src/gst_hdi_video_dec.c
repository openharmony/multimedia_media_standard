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

#include "gst_hdi_video_dec.h"
#include <inttypes.h>
#include "gst_dec_surface.h"
#include "securec.h"

#define GST_HDI_VIDEO_DEC_SUPPORTED_FORMATS "{ NV21 }"

GST_DEBUG_CATEGORY_STATIC(gst_hdi_video_dec_debug_category);
#define GST_CAT_DEFAULT gst_hdi_video_dec_debug_category

#define DEBUG_INIT \
    GST_DEBUG_CATEGORY_INIT(gst_hdi_video_dec_debug_category, "hdivideodec", 0, \
        "debug category for gst-hdi video decoder base class");
#define GST_1080P_STREAM_WIDTH (1920)
#define GST_1080P_STREAM_HEIGHT (1088)

static constexpr guint GET_BUFFER_TIMEOUT_MS = 10u;
static constexpr gint DEFAULT_HDI_BUFFER_SIZE = 0;
static constexpr PixelFormat DEFAULT_HDI_PIXEL_FORMAT = YVU_SEMIPLANAR_420;
static constexpr gint RETRY_SLEEP_UTIME = 10000;

static void gst_hdi_video_dec_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void gst_hdi_video_dec_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static gboolean gst_hdi_video_dec_open(GstVideoDecoder *decoder);
static gboolean gst_hdi_video_dec_close(GstVideoDecoder *decoder);
static gboolean gst_hdi_video_dec_start(GstVideoDecoder *decoder);
static gboolean gst_hdi_video_dec_stop(GstVideoDecoder *decoder);
static gboolean gst_hdi_video_dec_set_format(GstVideoDecoder *decoder, GstVideoCodecState *state);
static gboolean gst_hdi_video_dec_flush(GstVideoDecoder *decoder);
static GstFlowReturn gst_hdi_video_dec_handle_frame(GstVideoDecoder *decoder, GstVideoCodecFrame *frame);
static GstFlowReturn gst_hdi_video_dec_drain(GstVideoDecoder *decoder);
static void gst_hdi_video_dec_finalize(GObject *object);
static GstFlowReturn gst_hdi_video_dec_finish(GstVideoDecoder *decoder);
static void gst_hdi_video_dec_loop(GstHDIVideoDec *self);
static void gst_hdi_video_dec_pause_loop(GstHDIVideoDec *self, GstFlowReturn flow_ret);
static gboolean gst_hdi_video_dec_sink_event(GstVideoDecoder *decoder, GstEvent *event);

enum {
    PROP_0,
    PROP_SURFACE,
};

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(GstHDIVideoDec, gst_hdi_video_dec, GST_TYPE_VIDEO_DECODER, DEBUG_INIT);

static void gst_hdi_video_dec_class_init(GstHDIVideoDecClass *klass)
{
    g_return_if_fail(klass != NULL);
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstVideoDecoderClass *video_decoder_class = GST_VIDEO_DECODER_CLASS(klass);
    gobject_class->set_property = gst_hdi_video_dec_set_property;
    gobject_class->get_property = gst_hdi_video_dec_get_property;
    gobject_class->finalize = gst_hdi_video_dec_finalize;
    video_decoder_class->open = gst_hdi_video_dec_open;
    video_decoder_class->close = gst_hdi_video_dec_close;
    video_decoder_class->start = gst_hdi_video_dec_start;
    video_decoder_class->stop = gst_hdi_video_dec_stop;
    video_decoder_class->flush = gst_hdi_video_dec_flush;
    video_decoder_class->set_format = gst_hdi_video_dec_set_format;
    video_decoder_class->handle_frame = gst_hdi_video_dec_handle_frame;
    video_decoder_class->finish = gst_hdi_video_dec_finish;
    video_decoder_class->sink_event = gst_hdi_video_dec_sink_event;
    video_decoder_class->drain = gst_hdi_video_dec_drain;
    klass->cdata.default_src_template_caps = GST_VIDEO_CAPS_MAKE(GST_HDI_VIDEO_DEC_SUPPORTED_FORMATS);
    g_object_class_install_property(gobject_class, PROP_SURFACE,
    g_param_spec_pointer("surface", "Surface", "The surface which gets the buffers. ",
        (GParamFlags) (G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));
}

static void gst_hdi_video_dec_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
    g_return_if_fail(object != NULL);
    g_return_if_fail(value != NULL);
    GstHDIVideoDec *self = GST_HDI_VIDEO_DEC(object);
    GST_DEBUG_OBJECT(object, "set hdidec gst_hdi_video_dec_set_property");

    switch (property_id) {
        case PROP_SURFACE:
            self->surface = g_value_get_pointer(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gst_hdi_video_dec_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    (void)value;
    g_return_if_fail(object != NULL);
    switch (property_id) {
        default: {
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
        }
    }
}

static void gst_hdi_video_dec_init(GstHDIVideoDec *self)
{
    g_return_if_fail(self != NULL);

    gst_video_decoder_set_packetized(GST_VIDEO_DECODER(self), TRUE);
    gst_video_decoder_set_use_default_pad_acceptcaps(GST_VIDEO_DECODER_CAST (self), TRUE);
    g_return_if_fail(GST_VIDEO_DECODER_SINK_PAD(self) != NULL);
    GST_PAD_SET_ACCEPT_TEMPLATE (GST_VIDEO_DECODER_SINK_PAD(self));
    g_mutex_init(&self->lock);
    g_mutex_init(&self->drain_lock);
    g_cond_init(&self->drain_cond);
}

static void gst_hdi_video_dec_finalize(GObject *object)
{
    GST_DEBUG_OBJECT(NULL, "finalize the hdi ins");
    g_return_if_fail(object != NULL);
    GstHDIVideoDec *self = GST_HDI_VIDEO_DEC(object);

    g_mutex_clear(&self->drain_lock);
    g_cond_clear(&self->drain_cond);
    g_mutex_clear(&self->lock);
    if (G_OBJECT_CLASS(gst_hdi_video_dec_parent_class)) {
        G_OBJECT_CLASS(gst_hdi_video_dec_parent_class)->finalize(object);
    }
}

static gboolean gst_hdi_video_dec_open(GstVideoDecoder *decoder)
{
    g_return_val_if_fail(decoder != NULL, FALSE);
    GstHDIVideoDec *self = GST_HDI_VIDEO_DEC(decoder);
    GstHDIVideoDecClass *klass = GST_HDI_VIDEO_DEC_GET_CLASS(self);
    GstHDIFormat format;
    format.mime = klass->cdata.mime;
    format.width = klass->cdata.max_width;
    format.height = klass->cdata.max_height;
    format.codec_type = klass->cdata.codec_type;
    format.buffer_size = DEFAULT_HDI_BUFFER_SIZE;
    format.pixel_format = DEFAULT_HDI_PIXEL_FORMAT;
    GST_DEBUG_OBJECT(self, "Opening decoder");

    self->dec = gst_hdi_codec_new(&klass->cdata, &format);
    g_return_val_if_fail(self->dec != NULL, FALSE);
    if (gst_hdi_alloc_buffers(self->dec) == FALSE) {
        gst_hdi_codec_unref(self->dec);
        self->dec = NULL;
    }
    return TRUE;
}

static void gst_hdi_set_last_upstream_ts(GstHDIVideoDec *self, const GstClockTime last_upstream_ts)
{
    g_return_if_fail(self != NULL);
    g_mutex_lock(&self->lock);
    self->last_upstream_ts = last_upstream_ts;
    g_mutex_unlock(&self->lock);
}

static gboolean gst_hdi_get_task_start(GstHDIVideoDec *self)
{
    g_return_val_if_fail(self != NULL, FALSE);
    g_mutex_lock(&self->lock);
    gboolean start = self->started;
    g_mutex_unlock(&self->lock);
    return start;
}

static void gst_hdi_set_task_start(GstHDIVideoDec *self, const gboolean start)
{
    g_return_if_fail(self != NULL);
    g_mutex_lock(&self->lock);
    self->started = start;
    g_mutex_unlock(&self->lock);
}

static GstFlowReturn gst_hdi_get_downstream_flow_ret(GstHDIVideoDec *self)
{
    g_return_val_if_fail(self != NULL, GST_FLOW_ERROR);
    g_mutex_lock(&self->lock);
    GstFlowReturn downstream_flow_ret = self->downstream_flow_ret;
    g_mutex_unlock(&self->lock);
    return downstream_flow_ret;
}

static void gst_hdi_set_downstream_flow_ret(GstHDIVideoDec *self, const GstFlowReturn downstream_flow_ret)
{
    g_return_if_fail(self != NULL);
    g_mutex_lock(&self->lock);
    self->downstream_flow_ret = downstream_flow_ret;
    g_mutex_unlock(&self->lock);
}

static gboolean gst_hdi_get_flushing(GstHDIVideoDec *self)
{
    g_return_val_if_fail(self != NULL, FALSE);
    g_mutex_lock(&self->lock);
    gboolean hdi_flushing = self->hdi_flushing;
    g_mutex_unlock(&self->lock);
    return hdi_flushing;
}

static void gst_hdi_set_flushing(GstHDIVideoDec *self, const gboolean flushing)
{
    g_return_if_fail(self != NULL);
    g_mutex_lock(&self->lock);
    self->hdi_flushing = flushing;
    g_mutex_unlock(&self->lock);
}

static gboolean gst_hdi_get_task_pausing(GstHDIVideoDec *self)
{
    g_return_val_if_fail(self != NULL, FALSE);
    g_mutex_lock(&self->lock);
    gboolean pausing = self->pausing_task;
    g_mutex_unlock(&self->lock);
    return pausing;
}

static void gst_hdi_set_task_pausing(GstHDIVideoDec *self, const gboolean pausing)
{
    g_return_if_fail(self != NULL);
    g_mutex_lock(&self->lock);
    self->pausing_task = pausing;
    g_mutex_unlock(&self->lock);
}

static gboolean gst_hdi_video_dec_close(GstVideoDecoder *decoder)
{
    g_return_val_if_fail(decoder != NULL, FALSE);
    GstHDIVideoDec *self = GST_HDI_VIDEO_DEC(decoder);

    GST_DEBUG_OBJECT(self, "Closing decoder");
    gst_hdi_release_buffers(self->dec);
    if (self->dec) {
        gst_hdi_codec_unref(self->dec);
    }
    self->dec = NULL;

    gst_hdi_set_task_start(self, FALSE);
    return TRUE;
}

static gboolean gst_hdi_video_dec_start(GstVideoDecoder *decoder)
{
    g_return_val_if_fail(decoder != NULL, FALSE);
    GstHDIVideoDec *self = GST_HDI_VIDEO_DEC(decoder);
    GST_DEBUG_OBJECT(self, "dec start");
    self->started = FALSE;
    g_mutex_lock(&self->drain_lock);
    self->draining = FALSE;
    g_cond_broadcast (&self->drain_cond);
    g_mutex_unlock(&self->drain_lock);

    g_mutex_lock(&self->lock);
    self->last_upstream_ts = 0;
    self->downstream_flow_ret = GST_FLOW_OK;
    self->useBuffers = FALSE;
    self->started = FALSE;
    self->pausing_task = FALSE;
    g_mutex_unlock(&self->lock);

    return TRUE;
}

static gboolean gst_hdi_video_dec_stop(GstVideoDecoder *decoder)
{
    g_return_val_if_fail(decoder != NULL, FALSE);
    GstHDIVideoDec *self = GST_HDI_VIDEO_DEC(decoder);

    GST_DEBUG_OBJECT(self, "Stopping decoder");
    gst_hdi_set_task_pausing(self, TRUE);
    gst_pad_stop_task(GST_VIDEO_DECODER_SRC_PAD(decoder));

    gst_hdi_set_downstream_flow_ret(self, GST_FLOW_FLUSHING);
    gst_hdi_set_task_start(self, FALSE);
    gst_hdi_set_task_pausing(self, FALSE);

    g_mutex_lock(&self->drain_lock);
    self->draining = FALSE;
    g_cond_broadcast(&self->drain_cond);
    g_mutex_unlock(&self->drain_lock);

    gint ret = gst_hdi_codec_stop(self->dec);
    if (ret != HDI_SUCCESS) {
        GST_ERROR_OBJECT(self, "decoder stop failed %d", ret);
    }
    if (self->input_state) {
        gst_video_codec_state_unref(self->input_state);
    }
    self->input_state = NULL;
    GST_DEBUG_OBJECT(self, "Stopped decoder");

    return TRUE;
}

static gboolean gst_hdi_video_dec_flush(GstVideoDecoder *decoder)
{
    g_return_val_if_fail(decoder != NULL, FALSE);
    GstHDIVideoDec *self = GST_HDI_VIDEO_DEC(decoder);

    GST_DEBUG_OBJECT(self, "Flushing decoder");

    GST_VIDEO_DECODER_STREAM_UNLOCK(self);
    gst_hdi_set_task_pausing(self, TRUE);
    gst_pad_stop_task(GST_VIDEO_DECODER_SRC_PAD(decoder));
    GST_VIDEO_DECODER_STREAM_LOCK(self);

    gint ret = gst_hdi_port_flush(self->dec, ALL_TYPE);
    if (ret != HDI_SUCCESS) {
      GST_DEBUG_OBJECT(self, "Failed to populate output port: %d", ret);
    }
    gst_hdi_set_last_upstream_ts(self, 0);
    gst_hdi_set_downstream_flow_ret(self, GST_FLOW_OK);
    gst_hdi_set_flushing(self, FALSE);
    gst_hdi_set_task_start(self, FALSE);
    gst_hdi_set_task_pausing(self, FALSE);
    GST_DEBUG_OBJECT(self, "Flush finished");

    return TRUE;
}

static GstHDIBufferMode gst_hdi_video_dec_pick_input_buffer_mode(const GstHDIVideoDec *self)
{
    GstHDIVideoDecClass *klass = GST_HDI_VIDEO_DEC_GET_CLASS(self);
    if (klass->cdata.input_buffer_support & GST_HDI_BUFFER_EXTERNAL_SUPPORT) {
        return GST_HDI_BUFFER_EXTERNAL_MODE;
    }
    return GST_HDI_BUFFER_INTERNAL_MODE;
}

static gboolean gst_hdi_video_dec_negotiate(const GstHDIVideoDec *self)
{
    g_return_val_if_fail(self != NULL, FALSE);
    g_return_val_if_fail(GST_VIDEO_DECODER_SRC_PAD(self) != NULL, FALSE);
    GstCaps *templ_caps = NULL;
    GstCaps *intersection = NULL;
    GstVideoFormat format;
    const gchar *format_str = NULL;

    GST_DEBUG_OBJECT(self, "Trying to negotiate a video format with downstream");

    templ_caps = gst_pad_get_pad_template_caps(GST_VIDEO_DECODER_SRC_PAD(self));
    intersection = gst_pad_peer_query_caps(GST_VIDEO_DECODER_SRC_PAD(self), templ_caps);
    gst_caps_unref(templ_caps);

    GST_DEBUG_OBJECT(self, "Allowed downstream caps");

    if (gst_caps_is_empty(intersection)) {
        gst_caps_unref(intersection);
        GST_ERROR_OBJECT(self, "Empty caps");
        return FALSE;
    }

    intersection = gst_caps_truncate(intersection);
    intersection = gst_caps_fixate(intersection);

    GstStructure *s = gst_caps_get_structure(intersection, 0);
    format_str = gst_structure_get_string(s, "format");
    if (format_str == NULL || (format = gst_video_format_from_string (format_str)) == GST_VIDEO_FORMAT_UNKNOWN) {
        GST_ERROR_OBJECT(self, "Invalid caps");
        gst_caps_unref(intersection);
        return FALSE;
    }

    gst_caps_unref(intersection);
    return TRUE;
}

static gint gst_dec_queue_input_buffer(GstHDIVideoDec *self, GstBuffer *gst_buffer)
{
    gboolean done = FALSE;
    gint ret = HDI_SUCCESS;
    while (!done) {
        done = TRUE;
        ret = gst_hdi_queue_input_buffer(self->dec, gst_buffer, GET_BUFFER_TIMEOUT_MS);
        if (ret == HDI_ERR_STREAM_BUF_FULL) {
            done = FALSE;
        }
        if (!gst_hdi_get_task_start(self)) {
            GST_INFO_OBJECT(self, "decoder is not started stop queue input buffer");
            done = TRUE;
        }
    }
    return ret;
}

static gboolean gst_hdi_video_dec_enable(GstHDIVideoDec *self)
{
    g_return_val_if_fail(self != NULL, FALSE);
    g_return_val_if_fail(self->dec != NULL, FALSE);
    GST_DEBUG_OBJECT(self, "Enabling codec");
    if (gst_hdi_codec_is_start(self->dec)) {
        GST_DEBUG_OBJECT(self, "codec is enable already");
        return TRUE;
    }

    self->inputBufferMode = gst_hdi_video_dec_pick_input_buffer_mode(self);

    if (!gst_hdi_video_dec_negotiate(self)) {
        GST_ERROR_OBJECT(self, "Negotiation failed");
        return FALSE;
    }

    if (gst_hdi_codec_start(self->dec) != HDI_SUCCESS) {
        GST_ERROR_OBJECT(self, "start hdi decoder failed");
        return FALSE;
    }
    if (gst_hdi_port_flush(self->dec, ALL_TYPE) != HDI_SUCCESS) {
        GST_ERROR_OBJECT(self, "flush err");
    }
    return TRUE;
}

static gint gst_dec_deque_input_buffer(GstHDIVideoDec *self, GstBuffer **gst_buffer)
{
    g_return_val_if_fail(self != NULL, GST_FLOW_ERROR);
    gboolean done = FALSE;
    gint ret = HDI_SUCCESS;
    while (!done) {
        done = TRUE;
        ret = gst_hdi_deque_input_buffer(self->dec, gst_buffer, GET_BUFFER_TIMEOUT_MS);
        if (ret == HDI_ERR_FRAME_BUF_EMPTY) {
            done = FALSE;
        }
        if (!gst_hdi_get_task_start(self)) {
            GST_INFO_OBJECT(self, "decoder is not started stop deque input buffer");
            done = TRUE;
        }
    }
    return ret;
}

static GstFlowReturn gst_dec_get_gst_buffer_from_frame(GstHDIVideoDec *self,
    const GstVideoCodecFrame *frame, GstBuffer **gst_buffer)
{
    g_return_val_if_fail(self != NULL, GST_FLOW_ERROR);
    g_return_val_if_fail(frame != NULL, GST_FLOW_ERROR);
    g_return_val_if_fail(frame->input_buffer != NULL, GST_FLOW_ERROR);
    gint ret = HDI_SUCCESS;
    GstClockTime timestamp = frame->pts;
    GST_DEBUG_OBJECT(self, "PTS %" GST_TIME_FORMAT ", DTS %" GST_TIME_FORMAT
        ", dist %d", GST_TIME_ARGS(frame->pts), GST_TIME_ARGS (frame->dts),
        frame->distance_from_sync);
    GST_DEBUG_OBJECT(self, "input mode %d", self->inputBufferMode);
    if (self->inputBufferMode == GST_HDI_BUFFER_EXTERNAL_MODE) {
        *gst_buffer = frame->input_buffer;
    } else {
        ret = gst_dec_deque_input_buffer(self, gst_buffer);
        if (ret == HDI_ERR_FRAME_BUF_EMPTY) {
            return GST_FLOW_FLUSHING;
        }
        if (ret != HDI_SUCCESS) {
            return GST_FLOW_ERROR;
        }
        g_return_val_if_fail(*gst_buffer != NULL, GST_FLOW_ERROR);
        gst_buffer_copy_into(*gst_buffer, frame->input_buffer, 0, 0, -1);
    }
    GST_BUFFER_PTS(*gst_buffer) = timestamp;
    if (!GST_VIDEO_CODEC_FRAME_IS_SYNC_POINT(frame)) {
        gst_buffer_set_flags(*gst_buffer, GST_BUFFER_FLAG_DELTA_UNIT);
    }
    return GST_FLOW_OK;
}

static GstFlowReturn gst_hdi_video_dec_deal_frame(GstHDIVideoDec *self, const GstVideoCodecFrame *frame)
{
    g_return_val_if_fail(self != NULL, GST_FLOW_ERROR);
    g_return_val_if_fail(frame != NULL, GST_FLOW_ERROR);
    GST_DEBUG_OBJECT(NULL, "queue input buffer");
    GstFlowReturn ret = GST_FLOW_OK;
    GstBuffer *buffer = NULL;
    ret = gst_dec_get_gst_buffer_from_frame(self, frame, &buffer);
    g_return_val_if_fail(ret == GST_FLOW_OK, GST_FLOW_ERROR);
    ret = gst_dec_queue_input_buffer(self, buffer);
    if (ret == HDI_ERR_STREAM_BUF_FULL) {
        return GST_FLOW_FLUSHING;
    }
    if (ret != HDI_SUCCESS) {
        return GST_FLOW_ERROR;
    }
    return GST_FLOW_OK;
}

static void gst_hdi_video_dec_clean_all_frames(GstVideoDecoder *decoder)
{
    g_return_if_fail(decoder != NULL);
    GList *frame_head = gst_video_decoder_get_frames(decoder);

    g_return_if_fail(frame_head != NULL);
    for (GList *frame = frame_head; frame != NULL; frame = frame->next) {
        GstVideoCodecFrame *tmp = frame->data;
        if (tmp == NULL) {
            continue;
        }
        gst_video_decoder_release_frame(decoder, tmp);
    }
    g_list_free(frame_head);
    return;
}

static GstFlowReturn gst_hdi_video_dec_handle_frame(GstVideoDecoder *decoder, GstVideoCodecFrame *frame)
{
    g_return_val_if_fail(decoder != NULL, GST_FLOW_ERROR);
    g_return_val_if_fail(frame != NULL, GST_FLOW_ERROR);
    GstHDIVideoDec *self = GST_HDI_VIDEO_DEC(decoder);
    if (gst_hdi_get_downstream_flow_ret(self) != GST_FLOW_OK) {
        gst_video_codec_frame_unref(frame);
        GST_WARNING_OBJECT(self, "downstream_flow_ret %d", self->downstream_flow_ret);
        return gst_hdi_get_downstream_flow_ret(self);
    }
    if (gst_hdi_get_flushing(self)) {
        gst_video_codec_frame_unref(frame);
        return GST_FLOW_FLUSHING;
    }

    gst_hdi_video_dec_clean_all_frames(decoder);
    if (!gst_hdi_get_task_start(self)) {
        if (!GST_VIDEO_CODEC_FRAME_IS_SYNC_POINT(frame)) {
            gst_video_decoder_drop_frame(GST_VIDEO_DECODER(self), frame);
            return GST_FLOW_OK;
        }

        if (!gst_hdi_video_dec_enable(self)) {
            GST_WARNING_OBJECT(self, "hdi video dec enable failed");
            gst_video_codec_frame_unref(frame);
            return GST_FLOW_ERROR;
        }

        gst_hdi_set_task_start(self, TRUE);
        GST_DEBUG_OBJECT(self, "Starting task");
        gst_pad_start_task(GST_VIDEO_DECODER_SRC_PAD(self), (GstTaskFunction)gst_hdi_video_dec_loop, decoder, NULL);
    }
    GST_VIDEO_DECODER_STREAM_UNLOCK(self);
    GstFlowReturn ret = gst_hdi_video_dec_deal_frame(self, frame);
    GST_VIDEO_DECODER_STREAM_LOCK(self);

    gst_video_codec_frame_unref(frame);
    GST_DEBUG_OBJECT(self, "Passed frame to component");
    return ret;
}

static void gst_hdi_video_dec_loop_invalid_buffer_err(GstHDIVideoDec *self)
{
    g_return_if_fail(self != NULL);
    GST_ELEMENT_ERROR(self, LIBRARY, SETTINGS, (NULL), ("Invalid output buffer"));
    gst_pad_push_event(GST_VIDEO_DECODER_SRC_PAD (self), gst_event_new_eos ());
    gst_hdi_video_dec_pause_loop(self, GST_FLOW_ERROR);
}

static void gst_hdi_video_dec_loop_flow_err(GstHDIVideoDec *self, GstFlowReturn flow_ret)
{
    g_return_if_fail(self != NULL);
    GST_DEBUG_OBJECT(NULL, "loop flow err");
    if (flow_ret == GST_FLOW_EOS) {
        GST_DEBUG_OBJECT(self, "EOS");
        gst_pad_push_event(GST_VIDEO_DECODER_SRC_PAD(self), gst_event_new_eos());
    } else if (flow_ret < GST_FLOW_EOS) {
        GST_ELEMENT_ERROR(self, STREAM, FAILED, ("Internal data stream error."), ("stream stopped, reason %s",
            gst_flow_get_name(flow_ret)));
        gst_pad_push_event(GST_VIDEO_DECODER_SRC_PAD(self), gst_event_new_eos());
    } else if (flow_ret == GST_FLOW_FLUSHING) {
        GST_DEBUG_OBJECT(self, "Flushing -- stopping task");
    }
    gst_hdi_video_dec_pause_loop(self, flow_ret);
}

static void gst_hdi_video_dec_loop_hdi_eos(GstHDIVideoDec *self)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(GST_VIDEO_DECODER_SRC_PAD(self) != NULL);
    GST_DEBUG_OBJECT(NULL, "loop hdi eos");
    GstFlowReturn flow_ret = GST_FLOW_OK;
    g_mutex_lock(&self->drain_lock);
    if (self->draining) {
        GstQuery *query = gst_query_new_drain();

        if (!gst_pad_peer_query(GST_VIDEO_DECODER_SRC_PAD(self), query)) {
            GST_WARNING_OBJECT(self, "drain query failed");
        }
        gst_query_unref(query);

        GST_DEBUG_OBJECT(self, "Drained");
        self->draining = FALSE;
        g_cond_broadcast(&self->drain_cond);
        flow_ret = GST_FLOW_OK;
        gst_pad_pause_task(GST_VIDEO_DECODER_SRC_PAD(self));
    } else {
        GST_DEBUG_OBJECT(self, "codec signalled EOS");
        flow_ret = GST_FLOW_EOS;
    }
    g_mutex_unlock(&self->drain_lock);

    gst_hdi_set_downstream_flow_ret(self, flow_ret);
    if (flow_ret != GST_FLOW_OK) {
        gst_hdi_video_dec_loop_flow_err(self, flow_ret);
    }
}

static void gst_hdi_video_dec_loop_hdi_error(GstHDIVideoDec *self, gint ret)
{
    g_return_if_fail(self != NULL);
    GST_ELEMENT_ERROR(self, LIBRARY, FAILED, (NULL), ("decoder hdi in error state %s (%d)",
        gst_hdi_error_to_string(ret), ret));
    gst_pad_push_event (GST_VIDEO_DECODER_SRC_PAD(self), gst_event_new_eos());
    gst_hdi_video_dec_pause_loop(self, GST_FLOW_ERROR);
}

static void gst_hdi_update_video_meta(const GstHDIVideoDec *self, GstBuffer *outbuf)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(outbuf != NULL);
    GST_DEBUG_OBJECT(NULL, "update video meta");
    GstVideoMeta *video_meta = gst_buffer_get_video_meta(outbuf);
    gint width = self->hdi_video_out_format.width;
    gint height = self->hdi_video_out_format.height;
    if (video_meta == NULL) {
        gst_buffer_add_video_meta(
            outbuf, GST_VIDEO_FRAME_FLAG_NONE, GST_VIDEO_FORMAT_NV21, width, height);
    } else {
        video_meta->width = width;
        video_meta->height = height;
    }
}

static gint gst_hdi_get_out_buffer(GstHDIVideoDec *self, GstBuffer **gst_buffer)
{
    g_return_val_if_fail(self != NULL, HDI_FAILURE);
    g_return_val_if_fail(gst_buffer != NULL, HDI_FAILURE);
    g_return_val_if_fail(self->dec != NULL, HDI_FAILURE);
    gint ret = HDI_SUCCESS;
    gboolean done = FALSE;
    while (!done) {
        ret = gst_hdi_queue_output_buffers(self->dec, GET_BUFFER_TIMEOUT_MS);
        if (ret != HDI_SUCCESS) {
            GST_DEBUG_OBJECT(self, "hdi output buffer queue fail");
            break;
        }
        if (gst_hdi_get_task_pausing(self)) {
            return HDI_FAILURE;
        }
        done = TRUE;
#ifdef GST_HDI_PARAM_PILE
        ret = gst_hdi_deque_output_buffer_and_format(self->dec, gst_buffer,
            &self->hdi_video_out_format, GET_BUFFER_TIMEOUT_MS);
#else
        ret = gst_hdi_deque_output_buffer(self->dec, gst_buffer, GET_BUFFER_TIMEOUT_MS);
#endif
        if (ret == HDI_ERR_FRAME_BUF_EMPTY) {
            GST_DEBUG_OBJECT(self, "hdi output buffer empty");
            done = FALSE;
        }
    }
    return ret;
}

static gboolean gst_hdi_video_dec_fill_surface_buffer(const GstHDIVideoDec *self, const GstVideoCodecFrame *frame,
    GstBuffer *outbuf)
{
    g_return_val_if_fail(self != NULL, FALSE);
    g_return_val_if_fail(frame != NULL, FALSE);
    g_return_val_if_fail(outbuf != NULL, FALSE);
    g_return_val_if_fail(frame->output_buffer != NULL, FALSE);
    gint size = self->hdi_video_out_format.buffer_size;
    gint64 start = g_get_monotonic_time();

    GstMapInfo info = GST_MAP_INFO_INIT;
    if (!gst_buffer_map(outbuf, &info, GST_MAP_READ)) {
        return FALSE;
    }
    guint8 *dts = GetSurfaceBufferVirAddr(frame->output_buffer);
    guint8 *src = info.data;
    if (dts == NULL || src == NULL || memcpy_s(dts, GetSurfaceBufferSize(frame->output_buffer), src, size) != EOK) {
        gst_buffer_unmap(outbuf, &info);
        GST_ERROR_OBJECT(self, "memcpy_s failed");
        return FALSE;
    }
    gst_buffer_unmap(outbuf, &info);

    gint64 end = g_get_monotonic_time();
#ifdef GST_HDI_PARAM_PILE
    g_return_val_if_fail(self->hdi_video_out_format.vir_addr != NULL, FALSE);
    HI_MPI_SYS_Munmap(self->hdi_video_out_format.vir_addr, size);
#endif
    GST_DEBUG_OBJECT(self, "memcpy_s s %u %" PRId64 " ", gst_buffer_get_size(outbuf), end - start);
    GST_BUFFER_PTS (frame->output_buffer) = GST_BUFFER_PTS (outbuf);
    return TRUE;
}

static gboolean gst_hdi_video_dec_fill_gst_buffer(const GstHDIVideoDec *self, const GstVideoCodecFrame *frame,
    GstBuffer *outbuf)
{
    g_return_val_if_fail(self != NULL, FALSE);
    g_return_val_if_fail(frame != NULL, FALSE);
    g_return_val_if_fail(outbuf != NULL, FALSE);
    g_return_val_if_fail(frame->output_buffer != NULL, FALSE);
    gint64 start = g_get_monotonic_time();
    g_return_val_if_fail(gst_buffer_copy_into(frame->output_buffer, outbuf, 0, 0, -1), FALSE);
    gint64 end = g_get_monotonic_time();
#ifdef GST_HDI_PARAM_PILE
    g_return_val_if_fail(self->hdi_video_out_format.vir_addr != NULL, FALSE);
    HI_MPI_SYS_Munmap(self->hdi_video_out_format.vir_addr, self->hdi_video_out_format.buffer_size);
#endif
    GST_DEBUG_OBJECT(self, "memcpy_s s %u %" PRId64 " ", gst_buffer_get_size(outbuf), end - start);
    GST_BUFFER_PTS (frame->output_buffer) = GST_BUFFER_PTS (outbuf);
    return TRUE;
}

static gint gst_hdi_finish_frame(GstHDIVideoDec *self, GstVideoCodecFrame *frame, GstBuffer *outbuf)
{
    gint ret = HDI_SUCCESS;
    GstFlowReturn flow_ret = GST_FLOW_OK;
    if (self->surface) {
        if (!gst_hdi_video_dec_fill_surface_buffer(self, frame, outbuf)) {
            GST_ERROR_OBJECT(self, "fill surface buffer error");
            gst_buffer_unref(frame->output_buffer);
            flow_ret = gst_video_decoder_drop_frame(GST_VIDEO_DECODER(self), frame);
            frame = NULL;
            gst_buffer_unref(outbuf);
            gst_hdi_video_dec_loop_invalid_buffer_err(self);
            return ret;
        }
    } else {
        if (!gst_hdi_video_dec_fill_gst_buffer(self, frame, outbuf)) {
            GST_ERROR_OBJECT(self, "fill surface buffer error");
            gst_buffer_unref(frame->output_buffer);
            flow_ret = gst_video_decoder_drop_frame(GST_VIDEO_DECODER(self), frame);
            frame = NULL;
            gst_buffer_unref(outbuf);
            gst_hdi_video_dec_loop_invalid_buffer_err(self);
            return ret;
        }
    }
    gst_buffer_unref(outbuf);
    gst_hdi_update_video_meta(self, frame->output_buffer);
    flow_ret = gst_video_decoder_finish_frame(GST_VIDEO_DECODER(self), frame);
    frame = NULL;
    GST_INFO_OBJECT(self, "Finished frame: %s", gst_flow_get_name(flow_ret));
    gst_hdi_set_downstream_flow_ret(self, flow_ret);
    if (flow_ret != GST_FLOW_OK) {
        gst_hdi_video_dec_loop_flow_err(self, flow_ret);
    }
    return ret;
}

static GstVideoCodecFrame *gst_hdi_video_dec_new_frame()
{
    GstVideoCodecFrame *frame = g_slice_new0 (GstVideoCodecFrame);
    if (frame == NULL) {
        return NULL;
    }
    frame->ref_count = 1;
    frame->system_frame_number = 0;
    frame->decode_frame_number = 0;
    frame->dts = GST_CLOCK_TIME_NONE;
    frame->pts = GST_CLOCK_TIME_NONE;
    frame->duration = GST_CLOCK_TIME_NONE;
    frame->events = NULL;

    return frame;
}

static GstVideoCodecFrame *gst_hdi_get_output_frame(GstHDIVideoDec *self)
{
    GstVideoCodecFrame *frame = gst_hdi_video_dec_new_frame();
    if (frame == NULL) {
        GST_ERROR_OBJECT (self, "new GstVideoCodecFrame failed");
        gst_hdi_video_dec_loop_flow_err(self, GST_FLOW_ERROR);
        return NULL;
    }
    GstBuffer *outbuf = NULL;
    if (!self->surface) {
        outbuf = gst_video_decoder_allocate_output_buffer(GST_VIDEO_DECODER(self));
        frame->output_buffer = outbuf;
        return frame;
    }
    gint surface_try_count = 0;
    gint width = self->hdi_video_out_format.width;
    gint height = self->hdi_video_out_format.height;
    do {
        if (gst_hdi_get_task_pausing(self)) {
            GST_INFO_OBJECT(self, "the hdi task pause exit get surface");
            gst_video_codec_frame_unref(frame);
            return NULL;
        }
        outbuf = SurfaceBufferToGstBuffer(self->surface, width, height);
        if (outbuf == NULL) {
            GST_DEBUG_OBJECT(self, "retry to get surface retry count %d", surface_try_count);
            usleep(RETRY_SLEEP_UTIME);
            surface_try_count++;
        }
    } while (outbuf == NULL);
    frame->output_buffer = outbuf;

    return frame;
}

static void gst_hdi_update_src_caps(const GstHDIVideoDec *self)
{
    g_return_if_fail(self != NULL);
    GstVideoFormat format = gst_hdi_video_pixelformt_to_gstvideoformat(DEFAULT_HDI_PIXEL_FORMAT);
    GstVideoInterlaceMode interlace_mode = GST_VIDEO_INTERLACE_MODE_PROGRESSIVE;
    g_return_if_fail(self->input_state != NULL);
    GstVideoCodecState *state =
        gst_video_decoder_set_interlaced_output_state(
            GST_VIDEO_DECODER(self), format, interlace_mode, self->hdi_video_out_format.width,
            self->hdi_video_out_format.height, self->input_state);

    gst_video_decoder_negotiate(GST_VIDEO_DECODER(self));
    gst_video_codec_state_unref(state);
}

static void gst_hdi_video_dec_loop(GstHDIVideoDec *self)
{
    g_return_if_fail(self != NULL);
    GstBuffer *gst_buffer = NULL;
    gint ret = gst_hdi_get_out_buffer(self, &gst_buffer);
    if (ret == HDI_RECEIVE_EOS) {
        gst_hdi_video_dec_loop_hdi_eos(self);
        return;
    }
    if (ret != HDI_SUCCESS || gst_buffer == NULL) {
        if (gst_hdi_get_task_pausing(self)) {
            return;
        }
        gst_hdi_video_dec_loop_hdi_error(self, ret);
        return;
    }

#ifdef GST_HDI_PARAM_PILE
#else
    ret = gst_hdi_codec_get_params(self->dec, &self->hdi_video_out_format);
    if (ret != HDI_SUCCESS) {
        GST_ERROR_OBJECT (self, "get hdi codec params failed");
    }
#endif
    if (!gst_pad_has_current_caps(GST_VIDEO_DECODER_SRC_PAD(self))) {
        gst_hdi_update_src_caps(self);
    }
    GstVideoCodecFrame *frame = gst_hdi_get_output_frame(self);
    if (frame != NULL) {
        ret = gst_hdi_finish_frame(self, frame, gst_buffer);
    }
    if (ret != HDI_SUCCESS) {
        GST_ERROR_OBJECT(self, "deal buffer error: %s", gst_hdi_error_to_string(ret));
    }
    frame = NULL;
}

static void gst_hdi_video_dec_pause_loop(GstHDIVideoDec *self, GstFlowReturn flow_ret)
{
    g_return_if_fail(self != NULL);
    g_mutex_lock(&self->drain_lock);
    if (self->draining) {
        self->draining = FALSE;
        g_cond_broadcast(&self->drain_cond);
    }
    g_mutex_unlock(&self->drain_lock);
    GST_DEBUG_OBJECT(self, "pause loop.");
    gst_pad_pause_task(GST_VIDEO_DECODER_SRC_PAD(self));
    gst_hdi_set_downstream_flow_ret(self, flow_ret);
    gst_hdi_set_task_start(self, FALSE);
}

static gboolean gst_hdi_video_dec_set_format(GstVideoDecoder *decoder, GstVideoCodecState *state)
{
    g_return_val_if_fail(decoder != NULL, FALSE);
    g_return_val_if_fail(state != NULL, FALSE);
    GstHDIVideoDec *self;
    GstVideoInfo *info = &state->info;
    g_return_val_if_fail(info != NULL, FALSE);
    gboolean is_format_change = FALSE;
    gint ret = HDI_SUCCESS;

    self = GST_HDI_VIDEO_DEC(decoder);

    GST_DEBUG_OBJECT(self, "Setting new caps");

    is_format_change = is_format_change || self->hdi_video_in_format.width != info->width;
    is_format_change = is_format_change ||
        self->hdi_video_in_format.height != GST_VIDEO_INFO_FIELD_HEIGHT(info);
    is_format_change = is_format_change || (self->hdi_video_in_format.frame_rate == 0
        && info->fps_n != 0);

    if (is_format_change) {
      self->hdi_video_in_format.width = info->width;
      self->hdi_video_in_format.height = GST_VIDEO_INFO_FIELD_HEIGHT(info);
      self->hdi_video_in_format.frame_rate  = info->fps_n;
      self->hdi_video_in_format.width = info->width;
      self->hdi_video_in_format.buffer_size = 0;
    }

    GST_DEBUG_OBJECT(self, "Setting inport port definition");
    ret = gst_hdi_codec_set_params(self->dec, &self->hdi_video_in_format);
    if (ret != HDI_SUCCESS) {
      GST_DEBUG_OBJECT(self, "Setting definition failed %d", ret);
    }
    self->input_state = gst_video_codec_state_ref(state);
    gst_hdi_set_downstream_flow_ret(self, GST_FLOW_OK);
    return TRUE;
}

static GstFlowReturn gst_hdi_video_dec_finish(GstVideoDecoder *decoder)
{
    g_return_val_if_fail(decoder != NULL, GST_FLOW_ERROR);
    GstHDIVideoDec *self;
    gint ret = HDI_SUCCESS;
    self = GST_HDI_VIDEO_DEC(decoder);
    GST_DEBUG_OBJECT(self, "finish codec");
    if (!gst_hdi_get_task_start(self)) {
        GST_DEBUG_OBJECT(self, "Component not started yet");
        return GST_FLOW_OK;
    }
    GST_VIDEO_DECODER_STREAM_UNLOCK(self);
    g_mutex_lock(&self->drain_lock);
    self->draining = TRUE;
    ret = gst_dec_queue_input_buffer(self, NULL);
    if (ret != HDI_SUCCESS) {
        GST_ERROR_OBJECT(self, "Failed to queue input buffer for draining: %d", ret);
        g_mutex_unlock(&self->drain_lock);
        GST_VIDEO_DECODER_STREAM_LOCK(self);
        return GST_FLOW_ERROR;
    }
    GST_DEBUG_OBJECT(self, "Waiting until codec is drained");
    gint64 wait_until = g_get_monotonic_time() + G_TIME_SPAN_SECOND;
    if (!g_cond_wait_until(&self->drain_cond, &self->drain_lock, wait_until)) {
        GST_ERROR_OBJECT(self, "Drain timed out");
    } else {
    	GST_DEBUG_OBJECT(self, "finish hdi end");
    }
    g_mutex_unlock(&self->drain_lock);
    GST_VIDEO_DECODER_STREAM_LOCK(self);

    return GST_FLOW_OK;
}

static GstFlowReturn gst_hdi_video_dec_drain(GstVideoDecoder *decoder)
{
    GST_DEBUG_OBJECT(decoder, "Draining codec");
    g_return_val_if_fail(decoder != NULL, GST_FLOW_ERROR);
    GstFlowReturn ret = gst_hdi_video_dec_finish(decoder);
    gst_hdi_video_dec_flush(decoder);
    GST_DEBUG_OBJECT(decoder, "Draining codec finish");
    return ret;
}

static gboolean gst_hdi_video_dec_sink_event(GstVideoDecoder *decoder, GstEvent *event)
{
    g_return_val_if_fail(decoder != NULL, FALSE);
    g_return_val_if_fail(event != NULL, FALSE);
    GstHDIVideoDec *self = GST_HDI_VIDEO_DEC(decoder);
    g_return_val_if_fail(GST_VIDEO_DECODER_CLASS(gst_hdi_video_dec_parent_class) != NULL, FALSE);
    GST_DEBUG_OBJECT(self, "gst_hdi_video_dec_sink_event,type=%#x", GST_EVENT_TYPE(event));

    GstEventType ev_type = GST_EVENT_TYPE(event);

    switch (ev_type) {
        case GST_EVENT_FLUSH_START:
            gst_hdi_set_flushing(self, TRUE);
            break;
        default:
            break;
    }

    gboolean ret = GST_VIDEO_DECODER_CLASS(gst_hdi_video_dec_parent_class)->sink_event(decoder, event);
    return ret;
}