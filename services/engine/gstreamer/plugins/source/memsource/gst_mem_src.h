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

#ifndef __GST_MEM_SRC_H__
#define __GST_MEM_SRC_H__

#include <gst/base/gstbasesrc.h>

G_BEGIN_DECLS

#ifndef GST_API_EXPORT
#define GST_API_EXPORT __attribute__((visibility("default")))
#endif

#define GST_TYPE_MEM_SRC (gst_mem_src_get_type())
#define GST_MEM_SRC(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_MEM_SRC, GstMemSrc))
#define GST_MEM_SRC_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_MEM_SRC, GstMemSrcClass))
#define GST_MEM_SRC_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_MEM_SRC, GstMemSrcClass))
#define GST_IS_MEM_SRC(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_MEM_SRC))
#define GST_IS_MEM_SRC_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_MEM_SRC))
#define GST_MEM_SRC_CAST(obj) ((GstMemSrc*)(obj))

typedef struct _GstMemSrc GstMemSrc;
typedef struct _GstMemSrcClass GstMemSrcClass;
typedef struct _GstMemSrcPrivate GstMemSrcPrivate;
typedef GstFlowReturn (*BufferAvailable) (GstMemSrc *memsrc, gpointer user_data);

struct _GstMemSrc {
    GstBaseSrc basesrc;
    GstCaps *caps;
    guint buffer_size;
    guint buffer_num;

    /* < private > */
    GstMemSrcPrivate *priv;
};

struct _GstMemSrcClass {
    GstBaseSrcClass parent;

    // for API and action calling, subclass need accomplish it
    GstBuffer *(*pull_buffer) (GstMemSrc *memsrc);
    GstFlowReturn (*push_buffer) (GstMemSrc *memsrc, GstBuffer *buffer);
};

// for subclass to use
GST_API_EXPORT GstFlowReturn gst_mem_src_buffer_available(GstMemSrc *memsrc);
// for app to use
GST_API_EXPORT GstBuffer *gst_mem_src_pull_buffer(GstMemSrc *memsrc);

GST_API_EXPORT GstFlowReturn gst_mem_src_push_buffer(GstMemSrc *memsrc, GstBuffer *buffer);

GST_API_EXPORT void gst_mem_src_set_callback(GstMemSrc *memsrc, BufferAvailable callback,
    gpointer user_data, GDestroyNotify notify);

GST_API_EXPORT GType gst_mem_src_get_type(void);

G_END_DECLS
#endif