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

#ifndef GST_VENC_BASE_H
#define GST_VENC_BASE_H

#include <gst/video/gstvideoencoder.h>
#include <queue>
#include "gst_shmem_allocator.h"
#include "gst_shmem_pool.h"
#include "i_gst_codec.h"

#ifndef GST_API_EXPORT
#define GST_API_EXPORT __attribute__((visibility("default")))
#endif

G_BEGIN_DECLS

#define GST_TYPE_VENC_BASE \
    (gst_venc_base_get_type())
#define GST_VENC_BASE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_VENC_BASE, GstVencBase))
#define GST_VENC_BASE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_VENC_BASE, GstVencBaseClass))
#define GST_VENC_BASE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_VENC_BASE, GstVencBaseClass))
#define GST_IS_VENC_BASE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_VENC_BASE))
#define GST_IS_VENC_BASE_CLASS(obj) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_VENC_BASE))

typedef struct _GstVencBase GstVencBase;
typedef struct _GstVencBaseClass GstVencBaseClass;
typedef struct _GstVencBasePort GstVencBasePort;

struct _GstVencBasePort {
    gint frame_rate;
    gint width;
    gint height;
    guint min_buffer_cnt;
    guint buffer_cnt;
    guint buffer_size;
    std::shared_ptr<OHOS::Media::AVSharedMemoryPool> av_shmem_pool;
    GstShMemAllocator *allocator;
    gint64 frame_cnt;
    gint64 first_frame_time;
    gint64 last_frame_time;
};

struct _GstVencBase {
    GstVideoEncoder parent;
    std::shared_ptr<OHOS::Media::IGstCodec> encoder;
    GMutex drain_lock;
    GCond drain_cond;
    gboolean draining;
    GMutex lock;
    gboolean flushing;
    gboolean prepared;
    gboolean useBuffers;
    GstVideoCodecState *input_state;
    GstVideoCodecState *output_state;
    std::vector<GstVideoFormat> formats;
    GstVideoFormat format;
    OHOS::Media::GstCompressionFormat compress_format;
    gboolean is_codec_outbuffer;
    GstVencBasePort input;
    GstVencBasePort output;
    gint frame_rate;
    gint width;
    gint height;
    gint nstride;
    gint nslice_height;
    gint memtype;
    gint usage;
    guint bitrate;
    GstBufferPool *inpool;
    GstBufferPool *outpool;
};

struct _GstVencBaseClass {
    GstVideoEncoderClass parent_class;
    std::shared_ptr<OHOS::Media::IGstCodec> (*create_codec)(GstElementClass *kclass);
    GstCaps *(*get_caps)(GstVencBase *self, GstVideoCodecState *state);
};

GST_API_EXPORT GType gst_venc_base_get_type(void);

G_END_DECLS

#endif /* GST_VENC_BASE_H */
