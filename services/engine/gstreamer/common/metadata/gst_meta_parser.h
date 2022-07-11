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
#include "format.h"

namespace OHOS {
namespace Media {
// meta key, sorted by alphabetical order.
inline constexpr std::string_view INNER_META_KEY_ALBUM = "album";
inline constexpr std::string_view INNER_META_KEY_ALBUM_ARTIST = "albumartist";
inline constexpr std::string_view INNER_META_KEY_ARTIST = "artist";
inline constexpr std::string_view INNER_META_KEY_AUTHOR = "author";
inline constexpr std::string_view INNER_META_KEY_BITRATE = "bitrate";
inline constexpr std::string_view INNER_META_KEY_CHANNEL_COUNT = "channel-count";
inline constexpr std::string_view INNER_META_KEY_COMPOSER = "composer";
inline constexpr std::string_view INNER_META_KEY_DATE_TIME = "datetime";
inline constexpr std::string_view INNER_META_KEY_DURATION = "duration";
inline constexpr std::string_view INNER_META_KEY_FRAMERATE = "frame-rate";
inline constexpr std::string_view INNER_META_KEY_GENRE = "genre";
inline constexpr std::string_view INNER_META_KEY_VIDEO_HEIGHT = "height";
inline constexpr std::string_view INNER_META_KEY_IMAGE = "image";
inline constexpr std::string_view INNER_META_KEY_LANGUAGE = "language";
inline constexpr std::string_view INNER_META_KEY_VIDEO_ORIENTATION = "image-orientation";
inline constexpr std::string_view INNER_META_KEY_MIME_TYPE = "mime";
inline constexpr std::string_view INNER_META_KEY_SAMPLE_RATE = "samplerate";
inline constexpr std::string_view INNER_META_KEY_TITLE = "title";
inline constexpr std::string_view INNER_META_KEY_TRACK_INDEX = "track-index";
inline constexpr std::string_view INNER_META_KEY_TRACK_TYPE = "track-type";
inline constexpr std::string_view INNER_META_KEY_VIDEO_WIDTH = "width";

// video codec mime
inline constexpr std::string_view VIDEO_MIMETYPE_AVC = "video/avc";
inline constexpr std::string_view VIDEO_MIMETYPE_MPEG4 = "video/mp4v-es";

// audio codec mime
inline constexpr std::string_view AUDIO_MIMETYPE_AAC = "audio/mp4a-latm";
inline constexpr std::string_view AUDIO_MIMETYPE_MPEG = "audio/mpeg";

// container mime
inline constexpr std::string_view FILE_MIMETYPE_VIDEO_MP4 = "video/mp4";
inline constexpr std::string_view FILE_MIMETYPE_AUDIO_MP4 = "audio/mp4";
inline constexpr std::string_view FILE_MIMETYPE_VIDEO_MKV = "video/x-matroska";
inline constexpr std::string_view FILE_MIMETYPE_AUDIO_MKV = "audio/x-matroska";
inline constexpr std::string_view FILE_MIMETYPE_VIDEO_MPEGTS = "video/mp2ts";
inline constexpr std::string_view FILE_MIMETYPE_VIDEO_WEBM = "video/webm";
inline constexpr std::string_view FILE_MIMETYPE_AUDIO_WEBM = "audio/webm";
inline constexpr std::string_view FILE_MIMETYPE_AUDIO_AAC = "audio/aac-adts";
inline constexpr std::string_view FILE_MIMETYPE_AUDIO_OGG = "audio/ogg";
inline constexpr std::string_view FILE_MIMETYPE_AUDIO_FLAC = "audio/flac";
inline constexpr std::string_view FILE_MIMETYPE_AUDIO_WAV = "audio/wav";
inline constexpr std::string_view FILE_MIMETYPE_AUDIO_MP3 = "audio/mpeg";

// parse meta from GstTagList, GstCaps
class GstMetaParser {
public:
    static void ParseTagList(const GstTagList &tagList, Format &metadata);
    static void ParseStreamCaps(const GstCaps &caps, Format &metadata);
    static void ParseFileMimeType(const GstCaps &caps, Format &metadata);

private:
    GstMetaParser() = default;
    ~GstMetaParser() = default;
};
} // namespace Media
} // namespace OHOS
#endif // GST_META_PARSER_H