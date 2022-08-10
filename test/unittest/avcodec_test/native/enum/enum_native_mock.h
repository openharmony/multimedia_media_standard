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

#ifndef ENUM_NATIVE_MOCK_H
#define ENUM_NATIVE_MOCK_H

#include "avcodec_mock.h"
#include "enum_mock.h"
#include "media_description.h"
#include "av_common.h"
#include "avcodec_info.h"

namespace OHOS {
namespace Media {
const std::map<MediaDescriptionKeyMock, std::string_view>MEDIA_DESCRIPTION_KEY_INFOS = {
    {MOCK_MD_KEY_TRACK_INDEX, MediaDescriptionKey::MD_KEY_TRACK_INDEX},
    {MOCK_MD_KEY_TRACK_TYPE, MediaDescriptionKey::MD_KEY_TRACK_TYPE},
    {MOCK_MD_KEY_CODEC_MIME, MediaDescriptionKey::MD_KEY_CODEC_MIME},
    {MOCK_MD_KEY_DURATION, MediaDescriptionKey::MD_KEY_DURATION},
    {MOCK_MD_KEY_BITRATE, MediaDescriptionKey::MD_KEY_BITRATE},
    {MOCK_MD_KEY_WIDTH, MediaDescriptionKey::MD_KEY_WIDTH},
    {MOCK_MD_KEY_HEIGHT, MediaDescriptionKey::MD_KEY_HEIGHT},
    {MOCK_MD_KEY_PIXEL_FORMAT, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT},
    {MOCK_MD_KEY_FRAME_RATE, MediaDescriptionKey::MD_KEY_FRAME_RATE},
    {MOCK_MD_KEY_CHANNEL_COUNT, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT},
    {MOCK_MD_KEY_SAMPLE_RATE, MediaDescriptionKey::MD_KEY_SAMPLE_RATE},
};

const std::map<VideoPixelFormatMock, VideoPixelFormat> VIDEO_PIXEL_FORMAT_INFOS = {
    {MOCK_YUVI420, YUVI420},
    {MOCK_NV12, NV12},
    {MOCK_NV21, NV21},
    {MOCK_SURFACE_FORMAT, SURFACE_FORMAT},
    {MOCK_RGBA, RGBA},
};

const std::map<CodecMimeTypeMock, std::string_view> CODEC_MIME_INFOS = {
    {VIDEO_H263, CodecMimeType::VIDEO_H263},
    {VIDEO_AVC, CodecMimeType::VIDEO_AVC},
    {VIDEO_MPEG2, CodecMimeType::VIDEO_MPEG2},
    {VIDEO_HEVC, CodecMimeType::VIDEO_HEVC},
    {VIDEO_MPEG4, CodecMimeType::VIDEO_MPEG4},
    {VIDEO_VP8, CodecMimeType::VIDEO_VP8},
    {VIDEO_VP9, CodecMimeType::VIDEO_VP9},
    {AUDIO_AMR_NB, CodecMimeType::AUDIO_AMR_NB},
    {AUDIO_AMR_WB, CodecMimeType::AUDIO_AMR_WB},
    {AUDIO_MPEG, CodecMimeType::AUDIO_MPEG},
    {AUDIO_AAC, CodecMimeType::AUDIO_AAC},
    {AUDIO_VORBIS, CodecMimeType::AUDIO_VORBIS},
    {AUDIO_OPUS, CodecMimeType::AUDIO_OPUS},
    {AUDIO_FLAC, CodecMimeType::AUDIO_FLAC},
    {AUDIO_RAW, CodecMimeType::AUDIO_RAW},
};

class EnumNativeMock : public EnumMock {
public:
    EnumNativeMock() = default;
    std::string GetMediaDescriptionKey(const MediaDescriptionKeyMock &key) const override;
    int32_t GetVideoPixelFormat(const VideoPixelFormatMock &key) const override;
    std::string GetCodecMimeType(const CodecMimeTypeMock &key) const override;
};
} // Media
} // OHOS
#endif // ENUM_NATIVE_MOCK_H