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
#include "config.h"
#include "gst_surface_mem_sink.h"
#include "gst_shared_mem_sink.h"
#include "gst_video_display_sink.h"

static gboolean plugin_init(GstPlugin *plugin)
{
    g_return_val_if_fail(plugin != nullptr, false);
    gboolean ret = gst_element_register(plugin, "surfacememsink", GST_RANK_PRIMARY, GST_TYPE_SURFACE_MEM_SINK);
    if (ret == FALSE) {
        GST_WARNING_OBJECT(nullptr, "register surfacememsink failed");
    }
    ret = gst_element_register(plugin, "sharedmemsink", GST_RANK_PRIMARY, GST_TYPE_SHARED_MEM_SINK);
    if (ret == FALSE) {
        GST_WARNING_OBJECT(nullptr, "register sharedmemsink failed");
    }
    ret = gst_element_register(plugin, "videodisplaysink", GST_RANK_PRIMARY, GST_TYPE_VIDEO_DISPLAY_SINK);
    if (ret == FALSE) {
        GST_WARNING_OBJECT(nullptr, "register videodisplaysink failed");
    }
    return TRUE;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    _mem_sink,
    "GStreamer Memory Sink",
    plugin_init,
    PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
