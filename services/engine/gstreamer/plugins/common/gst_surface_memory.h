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

#ifndef GST_SURFACE_MEMORY_H
#define GST_SURFACE_MEMORY_H

#include <gst/gst.h>
#include "surface.h"

typedef struct _GstSurfaceMemory GstSurfaceMemory;

struct _GstSurfaceMemory {
    GstMemory parent;
    OHOS::sptr<OHOS::SurfaceBuffer> buf;
    gint fence;
    gboolean need_render;
};

static const char GST_SURFACE_MEMORY_TYPE[] = "SurfaceMemory";

static inline gboolean gst_is_surface_memory(GstMemory *mem)
{
    return gst_memory_is_type(mem, GST_SURFACE_MEMORY_TYPE);
}

#endif
