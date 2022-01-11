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

#ifndef GST_CONSUMER_SURFACE_MEMORY_H
#define GST_CONSUMER_SURFACE_MEMORY_H

#include <gst/gst.h>
#include "surface_buffer.h"

typedef struct _GstConsumerSurfaceMemory GstConsumerSurfaceMemory;

struct _GstConsumerSurfaceMemory {
    GstMemory parent;
    OHOS::sptr<OHOS::SurfaceBuffer> surface_buffer;
    intptr_t buffer_handle;
    gint32 fencefd;
    gint64 timestamp;
    gint32 is_key_frame;
    OHOS::Rect damage;
};

gboolean gst_is_consumer_surface_memory(GstMemory *mem);

#endif
