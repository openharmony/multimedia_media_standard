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

#ifndef GST_HDI_VIDEO_H
#define GST_HDI_VIDEO_H
#include "gst_hdi.h"
#define GST_HDI_VIDEO_DEC_SUPPORTED_FORMATS "{ NV21 }"

typedef struct {
  GstVideoFormat gst_format;
  PixelFormat pixel_format;
} GstHDIVideoNegotiationMap;

typedef struct {
  AvCodecMime mime;
  uint32_t buffer_size;
  CodecType codec_type;
  uint32_t height;
  uint32_t width;
  uint32_t stride;
  PixelFormat pixel_format;
  uint32_t frame_rate;
  uint64_t output_pts;
} GstHdiVideoFormat;

GstVideoFormat gst_hdi_video_get_format_from_hdi (PixelFormat hdiColorformat);
void gst_hdi_set_video_caps_format(GstCaps * caps, const ResizableArray * formats);
GstVideoCodecFrame * gst_hdi_video_find_nearest_frame (const GstElement * element, const OutputInfo * buf,
    GList * frames);
#endif /* GST_HDI_VIDEO_H */