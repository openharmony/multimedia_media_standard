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

#ifndef __GST_SURFACE_VIDEO_SRC_H__
#define __GST_SURFACE_VIDEO_SRC_H__

#include <memory>
#include <gst/base/gstpushsrc.h>
#include "common_utils.h"
#include "video_capture.h"

G_BEGIN_DECLS

#define GST_TYPE_SURFACE_VIDEO_SRC \
    (gst_surface_video_src_get_type())
#define GST_SURFACE_VIDEO_SRC(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_SURFACE_VIDEO_SRC, GstSurfaceVideoSrc))
#define GST_SURFACE_VIDEO_SRC_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_SURFACE_VIDEO_SRC, GstSurfaceVideoSrcClass))
#define GST_IS_SURFACE_VIDEO_SRC(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_SURFACE_VIDEO_SRC))
#define GST_IS_SURFACE_VIDEO_SRC_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_SURFACE_VIDEO_SRC))
#define GST_SURFACE_VIDEO_SRC_CAST(obj) ((GstSurfaceVideoSrc *)obj)

/**
 * GstSurfaceVideoSrc:
 *
 * Opaque #GstSurfaceVideoSrc structure.
 */
struct _GstSurfaceVideoSrc {
    GstBaseSrc parent_element;

    /* private */
    VideoStreamType stream_type;
    std::unique_ptr<OHOS::Media::VideoCapture> capture;
    GstCaps *src_caps;
    guint video_width;
    guint video_height;
    guint video_frame_rate;
    gboolean is_start;
    gboolean need_codec_data;
    gboolean is_eos;
    gboolean is_flushing;
};

struct _GstSurfaceVideoSrcClass {
    GstPushSrcClass parent_class;
};

using GstSurfaceVideoSrc = struct _GstSurfaceVideoSrc;
using GstSurfaceVideoSrcClass = struct _GstSurfaceVideoSrcClass;

GType gst_surface_video_src_get_type(void);

G_END_DECLS
#endif /* __GST_SURFACE_VIDEO_SRC_H__ */
