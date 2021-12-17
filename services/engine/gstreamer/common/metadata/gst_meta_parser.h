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

#ifndef GST_META_PARSER_H
#define GST_META_PARSER_H

#include <gst/gst.h>
#include <unordered_map>
#include <string>

namespace OHOS {
namespace Media {
enum InnerMetaKey : int32_t {
    INNER_META_KEY_ALBUM = 0,
    INNER_META_KEY_ALBUM_ARTIST,
    INNER_META_KEY_ARTIST,
    INNER_META_KEY_AUTHOR,
    INNER_META_KEY_COMPOSER,
    INNER_META_KEY_GENRE,
    INNER_META_KEY_HAS_AUDIO,
    INNER_META_KEY_HAS_VIDEO,
    INNER_META_KEY_HAS_IMAGE,
    INNER_META_KEY_HAS_TEXT,
    INNER_META_KEY_MIME_TYPE,
    INNER_META_KEY_NUM_TRACKS,
    INNER_META_KEY_SAMPLE_RATE,
    INNER_META_KEY_TITLE,
    INNER_META_KEY_VIDEO_HEIGHT,
    INNER_META_KEY_VIDEO_WIDTH,
    INNER_META_KEY_ROTATION,
    INNER_META_KEY_BUTT,
};

inline constexpr std::string_view FILE_MIMETYPE_VIDEO_MP4 = "video/mp4";
inline constexpr std::string_view FILE_MIMETYPE_AUDIO_MP4 = "audio/mp4";
inline constexpr std::string_view FILE_MIMETYPE_AUDIO_AAC = "audio/aac-adts";
inline constexpr std::string_view FILE_MIMETYPE_AUDIO_OGG = "audio/ogg";
inline constexpr std::string_view FILE_MIMETYPE_AUDIO_FLAC = "audio/flac";
inline constexpr std::string_view FILE_MIMETYPE_AUDIO_WAV = "audio/wav";
inline constexpr std::string_view FILE_MIMETYPE_AUDIO_MP3 = "audio/mpeg";

struct Metadata;

class GstMetaParser {
public:
    static void ParseTagList(const GstTagList &tagList, Metadata &metadata);
    static void ParseStreamCaps(const GstCaps &caps, Metadata &metadata);
    static void ParseFileMimeType(const GstCaps &caps, Metadata &metadata);

private:
    GstMetaParser() = default;
    ~GstMetaParser() = default;
};

struct Metadata {
    Metadata() = default;
    ~Metadata() = default;

    void SetMeta(int32_t key, const std::string &value)
    {
        tbl_[key] = value;
    }

    bool TryGetMeta(int32_t key, std::string &value) const
    {
        auto it = tbl_.find(key);
        if (it == tbl_.end()) {
            return false;
        }
        value = it->second;
        return true;
    }

    bool HasMeta(int32_t key) const
    {
        return tbl_.count(key) != 0;
    }

    std::string GetMeta(int32_t key) const
    {
        if (tbl_.count(key) != 0) {
            return tbl_.at(key);
        }
        return "";
    }

    std::unordered_map<int32_t, std::string> tbl_;
};
}
}

#endif