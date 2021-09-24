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

#ifndef GST_HDI_H
#define GST_HDI_H

#include "config.h"
#include <gmodule.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/allocators/gstdmabuf.h>
#include <gst/video/gstvideodecoder.h>
#include <unistd.h>

#include "codec_interface.h"
#include "codec_type.h"

typedef struct _GstHDICodec GstHDICodec;
typedef struct _GstHDIClassData GstHDIClassData;

typedef enum {
    HDI_FAILURE = -1,
    HDI_SUCCESS = 0,
    HDI_ERR_STREAM_BUF_FULL = 100,
    HDI_ERR_FRAME_BUF_EMPTY,
    HDI_RECEIVE_EOS,
    HDI_ERR_INVALID_OP,
} HDI_ERRORTYPE;

typedef enum {
    GST_HDI_BUFFER_INTERNAL_MODE,
    GST_HDI_BUFFER_EXTERNAL_MODE,
} GstHDIBufferMode;

typedef enum {
    GST_HDI_BUFFER_INTERNAL_SUPPORT  = 0x1,
    GST_HDI_BUFFER_EXTERNAL_SUPPORT  = 0x2,
} GstHDIBufferModeSupport;

typedef struct {
    AvCodecMime mime;
    guint buffer_size;
    CodecType codec_type;
    guint height;
    guint width;
    guint stride;
    PixelFormat pixel_format;
    GstVideoFormat gst_format;
    guint frame_rate;
    guint64 pts;
#ifdef GST_HDI_PARAM_PILE
    guint64 phy_addr[2];
    guint8 *vir_addr;
#endif
} GstHDIFormat;

struct _GstHDICodec {
    GstMiniObject mini_object;
    GstElement *parent;
    CODEC_HANDLETYPE handle;
    GMutex start_lock;
    gboolean hdi_started;
    gint input_buffer_num;
    gint output_buffer_num;
    GstHDIBufferMode input_mode;
    GstHDIBufferMode output_mode;
    GList *input_free_buffers;
    GList *output_free_buffers;
    GList *output_dirty_buffers;
    void (*format_to_params)(Param *param, const GstHDIFormat *format, gint *actual_size, const gint max_num);
};

struct _GstHDIClassData {
    const gchar *codec_name;
    CodecType codec_type;
    AvCodecMime mime;
    gboolean is_soft;
    gint max_width;
    gint max_height;
    GList *support_video_format;
    guint input_buffer_support;
    guint output_buffer_support;

    void (*format_to_params)(Param *param, const GstHDIFormat *format, gint *actual_size, const gint max_num);
    const gchar *default_src_template_caps;
    const gchar *default_sink_template_caps;
    GstCaps *src_caps;
    GstCaps *sink_caps;
};

GstHDICodec *gst_hdi_codec_new(const GstHDIClassData *cdata, const GstHDIFormat *format);
gboolean gst_hdi_alloc_buffers(GstHDICodec *codec);
void gst_hdi_release_buffers(GstHDICodec *codec);
gint gst_hdi_codec_set_params(const GstHDICodec *codec, const GstHDIFormat *format);
gint gst_hdi_codec_get_params(const GstHDICodec *codec, GstHDIFormat *format);
GstHDICodec *gst_hdi_codec_ref(GstHDICodec *codec);
gint gst_hdi_codec_start(GstHDICodec *codec);
gboolean gst_hdi_codec_is_start(GstHDICodec *codec);
gint gst_hdi_codec_stop(GstHDICodec *codec);
void gst_hdi_codec_unref(GstHDICodec *codec);
gint gst_hdi_port_flush(GstHDICodec *codec, DirectionType directType);
gint gst_hdi_queue_input_buffer(const GstHDICodec *codec, GstBuffer *gst_buffer, guint timeoutMs);
gint gst_hdi_deque_input_buffer(const GstHDICodec *codec, GstBuffer **gst_buffer, guint timeoutMs);
gint gst_hdi_queue_output_buffers(GstHDICodec *codec, guint timeoutMs);
gint gst_hdi_deque_output_buffer(GstHDICodec *codec, GstBuffer **gst_buffer, guint timeoutMs);
#ifdef GST_HDI_PARAM_PILE
gint gst_hdi_deque_output_buffer_and_format(GstHDICodec *codec, GstBuffer **gst_buffer,
    GstHDIFormat *format, guint timeoutMs);
#endif
void gst_hdi_class_data_init(GstHDIClassData *classData);
void gst_hdi_class_pad_caps_init(const GstHDIClassData *classData, GstElementClass *element_class);
const gchar *gst_hdi_error_to_string(gint err);
GHashTable *gst_hdi_init_caps_map(void);

GType gst_hdi_codec_get_type(void);
#endif /* GST_HDI_H */