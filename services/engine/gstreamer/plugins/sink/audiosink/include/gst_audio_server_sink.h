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


#ifndef GST_AUDIO_SERVER_SINK_H
#define GST_AUDIO_SERVER_SINK_H

#include <mutex>
#include <memory>
#include <gst/base/gstbasesink.h>
#include "audio_sink.h"
#include "common_utils.h"

G_BEGIN_DECLS

#define GST_TYPE_AUDIO_SERVER_SINK \
    (gst_audio_server_sink_get_type())
#define GST_AUDIO_SERVER_SINK(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_AUDIO_SERVER_SINK, GstAudioServerSink))
#define GST_AUDIO_SERVER_SINK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_AUDIO_SERVER_SINK, GstAudioServerSinkClass))
#define GST_IS_AUDIO_SERVER_SINK(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_AUDIO_SERVER_SINK))
#define GST_IS_AUDIO_SERVER_SINK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_AUDIO_SERVER_SINK))
#define GST_AUDIO_SERVER_SINK_CAST(obj) ((GstAudioServerSink *)(obj))

struct _GstAudioServerSink {
    GstBaseSink parent;

    /* private */
    std::unique_ptr<OHOS::Media::AudioSink> audio_sink;
    guint bits_per_sample;
    guint channels;
    guint sample_rate;
    gint appuid;
    gint apppid;
    gfloat volume;
    gfloat max_volume;
    gfloat min_volume;
    guint min_buffer_size;
    guint min_frame_count;
    gboolean frame_after_segment;
    GMutex render_lock;
    std::mutex mutex_;
    GstBuffer *pause_cache_buffer;
    guint renderer_desc;
    gint renderer_flag;
    GstClockTime last_render_pts;
    gboolean enable_opt_render_delay;
    GstClockTimeDiff last_running_time_diff;
};

struct _GstAudioServerSinkClass {
    GstBaseSinkClass parent_class;
};

using GstAudioServerSink = struct _GstAudioServerSink;
using GstAudioServerSinkClass = struct _GstAudioServerSinkClass;

G_GNUC_INTERNAL GType gst_audio_server_sink_get_type(void);

G_END_DECLS

#endif // GST_AUDIO_SERVER_SINK_H
