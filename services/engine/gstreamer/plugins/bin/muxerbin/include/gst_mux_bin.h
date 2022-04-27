/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef GST_MUX_BIN_H
#define GST_MUX_BIN_H

#include <gst/gst.h>

G_BEGIN_DECLS

GType gst_mux_bin_get_type(void);
#define GST_TYPE_MUX_BIN \
    (gst_mux_bin_get_type())
#define GST_MUX_BIN(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_MUX_BIN, GstMuxBin))
#define GST_MUX_BIN_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_MUX_BIN, GstMuxBinClass))
#define GST_IS_MUX_BIN(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_MUX_BIN))
#define GST_IS_MUX_BIN_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_MUX_BIN))
#define GST_MUX_BIN_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_MUX_BIN, GstMuxBinClass))

struct _GstTrackInfo {
    gchar *srcName_;
    gchar *parseName_;
    GstElement *src_;
    GstElement *parse_;
};

struct _GstMuxBin {
    GstPipeline parent_;

    /* private */
    GSList *video_src_list;
    GSList *audio_src_list;
    GstElement *split_mux_sink;

    gint out_fd;
    gchar *mux;
    gint rotation;
    gint latitude;
    gint longitude;
};

struct _GstMuxBinClass {
    GstPipelineClass parent_class;
    void (*add_track)(_GstMuxBin *mux_bin, const char *src_name, const char *parse_name, int32_t track_type);
};

using GstMuxBin = struct _GstMuxBin;
using GstMuxBinClass = struct _GstMuxBinClass;
using GstTrackInfo = struct _GstTrackInfo;

G_GNUC_INTERNAL GType gst_mux_bin_get_type(void);

G_END_DECLS
#endif // GST_MUX_BIN_H