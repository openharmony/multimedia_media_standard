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

#ifndef GST_SHARED_MEM_SINK_H
#define GST_SHARED_MEM_SINK_H

#include "gst_mem_sink.h"

G_BEGIN_DECLS

#define GST_TYPE_SHARED_MEM_SINK (gst_shared_mem_sink_get_type())
#define GST_SHARED_MEM_SINK(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_SHARED_MEM_SINK, GstSharedMemSink))
#define GST_SHARED_MEM_SINK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_SHARED_MEM_SINK, GstSharedMemSinkClass))
#define GST_IS_SHARED_MEM_SINK(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_SHARED_MEM_SINK))
#define GST_IS_SHARED_MEM_SINK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_SHARED_MEM_SINK))
#define GST_SHARED_MEM_SINK_CAST(obj) ((GstSharedMemSink*)(obj))

typedef struct _GstSharedMemSink GstSharedMemSink;
typedef struct _GstSharedMemSinkClass GstSharedMemSinkClass;
typedef struct _GstSharedMemSinkPrivate GstSharedMemSinkPrivate;

struct _GstSharedMemSink {
    GstMemSink memsink;

    /* < private > */
    GstSharedMemSinkPrivate *priv;
};

struct _GstSharedMemSinkClass {
    GstMemSinkClass basesink_class;
};

GST_API_EXPORT GType gst_shared_mem_sink_get_type(void);

#ifdef G_DEFINE_AUTOPTR_CLEANUP_FUNC
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GstSharedMemSink, gst_object_unref)
#endif

G_END_DECLS
#endif
