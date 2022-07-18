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

#ifndef GST_SURFACE_MEM_SINK_H
#define GST_SURFACE_MEM_SINK_H

#include <cstdio>
#include "gst_mem_sink.h"

G_BEGIN_DECLS

#define GST_TYPE_SURFACE_MEM_SINK (gst_surface_mem_sink_get_type())
#define GST_SURFACE_MEM_SINK(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_SURFACE_MEM_SINK, GstSurfaceMemSink))
#define GST_SURFACE_MEM_SINK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_SURFACE_MEM_SINK, GstSurfaceMemSinkClass))
#define GST_IS_SURFACE_MEM_SINK(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_SURFACE_MEM_SINK))
#define GST_IS_SURFACE_MEM_SINK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_SURFACE_MEM_SINK))
#define GST_SURFACE_MEM_SINK_CAST(obj) ((GstSurfaceMemSink*)(obj))
#define GST_SURFACE_MEM_SINK_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_SURFACE_MEM_SINK, GstSurfaceMemSinkClass))

typedef struct _GstSurfaceMemSink GstSurfaceMemSink;
typedef struct _GstSurfaceMemSinkClass GstSurfaceMemSinkClass;
typedef struct _GstSurfaceMemSinkPrivate GstSurfaceMemSinkPrivate;
typedef struct _GstSurfaceMemSinkDump GstSurfaceMemSinkDump;

struct _GstSurfaceMemSinkDump {
    gboolean enable_dump;
    FILE *dump_file;
};

struct _GstSurfaceMemSink {
    GstMemSink memsink;
    GstBuffer *prerollBuffer;
    gboolean firstRenderFrame;
    gboolean preInitPool;
    gboolean performanceMode;
    /* < private > */
    GstSurfaceMemSinkPrivate *priv;
    GstSurfaceMemSinkDump dump;
    GstCaps *caps;
    guint lastRate;
    guint renderCnt;
};

struct _GstSurfaceMemSinkClass {
    GstMemSinkClass basesink_class;
    GstFlowReturn (*do_app_render) (GstSurfaceMemSink *memsink, GstBuffer *buffer, bool is_preroll);
};

GST_API_EXPORT GType gst_surface_mem_sink_get_type(void);

#ifdef G_DEFINE_AUTOPTR_CLEANUP_FUNC
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GstSurfaceMemSink, gst_object_unref)
#endif

G_END_DECLS
#endif
