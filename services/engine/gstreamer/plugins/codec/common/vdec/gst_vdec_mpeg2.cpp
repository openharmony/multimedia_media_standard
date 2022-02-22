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

#include "gst_vdec_mpeg2.h"

G_DEFINE_TYPE(GstVdecMpeg2, gst_vdec_mpeg2, GST_TYPE_VDEC_BASE);

static void gst_vdec_mpeg2_class_init(GstVdecMpeg2Class *klass)
{
    GST_DEBUG_OBJECT(klass, "Init mpeg2 class");
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

    gst_element_class_set_static_metadata(element_class,
        "Hardware Driver Interface Mpeg2 Video Decoder",
        "Codec/Decoder/Video/Hardware",
        "Decode Mpeg2 video streams",
        "OpenHarmony");
    const gchar *sink_caps_string = "video/mpeg, "
        "mpegversion=(int) 2, "
        "systemstream=(boolean) false";
    GstCaps *sink_caps = gst_caps_from_string(sink_caps_string);

    if (sink_caps != nullptr) {
        GstPadTemplate *sink_templ = gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS, sink_caps);
        gst_element_class_add_pad_template(element_class, sink_templ);
        gst_caps_unref(sink_caps);
    }
}

static void gst_vdec_mpeg2_init(GstVdecMpeg2 *self)
{
    (void)self;
}