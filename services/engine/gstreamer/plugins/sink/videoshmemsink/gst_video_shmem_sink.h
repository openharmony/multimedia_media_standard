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

#ifndef GST_VIDEO_SHMEM_SINK_H
#define GST_VIDEO_SHMEM_SINK_H

#include "config.h"
#include <gst/base/gstbasesink.h>

G_BEGIN_DECLS

#define GST_TYPE_VIDEO_SHMEM_SINK (gst_video_shmem_sink_get_type())
#define GST_VIDEO_SHMEM_SINK(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_VIDEO_SHMEM_SINK, GstVideoShMemSink))
#define GST_VIDEO_SHMEM_SINK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_VIDEO_SHMEM_SINK, GstVideoShMemSinkClass))
#define GST_IS_VIDEO_SHMEM_SINK(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_VIDEO_SHMEM_SINK))
#define GST_IS_VIDEO_SHMEM_SINK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_VIDEO_SHMEM_SINK))
#define GST_VIDEO_SHMEM_SINK_CAST(obj) ((GstVideoShMemSink*)(obj))

typedef struct _GstVideoShMemSink GstVideoShMemSink;
typedef struct _GstVideoShMemSinkClass GstVideoShMemSinkClass;
typedef struct _GstVideoShMemSinkPrivate GstVideoShMemSinkPrivate;

struct _GstVideoShMemSink {
    GstBaseSink basesink;

    /* < private > */
    GstVideoShMemSinkPrivate *priv;
};

struct _GstVideoShMemSinkClass {
    GstBaseSinkClass basesink_class;

    /* signals */
    GstFlowReturn (*new_preroll)(GstVideoShMemSink *appsink);
    GstFlowReturn (*new_sample)(GstVideoShMemSink *appsink);

    /* actions */
    GstSample *(*pull_preroll)(GstVideoShMemSink *appsink);
    GstSample *(*pull_sample)(GstVideoShMemSink *appsink);
};

G_GNUC_INTERNAL GType gst_video_shmem_sink_get_type(void);

#ifdef G_DEFINE_AUTOPTR_CLEANUP_FUNC
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GstVideoShMemSink, gst_object_unref)
#endif

G_END_DECLS
#endif