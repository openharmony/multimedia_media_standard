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

#ifndef GST_MEM_SINK_H
#define GST_MEM_SINK_H

#include <gst/base/gstbasesink.h>

G_BEGIN_DECLS

#define GST_TYPE_MEM_SINK (gst_mem_sink_get_type())
#define GST_MEM_SINK(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_MEM_SINK, GstMemSink))
#define GST_MEM_SINK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_MEM_SINK, GstMemSinkClass))
#define GST_IS_MEM_SINK(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_MEM_SINK))
#define GST_IS_MEM_SINK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_MEM_SINK))
#define GST_MEM_SINK_CAST(obj) ((GstMemSink*)(obj))
#define GST_MEM_SINK_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_MEM_SINK, GstMemSinkClass))

#ifndef GST_API_EXPORT
#define GST_API_EXPORT __attribute__((visibility("default")))
#endif

typedef struct _GstMemSink GstMemSink;
typedef struct _GstMemSinkClass GstMemSinkClass;
typedef struct _GstMemSinkPrivate GstMemSinkPrivate;

typedef struct {
    void (*eos)(GstMemSink *memsink, gpointer user_data);
    GstFlowReturn (*new_preroll)(GstMemSink *memsink, GstBuffer *preroll, gpointer user_data);
    GstFlowReturn (*new_sample)(GstMemSink *memsink, GstBuffer *sample, gpointer user_data);
} GstMemSinkCallbacks;

struct _GstMemSink {
    GstBaseSink basesink;

    guint maxPoolCapacity; /* max buffer count for buffer pool */
    guint waitTime; /* longest waiting time for single try to acquire buffer from buffer pool */

    /* < private > */
    GstMemSinkPrivate *priv;
};

struct _GstMemSinkClass {
    GstBaseSinkClass basesink_class;

    gboolean (*do_propose_allocation) (GstMemSink *sink, GstQuery *query);
    GstFlowReturn (*do_stream_render) (GstMemSink *sink, GstBuffer **buffer);
    GstFlowReturn (*do_app_render) (GstMemSink *sink, GstBuffer *buffer);
};

GST_API_EXPORT GType gst_mem_sink_get_type(void);

/**
 * @brief call this interface to set the notifiers for new_preroll, new_sample and eos.
 *
 * @param memsink the sink element instance
 * @param callbacks callbacks, refer to {@GstMemSinkCallbacks}
 * @param userdata will be passed to callbacks
 * @param notify the function to be used to destroy the userdata when the memsink is disposed
 * @return GST_FLOW_OK if success, or error code.
 */
GST_API_EXPORT void gst_mem_sink_set_callback(GstMemSink *memsink,
                                              GstMemSinkCallbacks *callbacks,
                                              gpointer userdata,
                                              GDestroyNotify notify);

/**
 * @brief call this interface to indicate the app ends up the access of buffer, then
 * the buffer can be release back to buffer pool. Call this function is not necessary,
 * it is up to the sink and pool implementation and underlying memory type.
 *
 * @param memsink the sink element instance
 * @param buffer the buffer will be rendered, allowd to be nullptr at certain situation.
 * @return GST_FLOW_OK if success, or error code.
 */
GST_API_EXPORT GstFlowReturn gst_mem_sink_app_render(GstMemSink *memsink, GstBuffer *buffer);

#ifdef G_DEFINE_AUTOPTR_CLEANUP_FUNC
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GstMemSink, gst_object_unref)
#endif

G_END_DECLS
#endif
