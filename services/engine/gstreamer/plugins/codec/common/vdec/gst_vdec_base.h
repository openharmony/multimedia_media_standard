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

#ifndef GST_VDEC_BASE_H
#define GST_VDEC_BASE_H

#include <vector>
#include <list>
#include <gst/video/gstvideodecoder.h>
#include "gst_shmem_allocator.h"
#include "gst_shmem_pool.h"
#include "i_gst_codec.h"

#ifndef GST_API_EXPORT
#define GST_API_EXPORT __attribute__((visibility("default")))
#endif

G_BEGIN_DECLS

#define GST_TYPE_VDEC_BASE \
    (gst_vdec_base_get_type())
#define GST_VDEC_BASE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_VDEC_BASE, GstVdecBase))
#define GST_VDEC_BASE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_VDEC_BASE, GstVdecBaseClass))
#define GST_VDEC_BASE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_VDEC_BASE, GstVdecBaseClass))
#define GST_IS_VDEC_BASE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_VDEC_BASE))
#define GST_IS_VDEC_BASE_CLASS(obj) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_VDEC_BASE))

typedef struct _GstVdecBase GstVdecBase;
typedef struct _GstVdecBaseClass GstVdecBaseClass;
typedef struct _GstVdecBasePort GstVdecBasePort;
typedef struct _DisplayRect DisplayRect;

struct _GstVdecBasePort {
    gint frame_rate;
    gint width;
    gint height;
    guint min_buffer_cnt;
    guint buffer_cnt;
    guint buffer_size;
    std::shared_ptr<OHOS::Media::AVSharedMemoryPool> av_shmem_pool;
    GstShMemAllocator *allocator;
    GstAllocationParams allocParams;
    gint64 frame_cnt;
    gint64 first_frame_time;
    gint64 last_frame_time;
    gboolean enable_dump;
    FILE *dump_file;
    gboolean first_frame;
};

struct _DisplayRect {
    gint x;
    gint y;
    gint width;
    gint height;
};

struct _GstVdecBase {
    GstVideoDecoder parent;
    std::shared_ptr<OHOS::Media::IGstCodec> decoder;
    GMutex drain_lock;
    GCond drain_cond;
    gboolean draining;
    GMutex lock;
    gboolean flushing;
    gboolean prepared;
    GstVideoCodecState *input_state;
    GstVideoCodecState *output_state;
    std::vector<GstVideoFormat> formats;
    GstVideoFormat format;
    OHOS::Media::GstCompressionFormat compress_format;
    gboolean is_codec_outbuffer;
    GstVdecBasePort input;
    GstVdecBasePort output;
    gint frame_rate;
    gint width;
    gint height;
    gint memtype;
    gint usage;
    GstBufferPool *inpool;
    GstBufferPool *outpool;
    guint coding_outbuf_cnt;
    guint out_buffer_cnt;
    guint out_buffer_max_cnt;
    std::list<GstClockTime> pts_list;
    GstClockTime last_pts;
    gboolean flushing_stoping;
    gboolean decoder_start;
    gint stride;
    gint stride_height;
    gint real_stride;
    gint real_stride_height;
    DisplayRect rect;
};

struct _GstVdecBaseClass {
    GstVideoDecoderClass parentClass;
    std::shared_ptr<OHOS::Media::IGstCodec> (*create_codec)(GstElementClass *kclass);
};

GST_API_EXPORT GType gst_vdec_base_get_type(void);

G_END_DECLS

#endif /* GST_VDEC_BASE_H */
