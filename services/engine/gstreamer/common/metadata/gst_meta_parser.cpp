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
#include <string_view>
#include <vector>
#include "media_errors.h"
#include "media_log.h"
#include "gst_utils.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "GstMetaParser"};
}

namespace OHOS {
namespace Media {
#define INNER_META_KEY_TO_STRING_ITEM(key) { key, #key }
static const std::unordered_map<int32_t, std::string_view> INNER_META_KEY_TO_STRING_MAP = {
    INNER_META_KEY_TO_STRING_ITEM(INNER_META_KEY_ALBUM),
    INNER_META_KEY_TO_STRING_ITEM(INNER_META_KEY_ALBUM_ARTIST),
    INNER_META_KEY_TO_STRING_ITEM(INNER_META_KEY_ARTIST),
    INNER_META_KEY_TO_STRING_ITEM(INNER_META_KEY_AUTHOR),
    INNER_META_KEY_TO_STRING_ITEM(INNER_META_KEY_COMPOSER),
    INNER_META_KEY_TO_STRING_ITEM(INNER_META_KEY_GENRE),
    INNER_META_KEY_TO_STRING_ITEM(INNER_META_KEY_HAS_AUDIO),
    INNER_META_KEY_TO_STRING_ITEM(INNER_META_KEY_HAS_VIDEO),
    INNER_META_KEY_TO_STRING_ITEM(INNER_META_KEY_HAS_IMAGE),
    INNER_META_KEY_TO_STRING_ITEM(INNER_META_KEY_HAS_TEXT),
    INNER_META_KEY_TO_STRING_ITEM(INNER_META_KEY_MIME_TYPE),
    INNER_META_KEY_TO_STRING_ITEM(INNER_META_KEY_NUM_TRACKS),
    INNER_META_KEY_TO_STRING_ITEM(INNER_META_KEY_SAMPLE_RATE),
    INNER_META_KEY_TO_STRING_ITEM(INNER_META_KEY_TITLE),
    INNER_META_KEY_TO_STRING_ITEM(INNER_META_KEY_VIDEO_HEIGHT),
    INNER_META_KEY_TO_STRING_ITEM(INNER_META_KEY_VIDEO_WIDTH),
};

static const std::unordered_map<std::string_view, int32_t> GST_TAG_TO_KEY_MAPPING = {
    { GST_TAG_ALBUM, INNER_META_KEY_ALBUM },
    { GST_TAG_ALBUM_ARTIST, INNER_META_KEY_ALBUM_ARTIST },
    { GST_TAG_ARTIST, INNER_META_KEY_ARTIST },
    { GST_TAG_COMPOSER, INNER_META_KEY_COMPOSER },
    { GST_TAG_GENRE, INNER_META_KEY_GENRE },
    { GST_TAG_TRACK_COUNT, INNER_META_KEY_NUM_TRACKS },
    { GST_TAG_TITLE, INNER_META_KEY_TITLE }
};

static const std::unordered_map<std::string_view, int32_t> GST_CAPS_FIELD_TO_KEY_MAPPING = {
    { "width", INNER_META_KEY_VIDEO_WIDTH },
    { "height", INNER_META_KEY_VIDEO_HEIGHT },
    { "rate", INNER_META_KEY_SAMPLE_RATE },
};

struct StreamMetaParseTarget {
    std::vector<std::string_view> expectedCapsFields;
    int32_t indicatedExistMetaKey;
};

static const std::unordered_map<std::string_view, StreamMetaParseTarget> STREAM_TO_META_PARSE_TARGET_MAPPING = {
    { "video", { { "width", "height" }, INNER_META_KEY_HAS_VIDEO} },
    { "audio", { { "rate" }, INNER_META_KEY_HAS_AUDIO} },
    { "image", { { "width", "height" }, INNER_META_KEY_HAS_IMAGE} },
    { "text", { { "format" }, INNER_META_KEY_HAS_TEXT} },
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

static std::string GetSerializedValue(const GValue &value)
{
    if (G_VALUE_HOLDS_STRING(&value)) {
        const gchar *str = g_value_get_string(&value);
        CHECK_AND_RETURN_RET_LOG(str != nullptr, "", "gvalue has no null value");
        MEDIA_LOGI("value: %{public}s", str);
        return std::string(str);
    }

    gchar *str = gst_value_serialize(&value);
    CHECK_AND_RETURN_RET_LOG(str != nullptr, "", "serialize value failed");
    std::string serializedValue = str;
    g_free(str);

    return serializedValue;
}

static void ParseTag(const GstTagList &tagList, guint tagIndex, Metadata &metadata)
{
    const gchar *tag = gst_tag_list_nth_tag_name(&tagList, tagIndex);
    if (tag == nullptr) {
        return;
    }

    MEDIA_LOGD("visit tag: %{public}s", tag);
    auto tagToKeyIt = GST_TAG_TO_KEY_MAPPING.find(tag);
    if (tagToKeyIt == GST_TAG_TO_KEY_MAPPING.end()) {
        return;
    }

    int32_t metaKey = tagToKeyIt->second;
    const GValue *value = gst_tag_list_get_value_index(&tagList, tag, 0);
    CHECK_AND_RETURN_LOG(value != nullptr, "get value for tag: %{public}s failed", tag);

    if (GST_VALUE_HOLDS_SAMPLE(value) || GST_VALUE_HOLDS_BUFFER(value)) {
        return; // not considering the sample and bufer currently.
    }

    std::string result = GetSerializedValue(*value);
    if (result.empty()) {
        return;
    }

    metadata.SetMeta(metaKey, result);
    MEDIA_LOGI("got meta %{public}s : %{public}s", INNER_META_KEY_TO_STRING_MAP.at(metaKey).data(), result.c_str());
}

static void ParseSingleCapsStructure(const GstStructure &structure,
                                     const StreamMetaParseTarget &target,
                                     Metadata &metadata)
{
    metadata.SetMeta(target.indicatedExistMetaKey, "yes");

    for (auto &field : target.expectedCapsFields) {
        if (!gst_structure_has_field(&structure, field.data())) {
            continue;
        }
        const GValue *val = gst_structure_get_value(&structure, field.data());
        if (val == nullptr) {
            MEDIA_LOGE("get %{public}s filed's value failed", field.data());
            continue;
        }
        std::string str = GetSerializedValue(*val);
        if (str.empty()) {
            MEDIA_LOGE("serialize value for field %{public}s failed", field.data());
            continue;
        }

        MEDIA_LOGI("field %{public}s is %{public}s", field.data(), str.c_str());
        if (GST_CAPS_FIELD_TO_KEY_MAPPING.count(field) != 0) {
            metadata.SetMeta(GST_CAPS_FIELD_TO_KEY_MAPPING.at(field), str);
        }
    }
}

void GstMetaParser::ParseTagList(const GstTagList &tagList, Metadata &metadata)
{
    gint tagCnt = gst_tag_list_n_tags(&tagList);
    if (tagCnt < 0) {
        return;
    }
    for (guint tagIndex = 0; tagIndex < static_cast<guint>(tagCnt); tagIndex++) {
        ParseTag(tagList, tagIndex, metadata);
    }

    return;
}

void GstMetaParser::ParseStreamCaps(const GstCaps &caps, Metadata &metadata)
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
        auto it = STREAM_TO_META_PARSE_TARGET_MAPPING.find(streamType);
        if (it == STREAM_TO_META_PARSE_TARGET_MAPPING.end()) {
            continue;
        }
        ParseSingleCapsStructure(*struc, it->second, metadata);
    }
}

void GstMetaParser::ParseFileMimeType(const GstCaps &caps, Metadata &metadata)
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
    metadata.SetMeta(INNER_META_KEY_MIME_TYPE, it->second.data());
}
}
}