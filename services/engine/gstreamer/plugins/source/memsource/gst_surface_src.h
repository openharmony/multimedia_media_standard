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

#ifndef __GST_SURFACE_SRC_H__
#define __GST_SURFACE_SRC_H__

#include "gst_mem_src.h"
#include "surface.h"

G_BEGIN_DECLS

#define GST_TYPE_SURFACE_SRC (gst_surface_src_get_type())
#define GST_SURFACE_SRC(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_SURFACE_SRC, GstSurfaceSrc))
#define GST_SURFACE_SRC_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_SURFACE_SRC, GstSurfaceSrcClass))
#define GST_IS_SURFACE_SRC(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_SURFACE_SRC))
#define GST_IS_SURFACE_SRC_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_SURFACE_SRC))
#define GST_SURFACE_SRC_CAST(obj) ((GstSurfaceSrc*)(obj))

typedef struct _GstSurfaceSrc GstSurfaceSrc;
typedef struct _GstSurfaceSrcClass GstSurfaceSrcClass;

struct _GstSurfaceSrc {
    GstMemSrc memsrc;
    OHOS::sptr<OHOS::Surface> consumerSurface;
    OHOS::sptr<OHOS::Surface> producerSurface;
    GstBufferPool *pool;
    guint stride;
    gboolean need_flush;
    gboolean flushing;
};

struct _GstSurfaceSrcClass {
    GstMemSrcClass parent_class;
};

GST_API_EXPORT GType gst_surface_src_get_type(void);

G_END_DECLS
#endif