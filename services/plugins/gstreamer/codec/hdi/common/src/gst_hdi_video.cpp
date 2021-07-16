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
#include <gst/allocators/gstdmabuf.h>
#include "gst_hdi_video_dec.h"

GstVideoFormat
gst_hdi_video_get_format_from_hdi (PixelFormat hdiColorformat)
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

GstVideoCodecFrame *
gst_hdi_video_find_nearest_frame (const GstElement * element, const OutputInfo * buf,
    GList * frames)
{
  g_return_val_if_fail (buf != nullptr, nullptr);
  GstVideoCodecFrame *best = nullptr;
  GstClockTimeDiff best_diff = G_MAXINT64;
  GstClockTime timestamp;
  GList *l = nullptr;

  timestamp =
      gst_util_uint64_scale ((guint64)buf->timeStamp, GST_SECOND, HDI_USECOND_PER_SECOND);

  GST_DEBUG_OBJECT (element, "look for ts %" GST_TIME_FORMAT,
      GST_TIME_ARGS (timestamp));

  for (l = frames; l != nullptr; l = l->next) {
    GstVideoCodecFrame *tmp = (GstVideoCodecFrame *)l->data;
    if (tmp == nullptr) {
      continue;
    }
    GstClockTimeDiff diff = ABS (GST_CLOCK_DIFF (timestamp, tmp->pts));

    GST_DEBUG_OBJECT (element,
        "  frame %u diff %" G_GINT64_FORMAT " ts %" GST_TIME_FORMAT,
        tmp->system_frame_number, diff, GST_TIME_ARGS (tmp->pts));

    if (diff < best_diff) {
      best = tmp;
      best_diff = diff;

      if (diff == 0)
        break;
    }
  }

  if (best != nullptr) {
    gst_video_codec_frame_ref (best);

    if (best_diff >= GST_USECOND)
      GST_DEBUG_OBJECT (element,
          "Difference between ts (%" GST_TIME_FORMAT ") and frame %u (%"
          GST_TIME_FORMAT ") seems too high (%" GST_TIME_FORMAT ")",
          GST_TIME_ARGS (timestamp), best->system_frame_number,
          GST_TIME_ARGS (best->pts), GST_TIME_ARGS (best_diff));
  } else {
    GST_WARNING_OBJECT (element, "No best frame has been found");
  }

  g_list_foreach (frames, (GFunc) gst_video_codec_frame_unref, nullptr);
  g_list_free (frames);

  return best;
}

void
gst_hdi_set_video_caps_format(GstCaps * caps, const ResizableArray * formats)
{
  g_return_if_fail (formats != nullptr);
  GValue arr = {0,};
  GValue item = {0,};
  g_value_init (&arr, GST_TYPE_LIST);
  g_value_init (&item, G_TYPE_STRING);
  for (int i = 0; i < formats->actualLen; ++i) {
    GstVideoFormat tmp = gst_hdi_video_get_format_from_hdi ((PixelFormat)formats->element[i]);
    if (tmp == GST_VIDEO_FORMAT_UNKNOWN) {
      continue;
    }
    g_value_set_string (&item, gst_video_format_to_string(tmp));
    gst_value_list_append_value (&arr, &item);
  }
  g_value_unset (&item);
  gst_caps_set_value (caps, "format", &arr);
  g_value_unset (&arr);
}