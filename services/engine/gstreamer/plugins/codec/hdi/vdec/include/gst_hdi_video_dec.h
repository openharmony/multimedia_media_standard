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

#ifndef GST_HDI_VIDEO_DEC_H
#define GST_HDI_VIDEO_DEC_H

#include "gst_hdi.h"
#include "gst_hdi_video.h"

G_BEGIN_DECLS

#define GST_TYPE_HDI_VIDEO_DEC \
    (gst_hdi_video_dec_get_type())
#define GST_HDI_VIDEO_DEC(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HDI_VIDEO_DEC,GstHDIVideoDec))
#define GST_HDI_VIDEO_DEC_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HDI_VIDEO_DEC,GstHDIVideoDecClass))
#define GST_HDI_VIDEO_DEC_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_HDI_VIDEO_DEC,GstHDIVideoDecClass))
#define GST_IS_HDI_VIDEO_DEC(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HDI_VIDEO_DEC))
#define GST_IS_HDI_VIDEO_DEC_CLASS(obj) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HDI_VIDEO_DEC))

typedef struct _GstHDIVideoDec GstHDIVideoDec;
typedef struct _GstHDIVideoDecClass GstHDIVideoDecClass;

struct _GstHDIVideoDec {
    GstVideoDecoder parent;
    GMutex drain_lock;
    GCond drain_cond;
    gboolean draining;
    gboolean hdi_flushing;
    GMutex lock;
    GstHDICodec *dec;
    void *surface;
    gboolean started;
    gboolean pausing_task;
    gboolean useBuffers;
    GstFlowReturn downstream_flow_ret;
    GstClockTime last_upstream_ts;
    GstHDIBufferMode inputBufferMode;
    GstVideoCodecState *input_state;
    GstHDIFormat hdi_video_in_format;
    GstHDIFormat hdi_video_out_format;
};

struct _GstHDIVideoDecClass {
    GstVideoDecoderClass parentClass;
    GstHDIClassData cdata;
    gboolean (*isFormatChange) (GstHDIVideoDec *self, GstVideoCodecState *state);
    gboolean (*setFormat) (GstHDIVideoDec *self, GstVideoCodecState *state);
};

GType gst_hdi_video_dec_get_type(void);

G_END_DECLS

#endif /* GST_HDI_VIDEO_DEC_H */
