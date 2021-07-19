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
#include "gst_dec_surface.h"
#include "securec.h"

GST_DEBUG_CATEGORY_STATIC (gst_hdi_video_dec_debug_category);
#define GST_CAT_DEFAULT gst_hdi_video_dec_debug_category

#define GST_HDI_VIDEO_DEC_INTERNAL_ENTROPY_BUFFERS_DEFAULT (5)

#define DEBUG_INIT \
  GST_DEBUG_CATEGORY_INIT (gst_hdi_video_dec_debug_category, "hdivideodec", 0, \
      "debug category for gst-hdi video decoder base class");

static const int HDI_INTERFACE_PARAM_MAX_NUM = 30;
const uint32_t GET_BUFFER_TIMEOUT_MS = 10000u;
static const int DEFAULT_HDI_WIDTH = 1920;
static const int DEFAULT_HDI_HEIGHT = 1080;
static const int DEFAULT_HDI_BUFFER_SIZE = 0;
static const PixelFormat DEFAULT_HDI_PIXEL_FORMAT = YVU_SEMIPLANAR_420;
static const GstHdiBufferMode DEFAULT_HDI_BUFFER_MODE = GST_HDI_BUFFER_EXTERNAL_MODE;
static const int SURFACE_MAX_TRY_COUNT = 10;
static const int HDI_MAX_TRY_COUNT = 50;
static const int RETRY_SLEEP_UTIME = 10000;

static void
gst_hdi_video_dec_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_hdi_video_dec_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec);
static gboolean gst_hdi_video_dec_open (GstVideoDecoder * decoder);
static gboolean gst_hdi_video_dec_close (GstVideoDecoder * decoder);
static gboolean gst_hdi_video_dec_start (GstVideoDecoder * decoder);
static gboolean gst_hdi_video_dec_stop (GstVideoDecoder * decoder);
static gboolean gst_hdi_video_dec_set_format (GstVideoDecoder * decoder, GstVideoCodecState * state);
static gboolean gst_hdi_video_dec_flush (GstVideoDecoder * decoder);
static GstFlowReturn gst_hdi_video_dec_handle_frame (GstVideoDecoder * decoder, GstVideoCodecFrame * frame);
static GstFlowReturn gst_hdi_video_dec_drain (GstVideoDecoder * decoder);
static void gst_hdi_video_dec_finalize (GObject * object);
static GstFlowReturn gst_hdi_video_dec_finish (GstVideoDecoder * decoder);
static void gst_hdi_video_dec_loop (GstHDIVideoDec * self);
static void gst_hdi_video_dec_pause_loop (GstHDIVideoDec * self, GstFlowReturn flow_ret);
static gboolean gst_hdi_video_dec_sink_event (GstVideoDecoder * decoder, GstEvent * event);
static void get_hdi_video_frame_from_outInfo(GstHDIVideoFrame * frame, const OutputInfo * outInfo);
static void gst_hdi_cha_tile_to_linear_8Bit(const guint8 * map, const GstHDIVideoFrame * frame);
static gboolean gst_hdi_video_dec_fill_output_buffer(
  const GstHDIVideoDec * self, const GstHDIVideoFrame * inbuf, GstBuffer * outbuf);
enum {
  PROP_0,
  PROP_SURFACE,
};

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(GstHDIVideoDec, gst_hdi_video_dec,
    GST_TYPE_VIDEO_DECODER, DEBUG_INIT);

static void
gst_hdi_video_dec_class_init(GstHDIVideoDecClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  GstVideoDecoderClass *video_decoder_class = GST_VIDEO_DECODER_CLASS(klass);
  g_return_if_fail(gobject_class != nullptr);
  g_return_if_fail(video_decoder_class != nullptr);
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
  g_object_class_install_property (gobject_class, PROP_SURFACE,
      g_param_spec_pointer ("surface", "Surface",
          "The surface which gets the buffers. ",
          (GParamFlags) (G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));
}

static void
gst_hdi_video_dec_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  g_return_if_fail (object != nullptr);
  g_return_if_fail (value != nullptr);
  GstHDIVideoDec *self = GST_HDI_VIDEO_DEC (object);
  GST_DEBUG_OBJECT(object, "set hdidec gst_hdi_video_dec_set_property");

  switch (property_id) {
    case PROP_SURFACE:
      self->surface = g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
gst_hdi_video_dec_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  g_return_if_fail (object != nullptr);
  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
gst_hdi_video_dec_init (GstHDIVideoDec * self)
{
  g_return_if_fail (self != nullptr);

  gst_video_decoder_set_packetized (GST_VIDEO_DECODER (self), TRUE);
  gst_video_decoder_set_use_default_pad_acceptcaps (GST_VIDEO_DECODER_CAST (self), TRUE);
  g_return_if_fail (GST_VIDEO_DECODER_SINK_PAD (self) != nullptr);
  GST_PAD_SET_ACCEPT_TEMPLATE (GST_VIDEO_DECODER_SINK_PAD (self));
  g_mutex_init (&self->drain_lock);
  g_cond_init (&self->drain_cond);
}

static void
gst_hdi_video_dec_finalize (GObject * object)
{
  GST_DEBUG_OBJECT(nullptr, "finalize the hdi ins");
  g_return_if_fail (object != nullptr);
  GstHDIVideoDec *self = GST_HDI_VIDEO_DEC (object);

  g_mutex_clear (&self->drain_lock);
  g_cond_clear (&self->drain_cond);
  if (G_OBJECT_CLASS (gst_hdi_video_dec_parent_class)) {
    G_OBJECT_CLASS (gst_hdi_video_dec_parent_class)->finalize (object);
  }
}

static void
gst_hdi_change_format_to_params (Param * param, const GstHdiVideoFormat * format, int32_t * actual_size)
{
  int32_t index = 0;
  param[index].key = KEY_MIMETYPE;
  param[index].val = (void*)&(format->mime);
  param[index].size = sizeof(format->mime);
  index++;
  param[index].key = KEY_WIDTH;
  param[index].val = (void*)&(format->width);
  param[index].size = sizeof(format->width);
  index++;
  param[index].key = KEY_HEIGHT;
  param[index].val = (void*)&(format->height);
  param[index].size = sizeof(format->height);
  index++;
  param[index].key = KEY_BUFFERSIZE;
  param[index].val = (void*)&(format->buffer_size);
  param[index].size = sizeof(format->buffer_size);
  index++;
  param[index].key = KEY_CODEC_TYPE;
  param[index].val = (void*)&(format->codec_type);
  param[index].size = sizeof(format->codec_type);
  index++;
  param[index].key = KEY_PIXEL_FORMAT;
  param[index].val = (void*)&(format->pixel_format);
  param[index].size = sizeof(format->pixel_format);
  index++;
  *actual_size = index;
}

static int32_t
gst_hdi_set_dec_meta(GstHDICodec * codec, GstHdiVideoFormat * format)
{
  g_return_val_if_fail (codec != nullptr, HDI_FAILURE);
  g_return_val_if_fail (format != nullptr, HDI_FAILURE);
  Param param[HDI_INTERFACE_PARAM_MAX_NUM] = {};
  int max_num = HDI_INTERFACE_PARAM_MAX_NUM;
  int32_t actual_size = 0;
  if (memset_s(param, max_num * sizeof(Param), 0x00, max_num * sizeof(Param)) != EOK) {
    GST_ERROR_OBJECT(codec, "memset_s failed");
  }
  gst_hdi_change_format_to_params(param, format, &actual_size);
  int32_t ret = gst_hdi_codec_set_meta(codec, param, actual_size);
  return ret;
}

static GstHDICodec *
gst_hdi_video_dec_new (const gchar * codec_name, const GstHdiVideoFormat * format)
{
  g_return_val_if_fail (codec_name != nullptr, nullptr);
  g_return_val_if_fail (format != nullptr, nullptr);
  Param param[HDI_INTERFACE_PARAM_MAX_NUM] = {};
  int max_num = HDI_INTERFACE_PARAM_MAX_NUM;
  int32_t actual_size = 0;
  if (memset_s(param, max_num * sizeof(Param), 0x00, max_num * sizeof(Param)) != EOK) {
    GST_ERROR_OBJECT(nullptr, "memset_s failed");
  };
  gst_hdi_change_format_to_params(param, format, &actual_size);
  return gst_hdi_codec_new(codec_name, param, actual_size);
}

static gboolean
gst_hdi_video_dec_open (GstVideoDecoder * decoder)
{
  g_return_val_if_fail (decoder != nullptr, FALSE);
  GstHDIVideoDec *self = GST_HDI_VIDEO_DEC(decoder);
  GstHDIVideoDecClass *klass = GST_HDI_VIDEO_DEC_GET_CLASS(self);
  GstHdiVideoFormat format;
  format.mime = klass->cdata.mime;
  format.width = DEFAULT_HDI_WIDTH;
  format.height = DEFAULT_HDI_HEIGHT;
  format.codec_type = klass->cdata.codec_type;
  format.buffer_size = DEFAULT_HDI_BUFFER_SIZE;
  format.pixel_format = DEFAULT_HDI_PIXEL_FORMAT;
  GST_DEBUG_OBJECT(self, "Opening decoder");

  self->dec = gst_hdi_video_dec_new(klass->cdata.codec_name, &format);

  if (!self->dec) {
    GST_ERROR_OBJECT(self, "Opening decoder failed");
    return FALSE;
  }

  return TRUE;
}

static gboolean
gst_hdi_video_dec_close (GstVideoDecoder * decoder)
{
  g_return_val_if_fail (decoder != nullptr, FALSE);
  GstHDIVideoDec *self = GST_HDI_VIDEO_DEC(decoder);

  GST_DEBUG_OBJECT (self, "Closing decoder");

  if (self->dec) {
    gst_hdi_codec_unref(self->dec);
  }
  self->dec = nullptr;

  self->started = FALSE;

  return TRUE;
}

static gboolean
gst_hdi_video_dec_start (GstVideoDecoder * decoder)
{
  g_return_val_if_fail (decoder != nullptr, FALSE);
  GstHDIVideoDec *self = GST_HDI_VIDEO_DEC(decoder);
  GST_DEBUG_OBJECT(self, "dec start");
  self->started = FALSE;
  g_mutex_lock (&self->drain_lock);
  self->draining = FALSE;
  g_cond_broadcast (&self->drain_cond);
  g_mutex_unlock (&self->drain_lock);
  self->last_upstream_ts = 0;
  self->downstream_flow_ret = GST_FLOW_OK;
  self->useBuffers = FALSE;

  return TRUE;
}

static gboolean
gst_hdi_video_dec_stop (GstVideoDecoder * decoder)
{
  g_return_val_if_fail (decoder != nullptr, FALSE);
  GstHDIVideoDec *self = GST_HDI_VIDEO_DEC(decoder);

  GST_DEBUG_OBJECT (self, "Stopping decoder");

  gst_pad_stop_task (GST_VIDEO_DECODER_SRC_PAD (decoder));

  self->downstream_flow_ret = GST_FLOW_FLUSHING;
  self->started = FALSE;

  g_mutex_lock (&self->drain_lock);
  self->draining = FALSE;
  g_cond_broadcast (&self->drain_cond);
  g_mutex_unlock (&self->drain_lock);

  int32_t ret = gst_hdi_codec_stop(self->dec);
  if (ret != HDI_SUCCESS) {
    GST_ERROR_OBJECT (self, "decoder stop failed %d", ret);
  }
  if (self->input_state) {
    gst_video_codec_state_unref (self->input_state);
  }
  self->input_state = nullptr;
  GST_DEBUG_OBJECT (self, "Stopped decoder");

  return TRUE;
}

static gboolean
gst_hdi_video_dec_flush (GstVideoDecoder * decoder)
{
  g_return_val_if_fail (decoder != nullptr, FALSE);
  GstHDIVideoDec *self = GST_HDI_VIDEO_DEC (decoder);

  GST_DEBUG_OBJECT (self, "Flushing decoder");

  GST_VIDEO_DECODER_STREAM_UNLOCK (self);
  gst_pad_stop_task (GST_VIDEO_DECODER_SRC_PAD (decoder));
  GST_VIDEO_DECODER_STREAM_LOCK (self);

  int32_t ret = gst_hdi_port_flush (self->dec, ALL_TYPE);
  if (ret != HDI_SUCCESS) {
    GST_DEBUG_OBJECT (self, "Failed to populate output port: %d", ret);
  }

  self->last_upstream_ts = 0;
  self->downstream_flow_ret = GST_FLOW_OK;
  self->started = FALSE;
  GST_DEBUG_OBJECT (self, "Flush finished");

  return TRUE;
}

static GstHdiBufferMode
gst_hdi_video_dec_pick_input_buffer_mode (const GstHDIVideoDec * self)
{
  return DEFAULT_HDI_BUFFER_MODE;
}

static gboolean
gst_hdi_video_dec_negotiate (const GstHDIVideoDec * self)
{
  g_return_val_if_fail (self != nullptr, FALSE);
  g_return_val_if_fail (GST_VIDEO_DECODER_SRC_PAD (self) != nullptr, FALSE);
  GstCaps *templ_caps = nullptr;
  GstCaps *intersection = nullptr;
  GstVideoFormat format;
  const gchar *format_str = nullptr;

  GST_DEBUG_OBJECT (self, "Trying to negotiate a video format with downstream");

  templ_caps = gst_pad_get_pad_template_caps (GST_VIDEO_DECODER_SRC_PAD (self));
  intersection =
      gst_pad_peer_query_caps (GST_VIDEO_DECODER_SRC_PAD (self), templ_caps);
  gst_caps_unref (templ_caps);

  GST_DEBUG_OBJECT (self, "Allowed downstream caps");

  if (gst_caps_is_empty (intersection)) {
      gst_caps_unref (intersection);
      GST_ERROR_OBJECT (self, "Empty caps");
      return FALSE;
  }

  intersection = gst_caps_truncate (intersection);
  intersection = gst_caps_fixate (intersection);

  GstStructure * s = gst_caps_get_structure (intersection, 0);
  format_str = gst_structure_get_string (s, "format");
  if (format_str == nullptr ||
      (format =
          gst_video_format_from_string (format_str)) ==
      GST_VIDEO_FORMAT_UNKNOWN) {
      GST_ERROR_OBJECT (self, "Invalid caps");
      gst_caps_unref (intersection);
      return FALSE;
  }

  gst_caps_unref (intersection);
  return TRUE;
}

static int32_t
gst_dec_queue_input_buffer (const GstHDIVideoDec *self, InputInfo * inputData)
{
  gboolean done = FALSE;
  int32_t ret = HDI_SUCCESS;
  while (!done) {
    done = TRUE;
    ret = queue_input_buffer(self->dec, inputData, GET_BUFFER_TIMEOUT_MS);
    if (ret == HDI_ERR_STREAM_BUF_FULL) {
      done = FALSE;
    }
  }
  return ret;
}

static gboolean
gst_hdi_video_dec_enable (GstHDIVideoDec * self)
{
  g_return_val_if_fail (self != nullptr, FALSE);
  g_return_val_if_fail (self->dec != nullptr, FALSE);
  GST_DEBUG_OBJECT (self, "Enabling codec");

  self->inputBufferMode = gst_hdi_video_dec_pick_input_buffer_mode(self);

  if (!gst_hdi_video_dec_negotiate(self))
      GST_LOG_OBJECT (self, "Negotiation failed, will get output format later");

  if (gst_hdi_codec_start(self->dec) != HDI_SUCCESS) {
      return FALSE;
  }
  if (gst_hdi_port_flush(self->dec, ALL_TYPE) != HDI_SUCCESS) {
      GST_ERROR_OBJECT(self, "flush err");
  }
  return TRUE;
}

static GstFlowReturn
gst_hdi_video_dec_queue_input_buffer (GstHDIVideoDec *self, const GstVideoCodecFrame * frame)
{
  g_return_val_if_fail (self != nullptr, GST_FLOW_ERROR);
  g_return_val_if_fail (frame != nullptr, GST_FLOW_ERROR);
  GST_DEBUG_OBJECT(nullptr, "queue input buffer");
  gboolean done = FALSE;
  int32_t ret = HDI_SUCCESS;
  InputInfo inputData;
  CodecBufferInfo inBufInfo;
  inputData.bufferCnt = 1;
  inputData.buffers = &inBufInfo;

  GstClockTime timestamp;

  timestamp = frame->pts;
  GST_DEBUG_OBJECT (self, "PTS %" GST_TIME_FORMAT ", DTS %" GST_TIME_FORMAT
      ", dist %d", GST_TIME_ARGS (frame->pts), GST_TIME_ARGS (frame->dts),
      frame->distance_from_sync);
  if (memset_s(&inBufInfo, sizeof(inBufInfo), 0, sizeof(CodecBufferInfo)) != EOK) {
    GST_ERROR_OBJECT (self, "memset failed");
  }
  while (!done) {
    done = TRUE;
    ret = deque_input_buffer(self->dec, &inputData, GET_BUFFER_TIMEOUT_MS);
    if (ret != HDI_SUCCESS) {
      done = FALSE;
      GST_DEBUG_OBJECT (self, "deque_input_buffer failed and continue");
      continue;
    }

    if (!gst_hdi_buffer_map_buffer(&inBufInfo, frame->input_buffer)) {
      GST_ERROR_OBJECT (self, "map input buffer failed");
    }
    inputData.flag = STREAM_FLAG_KEYFRAME;

    if (timestamp != GST_CLOCK_TIME_NONE) {
      inputData.pts = (int64_t)gst_util_uint64_scale (timestamp, HDI_USECOND_PER_SECOND, GST_SECOND);
      self->last_upstream_ts = timestamp;
    } else {
      inputData.pts =  G_GUINT64_CONSTANT (0);
    }
    ret = gst_dec_queue_input_buffer (self, &inputData);
    if (ret != HDI_SUCCESS) {
      GST_DEBUG_OBJECT (self, "queue_input_buffer failed and continue");
    }
  }
  return ret == HDI_SUCCESS ? GST_FLOW_OK : GST_FLOW_ERROR;
}

static GstFlowReturn
gst_hdi_video_dec_handle_frame (GstVideoDecoder * decoder, GstVideoCodecFrame * frame)
{
  g_return_val_if_fail (decoder != nullptr, GST_FLOW_ERROR);
  g_return_val_if_fail (frame != nullptr, GST_FLOW_ERROR);
  GstHDIVideoDec *self = GST_HDI_VIDEO_DEC (decoder);
  gboolean flushing = FALSE;
  GST_DEBUG_OBJECT (self, "Handling frame");
  if (self->downstream_flow_ret != GST_FLOW_OK) {
    gst_video_codec_frame_unref (frame);
    return self->downstream_flow_ret;
  }
  g_mutex_lock (&self->dec->lock);
  flushing = self->dec->hdi_flushing;
  g_mutex_unlock (&self->dec->lock);
  if (flushing) {
    gst_video_codec_frame_unref (frame);
    return GST_FLOW_FLUSHING;
  }
  if (!self->started) {
    if (!GST_VIDEO_CODEC_FRAME_IS_SYNC_POINT(frame)) {
        gst_video_decoder_drop_frame (GST_VIDEO_DECODER (self), frame);
        return GST_FLOW_OK;
    }

    if (!gst_hdi_video_dec_enable(self)) {
        GST_DEBUG_OBJECT (self, "hdi video dec enable failed");
        gst_video_codec_frame_unref (frame);
        return GST_FLOW_ERROR;
    }
    self->started = TRUE;

    GST_DEBUG_OBJECT (self, "Starting task");
    gst_pad_start_task (GST_VIDEO_DECODER_SRC_PAD (self),
        (GstTaskFunction) gst_hdi_video_dec_loop, decoder, nullptr);
  }
  GST_VIDEO_DECODER_STREAM_UNLOCK (self);
  GstFlowReturn ret = gst_hdi_video_dec_queue_input_buffer (self, frame);
  GST_VIDEO_DECODER_STREAM_LOCK (self);

  gst_video_codec_frame_unref (frame);

  GST_DEBUG_OBJECT (self, "Passed frame to component");

  return ret;
}

static void
gst_hdi_video_dec_loop_invalid_buffer_err (GstHDIVideoDec * self)
{
  g_return_if_fail (self != nullptr);
  GST_ELEMENT_ERROR (self, LIBRARY, SETTINGS, (nullptr),
      ("Invalid output buffer"));
  gst_pad_push_event (GST_VIDEO_DECODER_SRC_PAD (self), gst_event_new_eos ());
  gst_hdi_video_dec_pause_loop (self, GST_FLOW_ERROR);
  return;
}

static void
gst_hdi_video_dec_loop_flow_err (GstHDIVideoDec * self, GstFlowReturn flow_ret)
{
  g_return_if_fail (self != nullptr);
  GST_DEBUG_OBJECT(nullptr, "loop flow err");
  if (flow_ret == GST_FLOW_EOS) {
    GST_DEBUG_OBJECT (self, "EOS");

    gst_pad_push_event (GST_VIDEO_DECODER_SRC_PAD (self),
        gst_event_new_eos ());
  } else if (flow_ret < GST_FLOW_EOS) {
    GST_ELEMENT_ERROR (self, STREAM, FAILED,
        ("Internal data stream error."), ("stream stopped, reason %s",
            gst_flow_get_name (flow_ret)));

    gst_pad_push_event (GST_VIDEO_DECODER_SRC_PAD (self),
        gst_event_new_eos ());
  } else if (flow_ret == GST_FLOW_FLUSHING) {
    GST_DEBUG_OBJECT (self, "Flushing -- stopping task");
  }
  gst_hdi_video_dec_pause_loop (self, flow_ret);
  return;
}

static void
gst_hdi_video_dec_loop_hdi_eos (GstHDIVideoDec * self)
{
  g_return_if_fail (self != nullptr);
  g_return_if_fail (GST_VIDEO_DECODER_SRC_PAD (self) != nullptr);
  GST_DEBUG_OBJECT(nullptr, "loop hdi eos");
  GstFlowReturn flow_ret = GST_FLOW_OK;
  g_mutex_lock (&self->drain_lock);
  if (self->draining) {
    GstQuery *query = gst_query_new_drain ();

    /* Drain the pipeline to reclaim all memories back to the pool */
    if (!gst_pad_peer_query (GST_VIDEO_DECODER_SRC_PAD (self), query))
      GST_DEBUG_OBJECT (self, "drain query failed");
    gst_query_unref (query);

    GST_DEBUG_OBJECT (self, "Drained");
    self->draining = FALSE;
    g_cond_broadcast (&self->drain_cond);
    flow_ret = GST_FLOW_OK;
    gst_pad_pause_task (GST_VIDEO_DECODER_SRC_PAD (self));
  } else {
    GST_DEBUG_OBJECT (self, "codec signalled EOS");
    flow_ret = GST_FLOW_EOS;
  }
  g_mutex_unlock (&self->drain_lock);

  GST_VIDEO_DECODER_STREAM_LOCK (self);
  self->downstream_flow_ret = flow_ret;
  GST_VIDEO_DECODER_STREAM_UNLOCK (self);

  /* Here we fallback and pause the task for the EOS case */
  if (flow_ret != GST_FLOW_OK)
    gst_hdi_video_dec_loop_flow_err (self, flow_ret);

  return;
}

static void
gst_hdi_video_dec_loop_hdi_error (GstHDIVideoDec * self, int32_t ret)
{
  g_return_if_fail (self != nullptr);
  GST_ELEMENT_ERROR (self, LIBRARY, FAILED, (nullptr),
      ("decoder hdi in error state %s (%d)",
          gst_hdi_error_to_string(ret),
          ret));
  gst_pad_push_event (GST_VIDEO_DECODER_SRC_PAD (self), gst_event_new_eos ());
  gst_hdi_video_dec_pause_loop (self, GST_FLOW_ERROR);
  return;
}

static void
gst_hdi_video_dec_loop_surface_error (GstHDIVideoDec * self)
{
  g_return_if_fail (self != nullptr);
  GST_ELEMENT_ERROR (self, LIBRARY, FAILED, (nullptr),
      ("decoder surface in error"));
  gst_pad_push_event (GST_VIDEO_DECODER_SRC_PAD (self), gst_event_new_eos ());
  gst_hdi_video_dec_pause_loop (self, GST_FLOW_ERROR);
  return;
}

static void
update_video_meta (const GstHDIVideoFrame * out_frame, GstBuffer * outbuf)
{
  g_return_if_fail (out_frame != nullptr);
  g_return_if_fail (outbuf != nullptr);
  GST_DEBUG_OBJECT(nullptr, "update video meta");
  GstVideoMeta *video_meta = gst_buffer_get_video_meta (outbuf);
  if (video_meta == nullptr) {
    gst_buffer_add_video_meta (
        outbuf, GST_VIDEO_FRAME_FLAG_NONE, GST_VIDEO_FORMAT_NV21, out_frame->width, out_frame->height);
  } else {
    video_meta->width = out_frame->width;
    video_meta->height = out_frame->height;
  }
}

static int32_t
gst_hdi_get_out_buffer (const GstHDIVideoDec * self, OutputInfo * outInfo)
{
  g_return_val_if_fail (self != nullptr, HDI_FAILURE);
  g_return_val_if_fail (outInfo != nullptr, HDI_FAILURE);
  g_return_val_if_fail (self->dec != nullptr, HDI_FAILURE);
  int32_t ret = HDI_SUCCESS;
  gboolean done = FALSE;
  while (!done) {
    ret = deque_output_buffer(self->dec, outInfo, GET_BUFFER_TIMEOUT_MS);
    if (ret == HDI_ERR_FRAME_BUF_EMPTY) {
      GST_DEBUG_OBJECT(self, "hdi output buffer empty");
      usleep(RETRY_SLEEP_UTIME);
      continue;
    }
    done = TRUE;
  }
  return ret;
}

static int32_t
gst_hdi_deal_with_out_buffer (
  GstHDIVideoDec * self, OutputInfo * outInfo, const GstHDIVideoFrame * out_frame, GstVideoCodecFrame * frame)
{
  g_return_val_if_fail (self != nullptr, HDI_FAILURE);
  g_return_val_if_fail (outInfo != nullptr, HDI_FAILURE);
  g_return_val_if_fail (out_frame != nullptr, HDI_FAILURE);
  int32_t ret = HDI_SUCCESS;
  GstFlowReturn flow_ret = GST_FLOW_OK;
  if (frame == nullptr && outInfo->bufferCnt != 0) {
    GstBuffer *outbuf = nullptr;
    GST_ERROR_OBJECT (self, "No corresponding frame found");
    outbuf = gst_video_decoder_allocate_output_buffer (GST_VIDEO_DECODER (self));
    if (gst_hdi_video_dec_fill_output_buffer (self, out_frame, outbuf) != TRUE) {
      gst_buffer_unref (outbuf);
      if (ret != HDI_SUCCESS) {
        GST_ERROR_OBJECT(self, "QueueOutput failed %d", ret);
      }
      gst_hdi_video_dec_loop_invalid_buffer_err(self);
      return ret;
    }
    flow_ret = gst_pad_push (GST_VIDEO_DECODER_SRC_PAD (self), outbuf);
  } else if (outInfo->bufferCnt != 0) {
    flow_ret = gst_video_decoder_allocate_output_frame (GST_VIDEO_DECODER (self), frame);
    g_return_val_if_fail (frame->output_buffer != nullptr, HDI_FAILURE);
    if (gst_hdi_video_dec_fill_output_buffer (self, out_frame, frame->output_buffer) != TRUE) {
      gst_buffer_replace (&frame->output_buffer, nullptr);
      flow_ret =
        gst_video_decoder_drop_frame (GST_VIDEO_DECODER (self), frame);
      frame = nullptr;
      if (ret != HDI_SUCCESS) {
        GST_ERROR_OBJECT(self, "QueueOutput failed %d", ret);
      }
      gst_hdi_video_dec_loop_invalid_buffer_err(self);
      return ret;
    }

    flow_ret =
        gst_video_decoder_finish_frame (GST_VIDEO_DECODER (self), frame);
    frame = nullptr;
  } else if (frame != nullptr) {
    flow_ret = GST_FLOW_OK;
    gst_video_codec_frame_unref (frame);
    frame = nullptr;
  }
  return ret;
}

static gboolean
gst_hdi_video_dec_fill_surface_buffer(GstHDIVideoDec * self, const GstHDIVideoFrame * inbuf, GstBuffer * outbuf)
{
  g_return_val_if_fail (self != nullptr, FALSE);
  g_return_val_if_fail (inbuf != nullptr, FALSE);
  g_return_val_if_fail (outbuf != nullptr, FALSE);
  gst_hdi_cha_tile_to_linear_8Bit(GetVirBuffer(outbuf), inbuf);

  GST_BUFFER_PTS (outbuf) = gst_util_uint64_scale (inbuf->pts, GST_SECOND, HDI_USECOND_PER_SECOND);

  return TRUE;
}

static int32_t
gst_hdi_finish_surface_buffer (GstHDIVideoDec * self, OutputInfo * outInfo,
    GstHDIVideoFrame * out_frame, GstVideoCodecFrame * frame, GstBuffer * outbuf)
{
  int32_t ret = HDI_SUCCESS;
  GstFlowReturn flow_ret = GST_FLOW_OK;
  if (frame == nullptr && outInfo->bufferCnt != 0) {
    GST_DEBUG_OBJECT (self, "No corresponding frame found");
    if (!gst_hdi_video_dec_fill_surface_buffer (self, out_frame, outbuf)) {
      GST_ERROR_OBJECT (self, "fill surface buffer error");
      gst_buffer_unref (outbuf);
      gst_hdi_video_dec_loop_invalid_buffer_err(self);
      return ret;
    }
    flow_ret = gst_pad_push (GST_VIDEO_DECODER_SRC_PAD (self), outbuf);
  } else if (outInfo->bufferCnt != 0) {
    if (!gst_hdi_video_dec_fill_surface_buffer (self, out_frame, outbuf)) {
      GST_ERROR_OBJECT (self, "fill surface buffer error");
      gst_buffer_unref (outbuf);
      flow_ret =
        gst_video_decoder_drop_frame (GST_VIDEO_DECODER (self), frame);
      frame = nullptr;
      gst_hdi_video_dec_loop_invalid_buffer_err(self);
      return ret;
    }

    frame->output_buffer = outbuf;
    update_video_meta (out_frame, outbuf);

    flow_ret =
        gst_video_decoder_finish_frame (GST_VIDEO_DECODER (self), frame);
    frame = nullptr;
  } else if (frame != nullptr) {
    flow_ret = GST_FLOW_OK;
    gst_video_codec_frame_unref (frame);
    frame = nullptr;
  }
  GST_DEBUG_OBJECT (self, "Finished frame: %s", gst_flow_get_name (flow_ret));
  GST_VIDEO_DECODER_STREAM_LOCK (self);
  self->downstream_flow_ret = flow_ret;
  GST_VIDEO_DECODER_STREAM_UNLOCK (self);
  if (flow_ret != GST_FLOW_OK) {
    gst_hdi_video_dec_loop_flow_err(self, flow_ret);
  }
  return ret;
}

static int32_t
gst_hdi_deal_with_surface_buffer (GstHDIVideoDec * self, OutputInfo * outInfo,
    GstHDIVideoFrame * out_frame, GstVideoCodecFrame * frame)
{
  GstBuffer * outbuf = GetSurfaceBuffer(self->surface, out_frame->buffer_size, out_frame->width, out_frame->height);
  int32_t surface_try_count = 0;
  while (outbuf == nullptr) {
    GST_DEBUG_OBJECT (self, "retry to get surface retry count %d", surface_try_count);
    if (surface_try_count >= SURFACE_MAX_TRY_COUNT) {
      GST_ERROR_OBJECT (self, "try get surface %d times and can not get surface", surface_try_count);
      gst_hdi_video_dec_loop_surface_error(self);
      return HDI_FAILURE;
    }
    usleep(RETRY_SLEEP_UTIME);
    surface_try_count++;
    outbuf = GetSurfaceBuffer(self->surface, out_frame->buffer_size, out_frame->width, out_frame->height);
  }

  return gst_hdi_finish_surface_buffer(self, outInfo, out_frame, frame, outbuf);
}

static void
gst_hdi_init_output_buffer_before_loop (OutputInfo * outInfo, CodecBufferInfo * outBuf)
{
  g_return_if_fail (outInfo != nullptr);
  g_return_if_fail (outBuf != nullptr);
  if (memset_s(outInfo, sizeof(OutputInfo), 0, sizeof(OutputInfo)) != EOK) {
      GST_ERROR_OBJECT (nullptr, "memset failed");
  }
  if (memset_s(outBuf, sizeof(CodecBufferInfo), 0, sizeof(CodecBufferInfo)) != EOK) {
      GST_ERROR_OBJECT (nullptr, "memset failed");
  }
  outInfo->bufferCnt = 1;
  outInfo->buffers = outBuf;
}

static void
gst_hdi_video_dec_loop (GstHDIVideoDec * self)
{
  g_return_if_fail (self != nullptr);
  OutputInfo outInfo;
  CodecBufferInfo outBuf;
  gst_hdi_init_output_buffer_before_loop(&outInfo, &outBuf);
  int32_t ret = HDI_SUCCESS;
  GstHDIVideoFrame out_frame;
  ret = gst_hdi_get_out_buffer (self, &outInfo);
  if (ret == HDI_RECEIVE_EOS) {
    gst_hdi_video_dec_loop_hdi_eos(self);
    return;
  }
  if (ret != HDI_SUCCESS) {
    gst_hdi_video_dec_loop_hdi_error(self, ret);
    return;
  }
  get_hdi_video_frame_from_outInfo(&out_frame, &outInfo);
  if (!gst_pad_has_current_caps (GST_VIDEO_DECODER_SRC_PAD (self))) {
    GstVideoFormat format = gst_hdi_video_get_format_from_hdi (out_frame.pixel_format);
    GstVideoInterlaceMode interlace_mode = GST_VIDEO_INTERLACE_MODE_PROGRESSIVE;
    GstVideoCodecState * state =
      gst_video_decoder_set_interlaced_output_state (
          GST_VIDEO_DECODER (self), format, interlace_mode, out_frame.width, out_frame.height, self->input_state);

    gst_video_decoder_negotiate (GST_VIDEO_DECODER (self));

    gst_video_codec_state_unref (state);
  }

  GstVideoCodecFrame * frame = gst_hdi_video_find_nearest_frame(GST_ELEMENT_CAST (self), &outInfo,
      gst_video_decoder_get_frames (GST_VIDEO_DECODER (self)));
  if (self->surface != nullptr) {
    GST_DEBUG_OBJECT (self, "deal with surface.");
    ret = gst_hdi_deal_with_surface_buffer(self, &outInfo, &out_frame, frame);
  } else {
    ret = gst_hdi_deal_with_out_buffer (self, &outInfo, &out_frame, frame);
  }
  if (ret != HDI_SUCCESS) {
    GST_ERROR_OBJECT (self, "deal buffer error: %s", gst_hdi_error_to_string (ret));
  }
  g_return_if_fail (self->dec != nullptr);
  ret = queue_output_buffer (self->dec, &outInfo, GET_BUFFER_TIMEOUT_MS);
  if (ret != HDI_SUCCESS) {
    gst_hdi_video_dec_loop_hdi_error(self, ret);
    return;
  }
}

static void
gst_hdi_video_dec_pause_loop (GstHDIVideoDec * self, GstFlowReturn flow_ret)
{
  g_return_if_fail (self != nullptr);
  g_mutex_lock (&self->drain_lock);
  if (self->draining) {
    self->draining = FALSE;
    g_cond_broadcast (&self->drain_cond);
  }
  GST_DEBUG_OBJECT (self, "pause loop.");
  gst_pad_pause_task (GST_VIDEO_DECODER_SRC_PAD (self));
  self->downstream_flow_ret = flow_ret;
  self->started = FALSE;
  g_mutex_unlock (&self->drain_lock);
}

static gboolean
gst_hdi_video_dec_fill_output_buffer(const GstHDIVideoDec * self, const GstHDIVideoFrame * inbuf, GstBuffer * outbuf)
{
  g_return_val_if_fail (self != nullptr, FALSE);
  g_return_val_if_fail (inbuf != nullptr, FALSE);
  g_return_val_if_fail (outbuf != nullptr, FALSE);
  GstVideoCodecState *state = gst_video_decoder_get_output_state (GST_VIDEO_DECODER (self));

  if (gst_buffer_get_size (outbuf) == inbuf->buffer_size) {
    GstMapInfo map = GST_MAP_INFO_INIT;

    if (!gst_buffer_map (outbuf, &map, GST_MAP_WRITE)) {
      GST_ERROR_OBJECT (self, "Failed to map output buffer");
      gst_video_codec_state_unref (state);
      return FALSE;
    }
    gst_hdi_cha_tile_to_linear_8Bit(map.data, inbuf);
    gst_buffer_unmap (outbuf, &map);
    GST_BUFFER_PTS (outbuf) = gst_util_uint64_scale ((guint64)inbuf->pts, GST_SECOND, HDI_USECOND_PER_SECOND);
    return TRUE;
  }
  return FALSE;
}

static gboolean
gst_hdi_video_dec_set_format (GstVideoDecoder * decoder,
    GstVideoCodecState * state)
{
  g_return_val_if_fail (decoder != nullptr, FALSE);
  g_return_val_if_fail (state != nullptr, FALSE);
  GstHDIVideoDec *self;
  GstHDIVideoDecClass *klass;
  GstVideoInfo *info = &state->info;
  gboolean is_format_change = FALSE;
  int32_t ret = HDI_SUCCESS;

  self = GST_HDI_VIDEO_DEC (decoder);
  klass = GST_HDI_VIDEO_DEC_GET_CLASS (decoder);

  GST_DEBUG_OBJECT (self, "Setting new caps");

  is_format_change = is_format_change || self->hdi_video_in_format.width != info->width;
  is_format_change = is_format_change ||
      self->hdi_video_in_format.height != GST_VIDEO_INFO_FIELD_HEIGHT (info);
  is_format_change = is_format_change || (self->hdi_video_in_format.frame_rate == 0
      && info->fps_n != 0);

  if (is_format_change) {
    self->hdi_video_in_format.width = info->width;
    self->hdi_video_in_format.height = GST_VIDEO_INFO_FIELD_HEIGHT (info);
    self->hdi_video_in_format.frame_rate  = info->fps_n;
    self->hdi_video_in_format.width = info->width;
    self->hdi_video_in_format.buffer_size = 0;
  }

  GST_DEBUG_OBJECT (self, "Setting inport port definition");
  ret = gst_hdi_set_dec_meta(self->dec, &self->hdi_video_in_format);
  if (ret != HDI_SUCCESS) {
    GST_DEBUG_OBJECT (self, "Setting definition failed %d", ret);
  }
  self->input_state = gst_video_codec_state_ref (state);
  self->downstream_flow_ret = GST_FLOW_OK;
  return TRUE;
}

static void
gst_init_eos_input_before_deque(InputInfo * inputData, CodecBufferInfo * inBufInfo)
{
  inputData->bufferCnt = 0;
  inputData->buffers = inBufInfo;
  inputData->pts = 0;
  inputData->flag = 0;
}

static void
gst_init_eos_input_before_queue(const GstHDIVideoDec * self, InputInfo * inputData)
{
  inputData->bufferCnt = 1;
  inputData->buffers[0].addr = nullptr;
  inputData->buffers[0].length = 0;
  inputData->flag = STREAM_FLAG_END_OF_FRAME;
  inputData->pts = (int64_t)gst_util_uint64_scale (self->last_upstream_ts, HDI_USECOND_PER_SECOND, GST_SECOND);
}

static int32_t
gst_hdi_dec_deque_input_buffer(const GstHDIVideoDec *self, InputInfo * inputData)
{
  int32_t try_count = 0;
  gboolean done = FALSE;
  int32_t ret = HDI_SUCCESS;
  while (!done && try_count < HDI_MAX_TRY_COUNT) {
    done = TRUE;
    ret = deque_input_buffer(self->dec, inputData, GET_BUFFER_TIMEOUT_MS);
    if (ret != HDI_SUCCESS) {
      done = FALSE;
      GST_DEBUG_OBJECT (self, "deque_input_buffer failed and continue");
      continue;
    }
  }
  if (try_count == HDI_MAX_TRY_COUNT) {
    GST_ERROR_OBJECT (self, "try %d times, cannot get input buffer", try_count);
  }
  return ret;
}

static GstFlowReturn
gst_hdi_video_dec_finish (GstVideoDecoder * decoder)
{
  g_return_val_if_fail (decoder != nullptr, GST_FLOW_ERROR);
  GstHDIVideoDec *self;
  InputInfo inputData;
  CodecBufferInfo inBufInfo;
  int32_t ret = HDI_SUCCESS;

  self = GST_HDI_VIDEO_DEC (decoder);
  GST_DEBUG_OBJECT (self, "finish codec");

  if (!self->started) {
    GST_DEBUG_OBJECT (self, "Component not started yet");
    return GST_FLOW_OK;
  }
  self->started = FALSE;
  if (memset_s(&inBufInfo, sizeof(inBufInfo), 0, sizeof(CodecBufferInfo)) != EOK) {
      GST_ERROR_OBJECT (self, "memset failed");
  }
  gst_init_eos_input_before_deque(&inputData, &inBufInfo);
  GST_VIDEO_DECODER_STREAM_UNLOCK (self);
  ret = gst_hdi_dec_deque_input_buffer(self, &inputData);
  if (ret != HDI_SUCCESS) {
    GST_VIDEO_DECODER_STREAM_LOCK (self);
    GST_ERROR_OBJECT (self, "Failed to deque input buffer for draining: %d", ret);
    return GST_FLOW_ERROR;
  }
  g_mutex_lock (&self->drain_lock);
  self->draining = TRUE;
  gst_init_eos_input_before_queue(self, &inputData);
  ret = queue_input_buffer(self->dec, &inputData, GET_BUFFER_TIMEOUT_MS);
  if (ret != HDI_SUCCESS) {
    GST_ERROR_OBJECT (self, "Failed to queue input buffer for draining: %d", ret);
    GST_VIDEO_DECODER_STREAM_LOCK (self);
    return GST_FLOW_ERROR;
  }
  GST_DEBUG_OBJECT (self, "Waiting until codec is drained");
  gint64 wait_until = g_get_monotonic_time () + G_TIME_SPAN_SECOND;
  if (!g_cond_wait_until (&self->drain_cond, &self->drain_lock, wait_until)) {
    GST_ERROR_OBJECT (self, "Drain timed out");
  } else {
    GST_DEBUG_OBJECT (self, "finish hdi end");
  }

  g_mutex_unlock (&self->drain_lock);
  GST_VIDEO_DECODER_STREAM_LOCK (self);
  self->started = FALSE;

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_hdi_video_dec_drain (GstVideoDecoder * decoder)
{
  GST_DEBUG_OBJECT (decoder, "Draining codec");
  g_return_val_if_fail (decoder != nullptr, GST_FLOW_ERROR);
  GstFlowReturn ret = gst_hdi_video_dec_finish (decoder);
  gst_hdi_video_dec_flush (decoder);
  GST_DEBUG_OBJECT (decoder, "Draining codec finish");
  return ret;
}

static void
gst_hdi_codec_flush_start (GstHDICodec *codec)
{
  g_return_if_fail (codec != nullptr);
  g_mutex_lock (&codec->lock);
  codec->hdi_flushing = TRUE;
  g_mutex_unlock (&codec->lock);

  return;
}

static gboolean
gst_hdi_video_dec_sink_event (GstVideoDecoder * decoder, GstEvent * event)
{
  g_return_val_if_fail (decoder != nullptr, FALSE);
  g_return_val_if_fail (event != nullptr, FALSE);
  GstHDIVideoDec *self = GST_HDI_VIDEO_DEC (decoder);
  g_return_val_if_fail(GST_VIDEO_DECODER_CLASS (gst_hdi_video_dec_parent_class) != nullptr, FALSE);
  GST_DEBUG_OBJECT (self, "gst_hdi_video_dec_sink_event,type=%#x", GST_EVENT_TYPE (event));

  GstEventType ev_type = GST_EVENT_TYPE (event);

  switch (ev_type) {
    case GST_EVENT_FLUSH_START:
      gst_hdi_codec_flush_start (self->dec);
      break;
    default:
      break;
  }

  gboolean ret = GST_VIDEO_DECODER_CLASS (gst_hdi_video_dec_parent_class)->sink_event (decoder, event);

  return ret;
}

static uint32_t
gst_get_buffer_size(uint32_t width, uint32_t height, uint32_t PixelFormat)
{
  switch (PixelFormat) {
    case YVU_SEMIPLANAR_420:
      // nv21 is like YYYYVU for every 4 BYTE
      return (width * height * 3) >> 1;
    default:
      GST_ERROR_OBJECT (nullptr, "the type of buffer is unknow");
      return 0;
  }
  return 0;
}

static void
get_hdi_video_frame_from_outInfo(GstHDIVideoFrame * frame, const OutputInfo * outInfo)
{
  VIDEO_FRAME_INFO_S* stVFrame = (VIDEO_FRAME_INFO_S*)outInfo->vendorPrivate;
  VIDEO_FRAME_S * pVBuf = &stVFrame->stVFrame;
  frame->height = pVBuf->u32Height;
  frame->width = pVBuf->u32Width;
  frame->pts = pVBuf->u64PTS;
  frame->stride = pVBuf->u32Stride[0];
  frame->phy_addr[0] = pVBuf->u64PhyAddr[0];
  frame->phy_addr[1] = pVBuf->u64PhyAddr[1];
  frame->pixel_format = YVU_SEMIPLANAR_420;
  frame->buffer_size = gst_get_buffer_size(frame->width, frame->height, frame->pixel_format);
  frame->gst_format = gst_hdi_video_get_format_from_hdi(frame->pixel_format);
  frame->enField = pVBuf->enField;
}
static void
gst_hdi_cha_tile_to_linear_8Bit(const guint8 * map, const GstHDIVideoFrame * frame)
{
  return;
}
