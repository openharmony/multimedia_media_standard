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
#include "gst_codec_bin.h"
#include <gst/gst.h>
#include "dumper.h"

enum {
    PROP_0,
    PROP_TYPE,
    PROP_USE_SOFTWARE,
    PROP_CODER_NAME,
    PROP_SRC,
    PROP_SINK,
    PROP_SRC_CONVERT,
    PROP_SINK_CONVERT,
    PROP_PARSER,
    PROP_FORCE_I_FRAME,
    PROP_USE_SURFACE_INPUT
};

#define gst_codec_bin_parent_class parent_class
G_DEFINE_TYPE(GstCodecBin, gst_codec_bin, GST_TYPE_BIN);

static void gst_codec_bin_finalize(GObject *object);
static void gst_codec_bin_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *param_spec);
static void gst_codec_bin_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *param_spec);
static GstStateChangeReturn gst_codec_bin_change_state(GstElement *element, GstStateChange transition);

#define GST_TYPE_CODEC_BIN_TYPE (gst_codec_bin_type_get_type())
static GType gst_codec_bin_type_get_type(void)
{
    static GType codec_bin_type = 0;
    static const GEnumValue bin_types[] = {
        {CODEC_BIN_TYPE_VIDEO_DECODER, "video decoder", "video decoder"},
        {CODEC_BIN_TYPE_VIDEO_ENCODER, "video encoder", "video encoder"},
        {CODEC_BIN_TYPE_AUDIO_DECODER, "audio decoder", "audio decoder"},
        {CODEC_BIN_TYPE_AUDIO_ENCODER, "audio encoder", "audio encoder"},
        {CODEC_BIN_TYPE_UNKNOWN, "unknown", "unknown"},
        {0, nullptr, nullptr}
    };
    if (!codec_bin_type) {
        codec_bin_type = g_enum_register_static("CodecBinType", bin_types);
    }
    return codec_bin_type;
}

static void gst_codec_bin_class_init(GstCodecBinClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstElementClass *gstelement_class = GST_ELEMENT_CLASS(klass);
    GstBinClass *gstbin_class = GST_BIN_CLASS(klass);

    g_return_if_fail(gobject_class != nullptr && gstelement_class != nullptr && gstbin_class != nullptr);

    gobject_class->finalize = gst_codec_bin_finalize;
    gobject_class->set_property = gst_codec_bin_set_property;
    gobject_class->get_property = gst_codec_bin_get_property;

    g_object_class_install_property(gobject_class, PROP_TYPE,
        g_param_spec_enum("type", "CodecBin type", "Type of CodecBin",
            GST_TYPE_CODEC_BIN_TYPE, CODEC_BIN_TYPE_UNKNOWN,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_USE_SOFTWARE,
        g_param_spec_boolean("use-software", "Use software coder plugin", "Use software coder plugin",
            TRUE, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_CODER_NAME,
        g_param_spec_string("coder-name", "Name of coder", "Name of the coder plugin",
            nullptr, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_SRC,
        g_param_spec_pointer("src", "Src plugin-in", "Src plugin-in",
            (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_SINK,
        g_param_spec_pointer("sink", "Sink plugin-in", "Sink plugin-in",
            (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_SRC_CONVERT,
        g_param_spec_boolean("src-convert", "Need src convert", "Need convert for input data",
            FALSE, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_SINK_CONVERT,
        g_param_spec_boolean("sink-convert", "Need sink convert", "Need convert for output data",
            FALSE, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_PARSER,
        g_param_spec_boolean("parser", "Need parser", "Need parser",
            FALSE, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_FORCE_I_FRAME,
        g_param_spec_int("req-i-frame", "Request I frame", "Request I frame for video encoder",
            -1, 1, 0, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_USE_SURFACE_INPUT,
        g_param_spec_boolean("use-surface-input", "use surface input", "The source is surface",
            FALSE, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));
    gst_element_class_set_static_metadata(gstelement_class,
        "Codec Bin", "Bin/Decoder&Encoder",
        "Auto construct codec pipeline", "OpenHarmony");

    gstelement_class->change_state = gst_codec_bin_change_state;
}

static void gst_codec_bin_init(GstCodecBin *bin)
{
    g_return_if_fail(bin != nullptr);
    GST_INFO_OBJECT(bin, "gst_codec_bin_init");
    bin->src = nullptr;
    bin->parser = nullptr;
    bin->src_convert = nullptr;
    bin->coder = nullptr;
    bin->sink_convert = nullptr;
    bin->sink = nullptr;
    bin->type = CODEC_BIN_TYPE_UNKNOWN;
    bin->is_start = FALSE;
    bin->use_software = FALSE;
    bin->coder_name = nullptr;
    bin->need_src_convert = FALSE;
    bin->need_sink_convert = FALSE;
    bin->need_parser = FALSE;
	bin->is_input_surface = FALSE;
}

static void gst_codec_bin_finalize(GObject *object)
{
    GstCodecBin *bin = GST_CODEC_BIN(object);
    GST_INFO_OBJECT(bin, "gst_codec_bin_finalize");
    g_return_if_fail(bin != nullptr);
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_codec_bin_set_property(GObject *object, guint prop_id,
    const GValue *value, GParamSpec *param_spec)
{
    (void)param_spec;
    GstCodecBin *bin = GST_CODEC_BIN(object);
    g_return_if_fail(bin != nullptr);
    switch (prop_id) {
        case PROP_TYPE:
            bin->type = static_cast<CodecBinType>(g_value_get_enum(value));
            break;
        case PROP_USE_SOFTWARE:
            bin->use_software = g_value_get_boolean(value);
            break;
        case PROP_CODER_NAME:
            bin->coder_name = g_strdup(g_value_get_string(value));
            break;
        case PROP_SRC:
            bin->src = static_cast<GstElement *>(g_value_get_pointer(value));
            GST_INFO_OBJECT(bin, "Set src element");
            break;
        case PROP_SINK:
            bin->sink = static_cast<GstElement *>(g_value_get_pointer(value));
            GST_INFO_OBJECT(bin, "Set sink element");
            break;
        case PROP_SRC_CONVERT:
            bin->need_src_convert = g_value_get_boolean(value);
            break;
        case PROP_SINK_CONVERT:
            bin->need_sink_convert = g_value_get_boolean(value);
            break;
        case PROP_PARSER:
            bin->need_parser = g_value_get_boolean(value);
            break;
        case PROP_FORCE_I_FRAME:
            if (bin->coder != nullptr) {
                g_object_set(bin->coder, "req-i-frame", g_value_get_int(value), nullptr);
            }
            break;
        case PROP_USE_SURFACE_INPUT:
            bin->is_input_surface = g_value_get_boolean(value);
            break;
        default:
            break;
    }
}

static void gst_codec_bin_get_property(GObject *object, guint prop_id,
    GValue *value, GParamSpec *param_spec)
{
    (void)param_spec;
    GstCodecBin *bin = GST_CODEC_BIN(object);
    g_return_if_fail(bin != nullptr);
    switch (prop_id) {
        case PROP_TYPE:
            g_value_set_enum(value, bin->type);
            break;
        case PROP_USE_SOFTWARE:
            g_value_set_boolean(value, bin->use_software);
            break;
        case PROP_CODER_NAME:
            g_value_set_string(value, bin->coder_name);
            break;
        case PROP_SRC_CONVERT:
            g_value_set_boolean(value, bin->need_src_convert);
            break;
        case PROP_SINK_CONVERT:
            g_value_set_boolean(value, bin->need_sink_convert);
            break;
        default:
            break;
    }
}

static gboolean create_coder(GstCodecBin *bin)
{
    g_return_val_if_fail(bin != nullptr, FALSE);
    g_return_val_if_fail(bin->coder_name != nullptr, FALSE);
    g_return_val_if_fail(bin->type != CODEC_BIN_TYPE_UNKNOWN, FALSE);
    bin->coder = gst_element_factory_make(bin->coder_name, "coder");
    g_return_val_if_fail(bin->coder != nullptr, FALSE);
    g_object_set(bin->coder, "enable-surface", bin->is_input_surface, nullptr);
    return TRUE;
}

static void add_dump_probe(GstCodecBin *bin)
{
    if (!OHOS::Media::Dumper::IsEnableDumpGstBuffer()) {
        return;
    }

    if (bin->parser != nullptr) {
        OHOS::Media::Dumper::AddDumpGstBufferProbe(bin->parser, "src");
        OHOS::Media::Dumper::AddDumpGstBufferProbe(bin->parser, "sink");
    }

    if (bin->src != nullptr) {
        OHOS::Media::Dumper::AddDumpGstBufferProbe(bin->src, "src");
    }
    if (bin->sink_convert != nullptr) {
        OHOS::Media::Dumper::AddDumpGstBufferProbe(bin->sink_convert, "src");
        OHOS::Media::Dumper::AddDumpGstBufferProbe(bin->sink_convert, "sink");
    }
    if (bin->sink != nullptr) {
        OHOS::Media::Dumper::AddDumpGstBufferProbe(bin->sink, "sink");
    }
}

static gboolean connect_element(GstCodecBin *bin)
{
    g_return_val_if_fail(bin != nullptr && bin->src != nullptr, FALSE);
    g_return_val_if_fail(bin->coder != nullptr && bin->sink != nullptr, FALSE);

    gboolean ret = FALSE;
    if (bin->src_convert != nullptr) {
        ret = gst_element_link_pads_full(bin->src, "src", bin->src_convert, "sink", GST_PAD_LINK_CHECK_NOTHING);
        g_return_val_if_fail(ret == TRUE, FALSE);
        ret = gst_element_link_pads_full(bin->src_convert, "src", bin->coder, "sink", GST_PAD_LINK_CHECK_NOTHING);
        g_return_val_if_fail(ret == TRUE, FALSE);
    } else if (bin->parser != nullptr) {
        ret = gst_element_link_pads_full(bin->src, "src", bin->parser, "sink", GST_PAD_LINK_CHECK_NOTHING);
        g_return_val_if_fail(ret == TRUE, FALSE);
        ret = gst_element_link_pads_full(bin->parser, "src", bin->coder, "sink", GST_PAD_LINK_CHECK_NOTHING);
        g_return_val_if_fail(ret == TRUE, FALSE);
    } else {
        ret = gst_element_link_pads_full(bin->src, "src", bin->coder, "sink", GST_PAD_LINK_CHECK_NOTHING);
        g_return_val_if_fail(ret == TRUE, FALSE);
    }

    if (bin->sink_convert != nullptr) {
        ret = gst_element_link_pads_full(bin->coder, "src", bin->sink_convert, "sink", GST_PAD_LINK_CHECK_NOTHING);
        g_return_val_if_fail(ret == TRUE, FALSE);
        ret = gst_element_link_pads_full(bin->sink_convert, "src", bin->sink, "sink", GST_PAD_LINK_CHECK_NOTHING);
        g_return_val_if_fail(ret == TRUE, FALSE);
    } else {
        ret = gst_element_link_pads_full(bin->coder, "src", bin->sink, "sink", GST_PAD_LINK_CHECK_NOTHING);
        g_return_val_if_fail(ret == TRUE, FALSE);
    }
    GST_INFO_OBJECT(bin, "connect_element success");

    add_dump_probe(bin);
    return TRUE;
}

static gboolean add_parser(GstCodecBin *bin)
{
    g_return_val_if_fail(bin != nullptr, FALSE);
    bin->parser = gst_element_factory_make("flacparse", "parser");
    g_return_val_if_fail(bin->parser != nullptr, FALSE);
    return gst_bin_add(GST_BIN_CAST(bin), bin->parser);
}

static gboolean add_src_convert(GstCodecBin *bin)
{
    g_return_val_if_fail(bin != nullptr, FALSE);
    gboolean isVideo = (bin->type == CODEC_BIN_TYPE_VIDEO_DECODER ||
        bin->type == CODEC_BIN_TYPE_VIDEO_ENCODER) ? TRUE : FALSE;
    if (isVideo) {
        bin->src_convert = gst_element_factory_make("videoconvert", "src_convert");
    } else {
        bin->src_convert = gst_element_factory_make("audioconvert", "src_convert");
    }
    g_return_val_if_fail(bin->src_convert != nullptr, FALSE);
    return gst_bin_add(GST_BIN_CAST(bin), bin->src_convert);
}

static gboolean add_sink_convert(GstCodecBin *bin)
{
    g_return_val_if_fail(bin != nullptr, FALSE);
    gboolean isVideo = (bin->type == CODEC_BIN_TYPE_VIDEO_DECODER ||
        bin->type == CODEC_BIN_TYPE_VIDEO_ENCODER) ? TRUE : FALSE;
    if (isVideo) {
        bin->sink_convert = gst_element_factory_make("videoconvert", "sink_convert");
    } else {
        bin->sink_convert = gst_element_factory_make("audioconvert", "sink_convert");
    }
    g_return_val_if_fail(bin->sink_convert != nullptr, FALSE);
    return gst_bin_add(GST_BIN_CAST(bin), bin->sink_convert);
}

static gboolean add_parser_if_necessary(GstCodecBin *bin)
{
    g_return_val_if_fail(bin != nullptr, FALSE);
    if (bin->need_parser && add_parser(bin) == FALSE) {
        GST_ERROR_OBJECT(bin, "Failed to add_parser");
        return FALSE;
    }
    return TRUE;
}

static gboolean add_convert_if_necessary(GstCodecBin *bin)
{
    g_return_val_if_fail(bin != nullptr, FALSE);
    if (bin->need_src_convert && add_src_convert(bin) == FALSE) {
        GST_ERROR_OBJECT(bin, "Failed to add_src_convert");
        return FALSE;
    }
    if (bin->need_sink_convert && add_sink_convert(bin) == FALSE) {
        GST_ERROR_OBJECT(bin, "Failed to add_sink_convert");
        return FALSE;
    }
    return TRUE;
}

static gboolean add_element_to_bin(GstCodecBin *bin)
{
    g_return_val_if_fail(bin != nullptr, FALSE);
    g_return_val_if_fail(bin->src != nullptr && bin->coder != nullptr && bin->sink != nullptr, FALSE);

    gboolean ret = gst_bin_add(GST_BIN_CAST(bin), bin->src);
    g_return_val_if_fail(ret == TRUE, FALSE);

    ret = add_parser_if_necessary(bin);
    g_return_val_if_fail(ret == TRUE, FALSE);

    ret = add_convert_if_necessary(bin);
    g_return_val_if_fail(ret == TRUE, FALSE);

    ret = gst_bin_add(GST_BIN_CAST(bin), bin->coder);
    g_return_val_if_fail(ret == TRUE, FALSE);

    return gst_bin_add(GST_BIN_CAST(bin), bin->sink);
}

static GstStateChangeReturn gst_codec_bin_change_state(GstElement *element, GstStateChange transition)
{
    GstCodecBin *bin = GST_CODEC_BIN(element);
    g_return_val_if_fail(bin != nullptr && bin->type != CODEC_BIN_TYPE_UNKNOWN, GST_STATE_CHANGE_FAILURE);

    switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
            if (create_coder(bin) == FALSE) {
                GST_ERROR_OBJECT(bin, "Failed to create_coder");
                return GST_STATE_CHANGE_FAILURE;
            }
            break;
        case GST_STATE_CHANGE_READY_TO_PAUSED:
            if (bin->is_start == FALSE) {
                if (add_element_to_bin(bin) == FALSE) {
                    GST_ERROR_OBJECT(bin, "Failed to add_element_to_bin");
                    return GST_STATE_CHANGE_FAILURE;
                }
                if (connect_element(bin) == FALSE) {
                    GST_ERROR_OBJECT(bin, "Failed to connect_element");
                    return GST_STATE_CHANGE_FAILURE;
                }
                bin->is_start = TRUE;
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
    return gst_element_register(plugin, "codecbin", GST_RANK_PRIMARY, GST_TYPE_CODEC_BIN);
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    _codec_bin,
    "GStreamer Codec Bin",
    plugin_init,
    PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
