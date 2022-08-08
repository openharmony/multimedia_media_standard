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

#ifndef ENUM_MOCK_H
#define ENUM_MOCK_H

#include <string>
#include "nocopyable.h"

namespace OHOS {
namespace Media {
enum MediaDescriptionKeyMock : int32_t {
    MOCK_MD_KEY_TRACK_INDEX = 0,
    MOCK_MD_KEY_TRACK_TYPE,
    MOCK_MD_KEY_CODEC_MIME,
    MOCK_MD_KEY_DURATION,
    MOCK_MD_KEY_BITRATE,
    MOCK_MD_KEY_WIDTH,
    MOCK_MD_KEY_HEIGHT,
    MOCK_MD_KEY_PIXEL_FORMAT,
    MOCK_MD_KEY_FRAME_RATE,
    MOCK_MD_KEY_CHANNEL_COUNT,
    MOCK_MD_KEY_SAMPLE_RATE,
};

enum VideoPixelFormatMock : int32_t {
    MOCK_YUVI420 = 1,
    MOCK_NV12,
    MOCK_NV21,
    MOCK_SURFACE_FORMAT,
    MOCK_RGBA,
};

enum CodecMimeTypeMock : int32_t {
    VIDEO_H263 = 0,
    VIDEO_AVC,
    VIDEO_MPEG2,
    VIDEO_HEVC,
    VIDEO_MPEG4,
    VIDEO_VP8,
    VIDEO_VP9,
    AUDIO_AMR_NB,
    AUDIO_AMR_WB,
    AUDIO_MPEG,
    AUDIO_AAC,
    AUDIO_VORBIS,
    AUDIO_OPUS,
    AUDIO_FLAC,
    AUDIO_RAW,
};

class EnumMock : public NoCopyable {
public:
    virtual ~EnumMock() = default;
    virtual std::string GetMediaDescriptionKey(const MediaDescriptionKeyMock &key) const = 0;
    virtual int32_t GetVideoPixelFormat(const VideoPixelFormatMock &key) const = 0;
    virtual std::string GetCodecMimeType(const CodecMimeTypeMock &key) const = 0;
};
} // Media
} // OHOS
#endif // ENUM_MOCK_H