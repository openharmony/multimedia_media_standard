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
#include "gst_hdi_h264_dec.h"

GST_DEBUG_CATEGORY_STATIC (gst_hdi_h264_dec_debug_category);
#define GST_CAT_DEFAULT gst_hdi_h264_dec_debug_category

#define DEBUG_INIT \
  GST_DEBUG_CATEGORY_INIT (gst_hdi_h264_dec_debug_category, "hdih264dec", 0, \
      "debug category for gst-hdi video decoder base class");

G_DEFINE_TYPE_WITH_CODE (GstHDIH264Dec, gst_hdi_h264_dec,
    GST_TYPE_HDI_VIDEO_DEC, DEBUG_INIT);

static void
gst_hdi_h264_dec_class_init (GstHDIH264DecClass * klass)
{
  GstHDIVideoDecClass *videodec_class = GST_HDI_VIDEO_DEC_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  g_return_if_fail(videodec_class != nullptr);

  videodec_class->cdata.default_sink_template_caps = "video/x-h264, "
      "alignment=(string) au, "
      "stream-format=(string){ byte-stream }, "
      "width=(int) [1,MAX], " "height=(int) [1,MAX]";

  gst_element_class_set_static_metadata (element_class,
      "Hardware Driver Interface H.264 Video Decoder",
      "Codec/Decoder/Video/Hardware",
      "Decode H.264 video streams",
      "Huawei");
  gst_hdi_class_data_init (
      &videodec_class->cdata, MEDIA_MIMETYPE_VIDEO_AVC, VIDEO_DECODER, 0, "codec.avc.software.decoder");
  GstCaps * sink_caps = gst_caps_from_string (videodec_class->cdata.default_sink_template_caps);
  GstCaps * src_caps = gst_caps_from_string (videodec_class->cdata.default_src_template_caps);

  if (videodec_class->cdata.get_caps == TRUE) {
    gst_hdi_set_video_caps_format(src_caps, &videodec_class->cdata.caps.supportPixelFormats);
  }

  GstPadTemplate * src_templ = gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS, src_caps);
  gst_element_class_add_pad_template (element_class, src_templ);
  gst_caps_unref (src_caps);
  GstPadTemplate * sink_templ = gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS, sink_caps);
  gst_element_class_add_pad_template (element_class, sink_templ);
  gst_caps_unref (sink_caps);
}

static void
gst_hdi_h264_dec_init (GstHDIH264Dec * self)
{
}