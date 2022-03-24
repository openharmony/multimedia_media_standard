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

#ifndef GST_HDI_H264_DEC_H
#define GST_HDI_H264_DEC_H

#include "gst_hdi_video_dec.h"

G_BEGIN_DECLS

#define GST_TYPE_HDI_H264_DEC \
    (gst_hdi_h264_dec_get_type())
#define GST_HDI_H264_DEC(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HDI_H264_DEC,GstHDIH264Dec))
#define GST_HDI_H264_DEC_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HDI_H264_DEC,GstHDIH264DecClass))
#define GST_HDI_H264_DEC_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_HDI_H264_DEC,GstHDIH264DecClass))
#define GST_IS_HDI_H264_DEC(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HDI_H264_DEC))
#define GST_IS_HDI_H264_DEC_CLASS(obj) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HDI_H264_DEC))

typedef struct _GstHDIH264Dec GstHDIH264Dec;
typedef struct _GstHDIH264DecClass GstHDIH264DecClass;

struct _GstHDIH264Dec {
    GstHDIVideoDec parent;
};

struct _GstHDIH264DecClass {
    GstHDIVideoDecClass parent_class;
};

GType gst_hdi_h264_dec_get_type(void);

G_END_DECLS

#endif /* GST_HDI_H264_DEC_H */
