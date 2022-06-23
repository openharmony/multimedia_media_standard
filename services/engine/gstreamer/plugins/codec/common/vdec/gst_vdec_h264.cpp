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

#include "gst_vdec_h264.h"
#include "securec.h"

G_DEFINE_TYPE(GstVdecH264, gst_vdec_h264, GST_TYPE_VDEC_BASE);

static GstBuffer *handle_slice_buffer(GstVdecBase *self, GstBuffer *buffer, bool &ready_push, bool is_finish);
static gboolean cat_slice_buffer(GstVdecBase *self, GstMapInfo *src_info);
static void flush_cache_slice_buffer(GstVdecBase *self);

static void gst_vdec_h264_class_init(GstVdecH264Class *klass)
{
    GST_DEBUG_OBJECT(klass, "Init h264 class");
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
    GstVdecBaseClass *base_class = GST_VDEC_BASE_CLASS(klass);
    base_class->handle_slice_buffer = handle_slice_buffer;
    base_class->flush_cache_slice_buffer = flush_cache_slice_buffer;

    gst_element_class_set_static_metadata(element_class,
        "Hardware Driver Interface H.264 Video Decoder",
        "Codec/Decoder/Video/Hardware",
        "Decode H.264 video streams",
        "OpenHarmony");
    const gchar *sink_caps_string = "video/x-h264, "
        "alignment=(string) nal, "
        "stream-format=(string){ byte-stream }";
    GstCaps *sink_caps = gst_caps_from_string(sink_caps_string);

    if (sink_caps != nullptr) {
        GstPadTemplate *sink_templ = gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS, sink_caps);
        gst_element_class_add_pad_template(element_class, sink_templ);
        gst_caps_unref(sink_caps);
    }
}

static void gst_vdec_h264_init(GstVdecH264 *self)
{
    (void)self;
    self->is_slice_buffer = false;
    self->cache_offset = 0;
    self->cache_slice_buffer = nullptr;
}

static GstBuffer *handle_slice_buffer(GstVdecBase *self, GstBuffer *buffer, bool &ready_push, bool is_finish)
{
    GstVdecH264 *vdec_h264 = GST_VDEC_H264(self);
    GstBuffer *buf = nullptr;
    if (is_finish) {
        gst_buffer_set_size(vdec_h264->cache_slice_buffer, vdec_h264->cache_offset);
        vdec_h264->is_slice_buffer = false;
        ready_push = true;
        return vdec_h264->cache_slice_buffer;
    }

    GstMapInfo info = GST_MAP_INFO_INIT;
    if (!gst_buffer_map(buffer, &info, GST_MAP_READ)) {
        GST_ERROR_OBJECT(self, "map buffer fail");
        return buffer;
    }
    gboolean slice_flag = false;
    guint8 offset = 2;
    for (gsize i = 0; i < info.size - offset; i++) {
        if (info.data[i] == 0x01) {
            slice_flag = ((info.data[i + offset] & 0x80) == 0x80) ? true : false; // 0x80 is nal flag of slice
            break;
        }
    }

    if (slice_flag == true && vdec_h264->is_slice_buffer == false) { // cache the first slice frame
        (void)cat_slice_buffer(self, &info);
        vdec_h264->is_slice_buffer = true;
        buf = vdec_h264->cache_slice_buffer;
        ready_push = false;
    } else if (slice_flag == false && vdec_h264->is_slice_buffer == true) { // cache the middle slice frame
        (void)cat_slice_buffer(self, &info);
        buf = vdec_h264->cache_slice_buffer;
        ready_push = false;
    } else if (slice_flag == true && vdec_h264->is_slice_buffer == true) { // get full frame, cache next first slice
        buf = vdec_h264->cache_slice_buffer;
        vdec_h264->cache_slice_buffer = nullptr;
        gst_buffer_set_size(buf, vdec_h264->cache_offset);
        ready_push = true;
        vdec_h264->cache_offset = 0;
        (void)cat_slice_buffer(self, &info);
    } else {
        buf = buffer;
        ready_push = true;
    }

    gst_buffer_unmap(buffer, &info);
    return buf;
}

static gboolean cat_slice_buffer(GstVdecBase *self, GstMapInfo *src_info)
{
    GstVdecH264 *vdec_h264 = GST_VDEC_H264(self);
    if (vdec_h264->cache_slice_buffer == nullptr) {
        GstFlowReturn flow_ret = gst_buffer_pool_acquire_buffer(self->inpool, &vdec_h264->cache_slice_buffer, nullptr);
        if (flow_ret != GST_FLOW_OK || vdec_h264->cache_slice_buffer == nullptr) {
            GST_ERROR_OBJECT(self, "Acquire buffer is nullptr");
            return false;
        }
    }
    GstMapInfo cache_info = GST_MAP_INFO_INIT;
    g_return_val_if_fail(gst_buffer_map(vdec_h264->cache_slice_buffer, &cache_info, GST_MAP_READ), FALSE);
    errno_t ret = memcpy_s(cache_info.data + vdec_h264->cache_offset,
        self->input.buffer_size - vdec_h264->cache_offset, src_info->data, src_info->size);
    if (ret != EOK) {
        GST_ERROR_OBJECT(self, "memcpy_s fail");
        gst_buffer_unmap(vdec_h264->cache_slice_buffer, &cache_info);
        return false;
    }
    vdec_h264->cache_offset += src_info->size;
    gst_buffer_unmap(vdec_h264->cache_slice_buffer, &cache_info);
    return true;
}

static void flush_cache_slice_buffer(GstVdecBase *self)
{
    GstVdecH264 *vdec_h264 = GST_VDEC_H264(self);
    if (vdec_h264 != nullptr) {
        vdec_h264->cache_offset = 0;
    }

}