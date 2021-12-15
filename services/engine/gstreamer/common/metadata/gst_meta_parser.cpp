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

#include "gst_meta_parser.h"
#include <functional>
#include <vector>
#include "media_defs.h"
#include "media_errors.h"
#include "media_log.h"
#include "gst/tag/tag.h"
#include "gst_utils.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "GstMetaParser"};
    static GType GST_SAMPLE_TYPE = gst_sample_get_type();
}

namespace OHOS {
namespace Media {
using MetaSetter = std::function<bool(const GValue &gval, const std::string_view &key, Format &metadata)>;

struct MetaParseItem {
    std::string_view toKey;
    GType valGType;
    MetaSetter setter;
};

static bool ParseGValueSimple(const GValue &value, const MetaParseItem &item, Format &metadata);
static bool FractionMetaSetter(const GValue &gval, const std::string_view &key, Format &metadata);
static bool ImageMetaSetter(const GValue &gval, const std::string_view &key, Format &metadata);

static const std::unordered_map<std::string_view, MetaParseItem> GST_TAG_PARSE_ITEMS = {
    { GST_TAG_ALBUM, { INNER_META_KEY_ALBUM, G_TYPE_STRING } },
    { GST_TAG_ALBUM_ARTIST, { INNER_META_KEY_ALBUM_ARTIST, G_TYPE_STRING } },
    { GST_TAG_ARTIST, { INNER_META_KEY_ARTIST, G_TYPE_STRING } },
    { GST_TAG_COMPOSER, { INNER_META_KEY_COMPOSER, G_TYPE_STRING } },
    { GST_TAG_GENRE, { INNER_META_KEY_GENRE, G_TYPE_STRING } },
    { GST_TAG_TITLE, { INNER_META_KEY_TITLE, G_TYPE_STRING } },
    { GST_TAG_AUTHOR, { INNER_META_KEY_AUTHOR, G_TYPE_STRING } },
    { GST_TAG_DURATION, { INNER_META_KEY_DURATION, G_TYPE_UINT64 } },
    { GST_TAG_BITRATE, { INNER_META_KEY_BITRATE, G_TYPE_UINT } },
    { GST_TAG_IMAGE, { INNER_META_KEY_IMAGE, GST_SAMPLE_TYPE, ImageMetaSetter } },
};

static const std::unordered_map<std::string_view, MetaParseItem> GST_CAPS_PARSE_ITEMS = {
    { "width", { INNER_META_KEY_VIDEO_WIDTH, G_TYPE_INT, nullptr } },
    { "height", { INNER_META_KEY_VIDEO_HEIGHT, G_TYPE_INT } },
    { "rate", { INNER_META_KEY_SAMPLE_RATE, G_TYPE_INT } },
    { "framerate", { INNER_META_KEY_FRAMERATE, GST_TYPE_FRACTION, FractionMetaSetter } },
    { "channels", { INNER_META_KEY_CHANNEL_COUNT, G_TYPE_INT } },
};

static const std::unordered_map<std::string_view, std::vector<std::string_view>> STREAM_CAPS_FIELDS = {
    { "video", { "width", "height", "framrate", "format" } },
    { "audio", { "rate", "channels" } },
    { "text", { "format" } },
};

static const std::unordered_map<std::string_view, std::string_view> FILE_MIME_TYPE_MAPPING = {
    { "application/x-3gp", FILE_MIMETYPE_VIDEO_MP4 }, // mp4, m4a, mov, 3g2, 3gp
    { "video/quicktime", FILE_MIMETYPE_VIDEO_MP4 },
    { "video/quicktime, variant=(string)apple", FILE_MIMETYPE_VIDEO_MP4 },
    { "video/quicktime, variant=(string)iso", FILE_MIMETYPE_VIDEO_MP4 },
    { "video/quicktime, variant=(string)3gpp", FILE_MIMETYPE_VIDEO_MP4 },
    { "video/quicktime, variant=(string)3g2", FILE_MIMETYPE_VIDEO_MP4 },
    { "audio/x-m4a", FILE_MIMETYPE_AUDIO_MP4 },
    { "audio/mpeg", FILE_MIMETYPE_AUDIO_AAC }, // aac
    { "application/x-id3", FILE_MIMETYPE_AUDIO_MP3 }, // mp3
    { "application/ogg", FILE_MIMETYPE_AUDIO_OGG }, // ogg
    { "audio/ogg", FILE_MIMETYPE_AUDIO_OGG },
    { "audio/x-flac", FILE_MIMETYPE_AUDIO_FLAC }, // flac
    { "audio/x-wav", FILE_MIMETYPE_AUDIO_WAV } // wav
};

static void ParseGValue(const GValue &value, const MetaParseItem &item, Format &metadata)
{
    if (G_VALUE_TYPE(&value) != item.valGType) {
        MEDIA_LOGE("value type for key %{public}s is expected, curr is %{public}s, but expect %{public}s",
            item.toKey.data(), g_type_name(G_VALUE_TYPE(&value)), g_type_name(item.valGType));
        return;
    }

    bool ret = true;
    if (item.setter != nullptr) {
        ret = item.setter(value, item.toKey, metadata);
    } else {
        ret = ParseGValueSimple(value, item, metadata);
    }

    if (!ret) {
        MEDIA_LOGE("parse gvalue failed for type: %{public}s, toKey: %{public}s",
            g_type_name(item.valGType), item.toKey.data());
    }
}

static void ParseTag(const GstTagList &tagList, guint tagIndex, Format &metadata)
{
    const gchar *tag = gst_tag_list_nth_tag_name(&tagList, tagIndex);
    if (tag == nullptr) {
        return;
    }

    MEDIA_LOGD("visit tag: %{public}s", tag);
    auto tagItemIt = GST_TAG_PARSE_ITEMS.find(tag);
    if (tagItemIt == GST_TAG_PARSE_ITEMS.end()) {
        return;
    }

    guint tagValSize = gst_tag_list_get_tag_size(&tagList, tag);
    for (guint i = 0; i < tagValSize; i++) {
        const GValue *value = gst_tag_list_get_value_index(&tagList, tag, i);
        if (value == nullptr) {
            MEDIA_LOGW("get value index %{public}d from tag %{public}s failed", i, tag);
            continue;
        }

        ParseGValue(*value, tagItemIt->second, metadata);
    }
}

void GstMetaParser::ParseTagList(const GstTagList &tagList, Format &metadata)
{
    gint tagCnt = gst_tag_list_n_tags(&tagList);
    if (tagCnt <= 0) {
        return;
    }

    for (guint tagIndex = 0; tagIndex < static_cast<guint>(tagCnt); tagIndex++) {
        ParseTag(tagList, tagIndex, metadata);
    }
}

static void ParseSingleCapsStructure(const GstStructure &structure,
                                     const std::vector<std::string_view> &target,
                                     Format &metadata)
{
    for (auto &field : target) {
        if (!gst_structure_has_field(&structure, field.data())) {
            continue;
        }
        const GValue *val = gst_structure_get_value(&structure, field.data());
        if (val == nullptr) {
            MEDIA_LOGE("get %{public}s filed's value failed", field.data());
            continue;
        }

        auto it = GST_CAPS_PARSE_ITEMS.find(field);
        if (it != GST_CAPS_PARSE_ITEMS.end()) {
            ParseGValue(*val, it->second, metadata);
        }
    }
}

void GstMetaParser::ParseStreamCaps(const GstCaps &caps, Format &metadata)
{
    guint capsSize = gst_caps_get_size(&caps);
    for (guint index = 0; index < capsSize; index++) {
        const GstStructure *struc = gst_caps_get_structure(&caps, index);
        std::string_view mimeType = STRUCTURE_NAME(struc);

        std::string_view::size_type delimPos = mimeType.find('/');
        if (delimPos == std::string_view::npos) {
            MEDIA_LOGE("unrecognizable mimetype, %{public}s", mimeType.data());
            continue;
        }

        MEDIA_LOGI("parse mimetype %{public}s's caps", mimeType.data());

        std::string_view streamType = mimeType.substr(0, delimPos);
        auto it = STREAM_CAPS_FIELDS.find(streamType);
        if (it == STREAM_CAPS_FIELDS.end()) {
            continue;
        }

        if (streamType.compare("video") == 0) {
            metadata.PutIntValue(INNER_META_KEY_TRACK_TYPE, MediaTrackType::MEDIA_TYPE_VIDEO);
        } else if (streamType.compare("audio") == 0) {
            metadata.PutIntValue(INNER_META_KEY_TRACK_TYPE, MediaTrackType::MEDIA_TYPE_AUDIO);
        } else if (streamType.compare("text") == 0) {
            metadata.PutIntValue(INNER_META_KEY_TRACK_TYPE, MediaTrackType::MEDIA_TYPE_SUBTITLE);
        }

        metadata.PutStringValue(INNER_META_KEY_MIME_TYPE, mimeType);
        ParseSingleCapsStructure(*struc, it->second, metadata);
    }
}

void GstMetaParser::ParseFileMimeType(const GstCaps &caps, Format &metadata)
{
    const GstStructure *struc = gst_caps_get_structure(&caps, 0); // ignore the caps size
    CHECK_AND_RETURN_LOG(struc != nullptr, "caps invalid");
    auto mimeType = STRUCTURE_NAME(struc);
    CHECK_AND_RETURN_LOG(mimeType != nullptr, "caps invalid");

    const auto &it = FILE_MIME_TYPE_MAPPING.find(mimeType);
    if (it == FILE_MIME_TYPE_MAPPING.end()) {
        MEDIA_LOGE("unrecognizable file mimetype: %{public}s", mimeType);
        return;
    }

    MEDIA_LOGI("file caps mime type: %{public}s, mapping to %{public}s", mimeType, it->second.data());
    metadata.PutStringValue(INNER_META_KEY_MIME_TYPE, it->second);
}

static bool ParseGValueSimple(const GValue &value, const MetaParseItem &item, Format &metadata)
{
    bool ret = true;
    switch (item.valGType) {
        case G_TYPE_UINT: {
            guint num = g_value_get_uint(&value);
            ret = metadata.PutIntValue(item.toKey, static_cast<int32_t>(num));
            MEDIA_LOGD("toKey: %{public}s, value: %{public}u", item.toKey.data(), num);
            break;
        }
        case G_TYPE_INT: {
            gint num = g_value_get_int(&value);
            ret = metadata.PutIntValue(item.toKey, num);
            MEDIA_LOGD("toKey: %{public}s, value: %{public}d", item.toKey.data(), num);
            break;
        }
        case G_TYPE_UINT64: {
            guint64 num = g_value_get_uint64(&value);
            ret = metadata.PutLongValue(item.toKey, static_cast<int64_t>(num));
            MEDIA_LOGD("toKey: %{public}s, value: %{public}" PRIu64, item.toKey.data(), num);
            break;
        }
        case G_TYPE_INT64: {
            gint64 num = g_value_get_int64(&value);
            ret = metadata.PutLongValue(item.toKey, num);
            MEDIA_LOGD("toKey: %{public}s, value: %{public}" PRIi64, item.toKey.data(), num);
            break;
        }
        case G_TYPE_STRING: {
            std::string_view str = g_value_get_string(&value);
            CHECK_AND_RETURN_RET_LOG(!str.empty(), false, "Parse key %{public}s failed", item.toKey.data());
            ret = metadata.PutStringValue(item.toKey, str);
            MEDIA_LOGD("toKey: %{public}s, value: %{public}s", item.toKey.data(), str.data());
            break;
        }
        default:
            break;
    }
    return true;
}

static bool FractionMetaSetter(const GValue &gval, const std::string_view &key, Format &metadata)
{
    gint numerator = gst_value_get_fraction_numerator(&gval);
    gint denominator = gst_value_get_fraction_denominator(&gval);
    CHECK_AND_RETURN_RET(denominator != 0, false);

    float val = static_cast<float>(numerator) / denominator;
    static constexpr int32_t FACTOR = 100;
    bool ret = metadata.PutIntValue(key, static_cast<int32_t>(val * FACTOR));
    if (!ret) {
        MEDIA_LOGE("Parse gvalue for %{public}s failed, num = %{public}d, den = %{public}d",
            key.data(), numerator, denominator);
    } else {
        MEDIA_LOGD("Parse gvalue for %{public}s ok, value = %{public}f", key.data(), val);
    }
    return ret;
}

static bool ImageMetaSetter(const GValue &gval, const std::string_view &key, Format &metadata)
{
    GstSample *sample = gst_value_get_sample(&gval);
    CHECK_AND_RETURN_RET(sample != nullptr, false);

    GstCaps *caps = gst_sample_get_caps(sample);
    CHECK_AND_RETURN_RET(caps != nullptr, false);

    GstStructure *structure = gst_caps_get_structure(caps, 0);
    CHECK_AND_RETURN_RET(structure != nullptr, false);

    const gchar *mime = gst_structure_get_name(structure);
    CHECK_AND_RETURN_RET(mime != nullptr, false);

    if (g_str_has_prefix(mime, "text")) {
        MEDIA_LOGI("image is uri, ignored, %{public}s", mime);
        return true;
    }

    GstBuffer *imageBuf = gst_sample_get_buffer(sample);
    CHECK_AND_RETURN_RET(imageBuf != nullptr, false);

    GstMapInfo mapInfo = { 0 };
    if (!gst_buffer_map(imageBuf, &mapInfo, GST_MAP_READ)) {
        MEDIA_LOGE("get buffer data failed");
        return false;
    }

    static const gsize minImageSize = 32;
    CHECK_AND_RETURN_RET(mapInfo.data != nullptr && mapInfo.size > minImageSize, false);

    bool ret = true;
    if (metadata.ContainKey(key)) {
        const GstStructure *imageInfo = gst_sample_get_info(sample);
        gint type = GST_TAG_IMAGE_TYPE_NONE;
        if (imageInfo != nullptr) {
            gst_structure_get_enum(imageInfo, "image-type", GST_TYPE_TAG_IMAGE_TYPE, &type);
        }
        if (type != GST_TAG_IMAGE_TYPE_FRONT_COVER) {
            MEDIA_LOGI("ignore the image type: %{public}d", type);
        } else {
            MEDIA_LOGI("got front cover image");
            ret = metadata.PutBuffer(key, mapInfo.data, mapInfo.size);
        }
    } else {
        ret = metadata.PutBuffer(key, mapInfo.data, mapInfo.size);
    }

    MEDIA_LOGD("Parse gvalue for %{public}s finish, ret = %{public}d, mime: %{public}s, "
               "size: %{public}" G_GSIZE_FORMAT,
               key.data(), ret, mime, mapInfo.size);

    gst_buffer_unmap(imageBuf, &mapInfo);
    return ret;
}
}
}