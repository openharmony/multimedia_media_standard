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

#include "gst_hdi_video.h"
#include "gst_hdi_video_dec.h"

GstVideoFormat gst_hdi_video_pixelformat_to_gstvideoformat(PixelFormat hdiColorformat)
{
    GstVideoFormat format;

    switch (hdiColorformat) {
        case YVU_SEMIPLANAR_420:
            format = GST_VIDEO_FORMAT_NV21;
            break;
        default:
            format = GST_VIDEO_FORMAT_UNKNOWN;
            break;
    }

    return format;
}

void gst_hdi_video_set_caps_pixelformat(GstCaps *caps, const GList *formats)
{
    g_return_if_fail(formats != NULL);
    g_return_if_fail(caps != NULL);
    GValue arr = G_VALUE_INIT;
    GValue item = G_VALUE_INIT;
    g_value_init(&arr, GST_TYPE_LIST);
    g_value_init(&item, G_TYPE_STRING);
    for (const GList *format = formats; format != NULL; format = format->next) {
        if (format->data == NULL) {
            continue;
        }
        GstVideoFormat tmp = gst_hdi_video_pixelformat_to_gstvideoformat(*((PixelFormat*)(format->data)));
        if (tmp == GST_VIDEO_FORMAT_UNKNOWN) {
            continue;
        }
        g_value_set_string(&item, gst_video_format_to_string(tmp));
        gst_value_list_append_value(&arr, &item);
    }
    g_value_unset(&item);
    gst_caps_set_value(caps, "format", &arr);
    g_value_unset(&arr);
}