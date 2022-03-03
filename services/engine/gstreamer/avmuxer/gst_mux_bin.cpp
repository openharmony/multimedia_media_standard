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
#include "gst_mux_bin.h"
#include <fcntl.h>
#include <unistd.h>
#include "gstappsrc.h"
#include "gstbaseparse.h"

enum {
    PROP_0,
    PROP_PATH,
    PROP_FD,
    PROP_MUX,
    PROP_DEGREES,
    PROP_LATITUDE,
    PROP_LONGITUDE,
};

#define gst_mux_bin_parent_class parent_class
G_DEFINE_TYPE(GstMuxBin, gst_mux_bin, GST_TYPE_PIPELINE);

static void gst_mux_bin_finalize(GObject *object);
static void gst_mux_bin_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *param_spec);
static void gst_mux_bin_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *param_spec);
static GstStateChangeReturn gst_mux_bin_change_state(GstElement *element, GstStateChange transition);

void gst_mux_bin_add_track(GstMuxBin *mux_bin, const char *srcName, const char *parseName, bool isVideo)
{
    g_return_if_fail(mux_bin != nullptr);
    g_return_if_fail(srcName != nullptr);
    g_return_if_fail(parseName != nullptr);
    GstTrackInfo *info = g_new(GstTrackInfo, 1);
    info->srcName_ = g_strdup((char *)srcName);
    info->parseName_ = g_strdup((char *)parseName);
    if (isVideo) {
        mux_bin->videoSrcList_ = g_slist_append(mux_bin->videoSrcList_, info);
    } else {
        mux_bin->audioSrcList_ = g_slist_append(mux_bin->audioSrcList_, info);
    }
}

static void gst_mux_bin_class_init(GstMuxBinClass *klass)
{
    g_return_if_fail(klass != nullptr);
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstElementClass *gstelement_class = GST_ELEMENT_CLASS(klass);
    GstBinClass *gstbin_class = GST_BIN_CLASS(klass);

    g_return_if_fail(gobject_class != nullptr && gstelement_class != nullptr && gstbin_class != nullptr);

    gobject_class->finalize = gst_mux_bin_finalize;
    gobject_class->set_property = gst_mux_bin_set_property;
    gobject_class->get_property = gst_mux_bin_get_property;

    g_object_class_install_property(gobject_class, PROP_PATH,
        g_param_spec_string("path", "Path", "Path of the output file",
            nullptr, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_FD,
        g_param_spec_int("fd", "FD", "fd of the output file",
            0, G_MAXINT32, -1, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_MUX,
        g_param_spec_string("mux", "Mux", "type of the mux",
            nullptr, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_DEGREES,
        g_param_spec_int("degrees", "Degrees", "rotation angle of the output file",
            0, G_MAXINT32, 0, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_LATITUDE,
        g_param_spec_int("latitude", "Latitude", "latitude of the output file",
            G_MININT32, G_MAXINT32, 0, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_LONGITUDE,
        g_param_spec_int("longitude", "Longitude", "longitude of the output file",
            G_MININT32, G_MAXINT32, 0, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    gst_element_class_set_static_metadata(gstelement_class,
        "Mux Bin", "Generic/Bin/Mux",
        "Auto construct mux pipeline", "OpenHarmony");

    gstelement_class->change_state = gst_mux_bin_change_state;
}

static void gst_mux_bin_init(GstMuxBin *mux_bin)
{
    g_return_if_fail(mux_bin != nullptr);
    GST_INFO_OBJECT(mux_bin, "gst_mux_bin_init");
    mux_bin->videoSrcList_ = nullptr;
    mux_bin->audioSrcList_ = nullptr;
    mux_bin->splitMuxSink_ = nullptr;
    mux_bin->path_ = nullptr;
    mux_bin->outFd_ = -1;
    mux_bin->mux_ = nullptr;
    mux_bin->degrees_ = 0;
    mux_bin->latitude_ = 0;
    mux_bin->longitude_ = 0;
}

static void gst_mux_bin_finalize(GObject *object)
{
    g_return_if_fail(object != nullptr);
    GstMuxBin *mux_bin = GST_MUX_BIN(object);
    GST_INFO_OBJECT(mux_bin, "gst_mux_bin_finalize");

    if (mux_bin->outFd_ > 0) {
        (void)::close(mux_bin->outFd_);
        mux_bin->outFd_ = -1;
    }
    if (mux_bin->path_ != nullptr) {
        g_free(mux_bin->path_);
        mux_bin->path_ = nullptr;
    }
    if (mux_bin->mux_ != nullptr) {
        g_free(mux_bin->mux_);
        mux_bin->mux_ = nullptr;
    }
    GSList *iter = mux_bin->videoSrcList_;
    while (iter != nullptr && iter->data != nullptr) {
        g_free(((GstTrackInfo *)(iter->data))->srcName_);
        g_free(((GstTrackInfo *)(iter->data))->parseName_);
        ((GstTrackInfo *)(iter->data))->srcName_ = nullptr;
        ((GstTrackInfo *)(iter->data))->parseName_ = nullptr;
        iter->data = nullptr;
        iter = iter->next;
    }

    iter = mux_bin->audioSrcList_;
    while (iter != nullptr && iter->data != nullptr) {
        g_free(((GstTrackInfo *)(iter->data))->srcName_);
        g_free(((GstTrackInfo *)(iter->data))->parseName_);
        ((GstTrackInfo *)(iter->data))->srcName_ = nullptr;
        ((GstTrackInfo *)(iter->data))->parseName_ = nullptr;
        iter->data = nullptr;
        iter = iter->next;
    }

    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_mux_bin_set_property(GObject *object, guint prop_id,
    const GValue *value, GParamSpec *param_spec)
{
    g_return_if_fail(object != nullptr);
    g_return_if_fail(value != nullptr);
    (void)param_spec;
    GstMuxBin *mux_bin = GST_MUX_BIN(object);
    switch (prop_id) {
        case PROP_PATH:
            mux_bin->path_ = g_strdup(g_value_get_string(value));
            break;
        case PROP_FD:
            mux_bin->outFd_ = g_value_get_int(value);
            break;
        case PROP_MUX:
            mux_bin->mux_ = g_strdup(g_value_get_string(value));
            break;
        case PROP_DEGREES:
            mux_bin->degrees_ = g_value_get_int(value);
            break;
        case PROP_LATITUDE:
            mux_bin->latitude_ = g_value_get_int(value);
            break;
        case PROP_LONGITUDE:
            mux_bin->longitude_ = g_value_get_int(value);
            break;
        default:
            break;
    }
}

static void gst_mux_bin_get_property(GObject *object, guint prop_id,
    GValue *value, GParamSpec *param_spec)
{
    g_return_if_fail(object != nullptr);
    g_return_if_fail(value != nullptr);
    (void)param_spec;
    GstMuxBin *mux_bin = GST_MUX_BIN(object);
    switch (prop_id) {
        case PROP_PATH:
            g_value_set_string(value, mux_bin->path_);
            break;
        case PROP_FD:
            g_value_set_int(value, mux_bin->outFd_);
            break;
        case PROP_MUX:
            g_value_set_string(value, mux_bin->mux_);
            break;
        case PROP_DEGREES:
            g_value_set_int(value, mux_bin->degrees_);
            break;
        case PROP_LATITUDE:
            g_value_set_int(value, mux_bin->latitude_);
            break;
        case PROP_LONGITUDE:
            g_value_set_int(value, mux_bin->longitude_);
            break;
        default:
            break;
    }
}

static bool create_splitmuxsink(GstMuxBin *mux_bin)
{
    GST_INFO_OBJECT(mux_bin, "create_splitmuxsink");
    g_return_val_if_fail(mux_bin != nullptr, false);
    g_return_val_if_fail(mux_bin->path_ != nullptr || mux_bin->outFd_ >= 0, false);
    g_return_val_if_fail(mux_bin->mux_ != nullptr, false);

    mux_bin->splitMuxSink_ = gst_element_factory_make("splitmuxsink", "splitmuxsink");
    g_return_val_if_fail(mux_bin->splitMuxSink_ != nullptr, false);

    GstElement *fdsink = gst_element_factory_make("fdsink", "fdsink");
    g_return_val_if_fail(fdsink != nullptr, false);

    if (mux_bin->outFd_ < 0 && mux_bin->path_ != nullptr) {
        mux_bin->outFd_ = open(mux_bin->path_, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
        if (mux_bin->outFd_ < 0) {
            GST_ERROR_OBJECT(mux_bin, "Open file failed! filePath is: %s", mux_bin->path_);
            return GST_STATE_CHANGE_FAILURE;
        }
    }

    g_object_set(fdsink, "fd", mux_bin->outFd_, nullptr);
    g_object_set(mux_bin->splitMuxSink_, "sink", fdsink, nullptr);

    GstElement *qtmux = gst_element_factory_make(mux_bin->mux_, mux_bin->mux_);
    g_return_val_if_fail(qtmux != nullptr, false);
    g_object_set(qtmux, "orientation-hint", mux_bin->degrees_, "set-latitude", mux_bin->latitude_,
        "set-longitude", mux_bin->longitude_, nullptr);
    g_object_set(mux_bin->splitMuxSink_, "muxer", qtmux, nullptr);

    return true;
}

static GstElement *create_parse(GstMuxBin *mux_bin, const char* parseName)
{
    GST_INFO_OBJECT(mux_bin, "create_parse");
    g_return_val_if_fail(mux_bin != nullptr, nullptr);
    GstElement *parse = nullptr;
    g_return_val_if_fail(mux_bin != nullptr, nullptr);
    if (strcmp(parseName, "h264parse") == 0) {
        parse = gst_element_factory_make("h264parse", "h264parse");
        g_return_val_if_fail(parse != nullptr, nullptr);
    } else if (strcmp(parseName, "mpeg4videoparse") == 0) {
        parse = gst_element_factory_make("mpeg4videoparse", "mpeg4parse");
        g_return_val_if_fail(parse != nullptr, nullptr);
        g_object_set(parse, "config-interval", -1, "drop", false, nullptr);
    } else if (strcmp(parseName, "aacparse") == 0) {
        parse = gst_element_factory_make("aacparse", "aacparse");
        g_return_val_if_fail(parse != nullptr, nullptr);
    } else {
        GST_ERROR_OBJECT(mux_bin, "Invalid videoParse");
    }

    return parse;
}

static bool create_src(GstMuxBin *mux_bin, gboolean isVideo)
{
    GST_INFO_OBJECT(mux_bin, "create_src");
    g_return_val_if_fail(mux_bin != nullptr, false);
    GSList *iter = nullptr;
    if (isVideo) {
        iter = mux_bin->videoSrcList_;
    } else {
        iter = mux_bin->audioSrcList_;
    }
    while (iter != nullptr) {
        GstElement *appSrc = gst_element_factory_make("appsrc", ((GstTrackInfo *)(iter->data))->srcName_);
        g_return_val_if_fail(appSrc != nullptr, false);
        g_object_set(appSrc, "is-live", true, "format", GST_FORMAT_TIME, nullptr);
        ((GstTrackInfo *)(iter->data))->src_ = appSrc;
        iter = iter->next;
    }

    return true;
}

static bool create_element(GstMuxBin *mux_bin)
{
    GST_INFO_OBJECT(mux_bin, "create_element");
    g_return_val_if_fail(mux_bin != nullptr, false);
    if (!create_splitmuxsink(mux_bin)) {
        GST_ERROR_OBJECT(mux_bin, "Failed to call create_splitmuxsink");
        return false;
    }

    if (!create_src(mux_bin, TRUE)) {
        GST_ERROR_OBJECT(mux_bin, "Failed to call create_video_src");
        return false;
    }

    if (!create_src(mux_bin, FALSE)) {
        GST_ERROR_OBJECT(mux_bin, "Failed to call create_audio_src");
        return false;
    }

    return true;
}

static bool add_element_to_bin(GstMuxBin *mux_bin)
{
    GST_INFO_OBJECT(mux_bin, "add_element_to_bin");
    g_return_val_if_fail(mux_bin != nullptr, false);
    g_return_val_if_fail(mux_bin->splitMuxSink_ != nullptr, false);
    bool ret;
    GSList *iter = mux_bin->videoSrcList_;
    while (iter != nullptr) {
        ret = gst_bin_add(GST_BIN(mux_bin), ((GstTrackInfo *)(iter->data))->src_);
        g_return_val_if_fail(ret == TRUE, false);
        iter = iter->next;
    }
    iter = mux_bin->audioSrcList_;
    while (iter != nullptr) {
        ret = gst_bin_add(GST_BIN(mux_bin), ((GstTrackInfo *)(iter->data))->src_);
        g_return_val_if_fail(ret == TRUE, false);
        iter = iter->next;
    }
    ret = gst_bin_add(GST_BIN(mux_bin), mux_bin->splitMuxSink_);
    g_return_val_if_fail(ret == TRUE, false);

    return true;
}

static bool connect_parse(GstMuxBin *mux_bin, GstElement *parse, GstPad *upstream_pad, GstPad *downstream_pad)
{
    GST_INFO_OBJECT(mux_bin, "connect_parse");
    g_return_val_if_fail(mux_bin != nullptr, false);
    g_return_val_if_fail(parse != nullptr, false);
    g_return_val_if_fail(upstream_pad != nullptr, false);
    g_return_val_if_fail(downstream_pad != nullptr, false);
    GstPad *parse_sink_pad = gst_element_get_static_pad(parse, "sink");
    if (gst_pad_link(upstream_pad, parse_sink_pad) != GST_PAD_LINK_OK) {
        GST_ERROR_OBJECT(mux_bin, "Failed to link src_src_pad and parse_sink_pad");
        return false;
    }
    GstPad *parse_src_pad = gst_element_get_static_pad(parse, "src");
    if (gst_pad_link(parse_src_pad, downstream_pad) != GST_PAD_LINK_OK) {
        GST_ERROR_OBJECT(mux_bin, "Failed to link parse_src_pad and split_mux_sink_sink_pad");
        return false;
    }
    
    return true;
}

static bool connect_element(GstMuxBin *mux_bin, gboolean isVideo)
{
    GST_INFO_OBJECT(mux_bin, "connect_element");
    g_return_val_if_fail(mux_bin != nullptr, false);
    g_return_val_if_fail(mux_bin->splitMuxSink_ != nullptr, false);
    bool ret;
    GSList *iter = isVideo ? mux_bin->videoSrcList_ : mux_bin->audioSrcList_;
    while (iter != nullptr) {
        GstPad *src_src_pad = gst_element_get_static_pad(((GstTrackInfo *)(iter->data))->src_, "src");
        GstPad *split_mux_sink_sink_pad = nullptr;
        if (isVideo) {
            split_mux_sink_sink_pad = gst_element_get_request_pad(mux_bin->splitMuxSink_, "video");
        } else {
            split_mux_sink_sink_pad = gst_element_get_request_pad(mux_bin->splitMuxSink_, "audio_%u");
        }
        if (((GstTrackInfo *)(iter->data))->parseName_ != nullptr) {
            GstElement *parse = create_parse(mux_bin, ((GstTrackInfo *)(iter->data))->parseName_);
            g_return_val_if_fail(parse != nullptr, false);
            ret = gst_bin_add(GST_BIN(mux_bin), parse);
            g_return_val_if_fail(ret == TRUE, false);
            if (!connect_parse(mux_bin, parse, src_src_pad, split_mux_sink_sink_pad)) {
                GST_ERROR_OBJECT(mux_bin, "Failed to call connect_parse");
                return false;
            }
        } else {
            if (gst_pad_link(src_src_pad, split_mux_sink_sink_pad) != GST_PAD_LINK_OK) {
                GST_ERROR_OBJECT(mux_bin, "Failed to link src_src_pad and split_mux_sink_sink_pad");
                return false;
            }
        }
        iter = iter->next;
    }
    
    return true;
}

static GstStateChangeReturn gst_mux_bin_change_state(GstElement *element, GstStateChange transition)
{
    g_return_val_if_fail(element != nullptr, GST_STATE_CHANGE_FAILURE);
    GstMuxBin *mux_bin = GST_MUX_BIN(element);
    GST_INFO_OBJECT(mux_bin, "gst_mux_bin_change_state");

    switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
            if (!create_element(mux_bin)) {
                GST_ERROR_OBJECT(mux_bin, "Failed to create element");
                return GST_STATE_CHANGE_FAILURE;
            }
            if (!add_element_to_bin(mux_bin)) {
                GST_ERROR_OBJECT(mux_bin, "Failed to add element to bin");
                return GST_STATE_CHANGE_FAILURE;
            }
            break;
        case GST_STATE_CHANGE_READY_TO_PAUSED:
            if (!connect_element(mux_bin, TRUE)) {
                GST_ERROR_OBJECT(mux_bin, "Failed to connect element");
                return GST_STATE_CHANGE_FAILURE;
            }
            if (!connect_element(mux_bin, FALSE)) {
                GST_ERROR_OBJECT(mux_bin, "Failed to connect element");
                return GST_STATE_CHANGE_FAILURE;
            }
            break;
        default:
            break;
    }
    return GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
}

static gboolean plugin_init(GstPlugin *plugin)
{
    g_return_val_if_fail(plugin != nullptr, FALSE);
    return gst_element_register(plugin, "muxbin", GST_RANK_PRIMARY, GST_TYPE_MUX_BIN);
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    _avmuxer_bin,
    "GStreamer Mux Bin",
    plugin_init,
    PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)