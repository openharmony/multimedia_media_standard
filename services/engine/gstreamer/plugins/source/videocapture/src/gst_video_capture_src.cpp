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

#include "gst_video_capture_src.h"
#include <gst/gst.h>
#include "gst_video_capture_pool.h"
#include "media_errors.h"
#include "display_type.h"
#include "buffer_type_meta.h"

using namespace OHOS::Media;
#define gst_video_capture_src_parent_class parent_class
#define GST_TYPE_VIDEO_CAPTURE_SRC_STREAM_TYPE (gst_video_capture_src_stream_type_get_type())
static GType gst_video_capture_src_stream_type_get_type(void)
{
    static GType surface_video_src_stream_type = 0;
    static const GEnumValue stream_types[] = {
        {VIDEO_STREAM_TYPE_UNKNOWN, "UNKNOWN", "UNKNOWN"},
        {VIDEO_STREAM_TYPE_ES_AVC, "ES_AVC", "ES_AVC"},
        {VIDEO_STREAM_TYPE_YUV_420, "YUV_420", "YUV_420"},
        {0, nullptr, nullptr}
    };
    if (!surface_video_src_stream_type) {
        surface_video_src_stream_type = g_enum_register_static("VideoStreamType", stream_types);
    }
    return surface_video_src_stream_type;
}

static GstStaticPadTemplate gst_video_src_template =
GST_STATIC_PAD_TEMPLATE("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

namespace {
    constexpr VideoStreamType DEFAULT_STREAM_TYPE = VIDEO_STREAM_TYPE_UNKNOWN;
    constexpr guint DEFAULT_FRAME_RATE = 25;
}

GST_DEBUG_CATEGORY_STATIC (gst_video_capture_src_debug);
#define GST_CAT_DEFAULT gst_video_capture_src_debug

enum {
    PROP_0,
    PROP_STREAM_TYPE,
    PROP_SURFACE_WIDTH,
    PROP_SURFACE_HEIGHT,
    PROP_FRAME_RATE,
};

G_DEFINE_TYPE(GstVideoCaptureSrc, gst_video_capture_src, GST_TYPE_SURFACE_SRC);

static void gst_video_capture_src_finalize(GObject *object);
static void gst_video_capture_src_set_stream_type(GstVideoCaptureSrc *src, gint stream_type);
static void gst_video_capture_src_set_caps(GstVideoCaptureSrc *src, uint32_t pixelFormat);
static void gst_video_capture_src_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_video_capture_src_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static GstStateChangeReturn gst_video_capture_src_change_state(GstElement *element, GstStateChange transition);
static void gst_video_caputre_deal_with_pts(GstVideoCaptureSrc *src, GstBuffer *buf);
static GstFlowReturn gst_video_capture_src_fill(GstBaseSrc *src, guint64 offset, guint size, GstBuffer *buf);
static GstBufferPool *gst_video_capture_create_pool();
static void gst_video_capture_src_start(GstVideoCaptureSrc *src);
static void gst_video_capture_src_pause(GstVideoCaptureSrc *src);
static void gst_video_capture_src_resume(GstVideoCaptureSrc *src);
static void gst_video_capture_src_stop(GstVideoCaptureSrc *src);

static void gst_video_capture_src_class_init(GstVideoCaptureSrcClass *klass)
{
    g_return_if_fail(klass != nullptr);
    GObjectClass *gobject_class = reinterpret_cast<GObjectClass*>(klass);
    GstElementClass *gstelement_class = reinterpret_cast<GstElementClass*>(klass);
    GstBaseSrcClass *gstbasesrc_class = reinterpret_cast<GstBaseSrcClass*>(klass);
    GstMemSrcClass *gstmemsrc_class = reinterpret_cast<GstMemSrcClass *>(klass);
    GST_DEBUG_CATEGORY_INIT(gst_video_capture_src_debug, "videocapturesrc", 0, "video capture src base class");

    gobject_class->finalize = gst_video_capture_src_finalize;
    gobject_class->set_property = gst_video_capture_src_set_property;
    gobject_class->get_property = gst_video_capture_src_get_property;

    g_object_class_install_property(gobject_class, PROP_STREAM_TYPE,
        g_param_spec_enum("stream-type", "Stream type",
            "Stream type", GST_TYPE_VIDEO_CAPTURE_SRC_STREAM_TYPE, DEFAULT_STREAM_TYPE,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_FRAME_RATE,
        g_param_spec_uint("frame-rate", "Frame rate",
            "recorder frame rate", 0, G_MAXUINT32, DEFAULT_FRAME_RATE,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_SURFACE_WIDTH,
        g_param_spec_uint("surface-width", "Surface width",
            "Surface width", 0, G_MAXUINT32, 0,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_SURFACE_HEIGHT,
        g_param_spec_uint("surface-height", "Surface height",
            "Surface width", 0, G_MAXUINT32, 0,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    gstelement_class->change_state = gst_video_capture_src_change_state;
    gstbasesrc_class->fill = gst_video_capture_src_fill;
    gstmemsrc_class->create_pool = gst_video_capture_create_pool;

    gst_element_class_set_static_metadata(gstelement_class,
        "video capture source", "Source/Video",
        "Retrieve video frame from surface buffer queue", "OpenHarmony");

    gst_element_class_add_static_pad_template(gstelement_class, &gst_video_src_template);
}

static void gst_video_capture_src_init(GstVideoCaptureSrc *videocapturesrc)
{
    g_return_if_fail(videocapturesrc != nullptr);

    // private param
    videocapturesrc->video_frame_rate = DEFAULT_FRAME_RATE;
    videocapturesrc->src_caps = nullptr;
    videocapturesrc->is_first_buffer = true;
    videocapturesrc->video_width = 0;
    videocapturesrc->video_height = 0;
    videocapturesrc->cur_state = RECORDER_INITIALIZED;
    videocapturesrc->min_interval = 0;

    videocapturesrc->last_timestamp = 0;
    videocapturesrc->paused_time = -1;
    videocapturesrc->resume_time = 0;
    videocapturesrc->paused_count = 0;
    videocapturesrc->persist_time = 0;
    videocapturesrc->total_pause_time = 0;
}

static void gst_video_capture_src_finalize(GObject *object)
{
    g_return_if_fail(object != nullptr);
    GstVideoCaptureSrc *src = GST_VIDEO_CAPTURE_SRC(object);
    g_return_if_fail(src != nullptr);

    if (src->src_caps != nullptr) {
        gst_caps_unref(src->src_caps);
        src->src_caps = nullptr;
    }
}

static void gst_video_capture_src_set_stream_type(GstVideoCaptureSrc *src, gint stream_type)
{
    switch (stream_type) {
        case VideoStreamType::VIDEO_STREAM_TYPE_ES_AVC:
            src->stream_type = VIDEO_STREAM_TYPE_ES_AVC;
            break;
        case VideoStreamType::VIDEO_STREAM_TYPE_YUV_420:
            src->stream_type = VIDEO_STREAM_TYPE_YUV_420;
            break;
        default:
            return;
    }
}

static void gst_video_capture_src_set_caps(GstVideoCaptureSrc *src, uint32_t pixelFormat)
{
    g_return_if_fail(src != nullptr);

    if (src->src_caps != nullptr) {
        gst_caps_unref(src->src_caps);
    }

    std::string format = "";

    switch (pixelFormat) {
        case PIXEL_FMT_YCRCB_420_SP:
            GST_INFO("input pixel foramt is nv21");
            format = "NV21";
            break;
        case PIXEL_FMT_YCBCR_420_P:
            GST_INFO("input pixel foramt is I420");
            format = "I420";
            break;
        case PIXEL_FMT_YCBCR_420_SP:
            GST_INFO("input pixel foramt is nv12");
            format = "NV12";
            break;
        default:
            break;
    }

    if (src->stream_type == VIDEO_STREAM_TYPE_ES_AVC) {
        src->src_caps = gst_caps_new_simple("video/x-h264",
            "framerate", GST_TYPE_FRACTION, src->video_frame_rate, 1,
            "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
            "stream-format", G_TYPE_STRING, "avc",
            nullptr);
    } else {
        src->src_caps = gst_caps_new_simple("video/x-raw",
            "format", G_TYPE_STRING, format.c_str(),
            "framerate", GST_TYPE_FRACTION, src->video_frame_rate, 1,
            "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
            "width", G_TYPE_INT, src->video_width,
            "height", G_TYPE_INT, src->video_height,
            nullptr);
    }
    gst_base_src_set_caps(GST_BASE_SRC(src), src->src_caps);
}

static void gst_video_capture_src_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    g_return_if_fail(object != nullptr);
    g_return_if_fail(value != nullptr);
    GstVideoCaptureSrc *src = GST_VIDEO_CAPTURE_SRC(object);
    (void)pspec;
    switch (prop_id) {
        case PROP_STREAM_TYPE:
            gst_video_capture_src_set_stream_type(src, g_value_get_enum(value));
            break;
        case PROP_SURFACE_WIDTH:
            src->video_width = g_value_get_uint(value);
            break;
        case PROP_SURFACE_HEIGHT:
            src->video_height = g_value_get_uint(value);
            break;
        case PROP_FRAME_RATE:
            src->video_frame_rate = g_value_get_uint(value);
            src->min_interval = 1000000000 / src->video_frame_rate; // 1s = 1000000000ns
            break;
        default:
            break;
    }
}

static void gst_video_capture_src_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    g_return_if_fail(object != nullptr);
    g_return_if_fail(value != nullptr);
    GstVideoCaptureSrc *src = GST_VIDEO_CAPTURE_SRC(object);
    (void)pspec;
    switch (prop_id) {
        case PROP_STREAM_TYPE:
            g_value_set_enum(value, src->stream_type);
            break;
        case PROP_SURFACE_WIDTH:
            src->video_width = g_value_get_uint(value);
            break;
        case PROP_SURFACE_HEIGHT:
            src->video_height = g_value_get_uint(value);
            break;
        case PROP_FRAME_RATE:
            g_value_set_uint(value, src->video_frame_rate);
            break;
        default:
            break;
    }
}

static GstStateChangeReturn gst_video_capture_src_change_state(GstElement *element, GstStateChange transition)
{
    GstVideoCaptureSrc *capturesrc = GST_VIDEO_CAPTURE_SRC(element);
    g_return_val_if_fail(capturesrc != nullptr, GST_STATE_CHANGE_FAILURE);

    switch (transition) {
        case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
            if (capturesrc->cur_state == RECORDER_INITIALIZED) {
                gst_video_capture_src_start(capturesrc);
            } else {
                gst_video_capture_src_resume(capturesrc);
            }
            break;
        default:
            break;
    }
    GstStateChangeReturn ret = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);

    switch (transition) {
        case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
            gst_video_capture_src_pause(capturesrc);
            break;
        case GST_STATE_CHANGE_PAUSED_TO_READY:
            gst_video_capture_src_stop(capturesrc);
            break;
        default:
            break;
    }

    return ret;
}

static GstBufferPool *gst_video_capture_create_pool()
{
    return gst_video_capture_pool_new();
}

static void gst_video_caputre_deal_with_pts(GstVideoCaptureSrc *src, GstBuffer *buf)
{
    g_return_if_fail(buf != nullptr);

    gint64 timestamp = GST_BUFFER_PTS (buf);
    GST_DEBUG_OBJECT(src, "video capture buffer size is: %" G_GSIZE_FORMAT ", pts: % " G_GINT64_FORMAT "",
        gst_buffer_get_size(buf), timestamp);
    GST_INFO_OBJECT(src, "videoCapturer timestamp has increased: % " G_GINT64_FORMAT "",
        timestamp - src->last_timestamp);

    if (src->cur_state == RECORDER_RESUME && src->paused_time == -1) {
        src->paused_time = src->last_timestamp;
        GST_INFO_OBJECT(src, "video pause timestamp % " G_GINT64_FORMAT "", src->paused_time);
    }

    if (src->cur_state == RECORDER_PAUSED) {
        src->paused_time = timestamp;
        GST_INFO_OBJECT(src, "video pause timestamp % " G_GINT64_FORMAT "", src->paused_time);
    }

    if (src->cur_state == RECORDER_RESUME) {
        src->cur_state = RECORDER_RUNNING;
        src->resume_time = timestamp;
        src->persist_time = fabs(src->resume_time - src->paused_time) - src->min_interval;
        GST_INFO_OBJECT(src, "video resume timestamp % " G_GINT64_FORMAT "", src->resume_time);
        src->paused_time = -1; // reset pause time
        src->total_pause_time += src->persist_time;
        GST_INFO_OBJECT(src, "video has %d times pause, total PauseTime: % " G_GINT64_FORMAT "",
            src->paused_count, src->total_pause_time);
    }

    src->last_timestamp = timestamp; // updata last_timestamp
    GST_BUFFER_PTS (buf) = timestamp - src->total_pause_time; // running state timestamp to encoder is upwith pause
}

static GstFlowReturn gst_video_capture_src_fill(GstBaseSrc *src, guint64 offset, guint size, GstBuffer *buf)
{
    g_return_val_if_fail(src != nullptr, GST_FLOW_ERROR);
    GstVideoCaptureSrc *capturesrc = GST_VIDEO_CAPTURE_SRC(src);
    (void)offset;
    (void)size;
    GstBufferTypeMeta *meta = gst_buffer_get_buffer_type_meta(buf);
    g_return_val_if_fail(meta != nullptr, GST_FLOW_ERROR);
    if (meta != nullptr && (meta->bufferFlag & BUFFER_FLAG_EOS)) {
        return GST_FLOW_EOS;
    }

    // set caps, called at first frame
    if (capturesrc->is_first_buffer) {
        gst_video_capture_src_set_caps(capturesrc, meta->pixelFormat);
        capturesrc->is_first_buffer = false;
    }

    if (capturesrc->stream_type == VIDEO_STREAM_TYPE_ES_AVC) {
        gst_buffer_resize(buf, 0, meta->length); // es stream gstbuffer size is not equle to surface buffer size
    }

    // do with pts
    gst_video_caputre_deal_with_pts(capturesrc, buf);

    return GST_FLOW_OK;
}

static void gst_video_capture_src_start(GstVideoCaptureSrc *src)
{
    g_return_if_fail(src != nullptr);
    src->cur_state = RECORDER_RUNNING;
}

static void gst_video_capture_src_pause(GstVideoCaptureSrc *src)
{
    g_return_if_fail(src != nullptr);
    GstSurfaceSrc *surfacesrc = GST_SURFACE_SRC(src);
    g_return_if_fail(surfacesrc->pool != nullptr);

    g_object_set(surfacesrc->pool, "suspend", TRUE, nullptr);

    src->cur_state = RECORDER_PAUSED;
    src->paused_count++;

    GstBufferPool *bufferpool = GST_BUFFER_POOL(surfacesrc->pool);
    gst_buffer_pool_set_flushing(bufferpool, TRUE);
    gst_buffer_pool_set_flushing(bufferpool, FALSE);
}

static void gst_video_capture_src_resume(GstVideoCaptureSrc *src)
{
    g_return_if_fail(src != nullptr);
    GstSurfaceSrc *surfacesrc = GST_SURFACE_SRC(src);
    g_return_if_fail(surfacesrc->pool != nullptr);

    g_object_set(surfacesrc->pool, "suspend", FALSE, nullptr);

    src->cur_state = RECORDER_RESUME;
}

static void gst_video_capture_src_stop(GstVideoCaptureSrc *src)
{
    g_return_if_fail(src != nullptr);

    src->cur_state = RECORDER_STOP;
}
