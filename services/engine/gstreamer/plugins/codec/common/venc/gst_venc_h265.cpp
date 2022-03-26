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

#include "gst_venc_h265.h"

G_DEFINE_TYPE(GstVencH265, gst_venc_h265, GST_TYPE_VENC_BASE);

static GstCaps *gst_venc_h265_get_caps(GstVencBase *self, GstVideoCodecState *state);

static void gst_venc_h265_class_init(GstVencH265Class *klass)
{
    GST_DEBUG_OBJECT(klass, "Init h265 class");
    g_return_if_fail(klass != nullptr);
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
    GstVencBaseClass *base_class = GST_VENC_BASE_CLASS(klass);
    base_class->get_caps = gst_venc_h265_get_caps;

    gst_element_class_set_static_metadata(element_class,
        "Hardware Driver Interface H.265 Video Encoder",
        "Codec/Encoder/Video/Hardware",
        "Decode H.265 video streams",
        "OpenHarmony");
    const gchar *src_caps_string = "video/x-h265, "
        "alignment=(string) nal, "
        "stream-format=(string){ byte-stream }, "
        "width=(int) [1,MAX], " "height=(int) [1,MAX]";
    GstCaps *src_caps = gst_caps_from_string(src_caps_string);

    if (src_caps != nullptr) {
        GstPadTemplate *src_templ = gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS, src_caps);
        gst_element_class_add_pad_template(element_class, src_templ);
        gst_caps_unref(src_caps);
    }
}

static void gst_venc_h265_init(GstVencH265 *self)
{
    (void)self;
}

static GstCaps *gst_venc_h265_get_caps(GstVencBase *self, GstVideoCodecState *state)
{
    GstCaps *caps = gst_caps_new_simple("video/x-h265",
        "stream-format", G_TYPE_STRING, "byte-stream",
        "width", G_TYPE_INT, self->width,
        "height", G_TYPE_INT, self->height, nullptr);
    return caps;
}