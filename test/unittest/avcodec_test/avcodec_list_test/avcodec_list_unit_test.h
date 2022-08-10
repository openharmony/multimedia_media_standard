/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef AVCODEC_LIST_UNIT_TEST_H
#define AVCODEC_LIST_UNIT_TEST_H

#include "gtest/gtest.h"
#include "avcodec_mock.h"
#include "enum_mock.h"

namespace OHOS {
namespace Media {
constexpr uint32_t DEFAULT_WIDTH = 1920;
constexpr uint32_t DEFAULT_HEIGHT = 1080;
constexpr uint32_t MIN_WIDTH = 2;
constexpr uint32_t MIN_HEIGHT = 2;
constexpr uint32_t MAX_WIDTH = 3840;
constexpr uint32_t MAX_HEIGHT = 2160;
constexpr uint32_t MAX_FRAME_RATE = 30;
constexpr uint32_t MAX_VIDEO_BITRATE = 3000000;
constexpr uint32_t MAX_AUDIO_BITRATE = 384000;
constexpr uint32_t DEFAULT_SAMPLE_RATE = 8000;
constexpr uint32_t MAX_CHANNEL_COUNT = 2;
constexpr uint32_t MAX_CHANNEL_COUNT_VORBIS = 7;
class AVCodecListUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
protected:
    std::shared_ptr<AVCodecListMock> avCodecList_ = nullptr;
    std::shared_ptr<EnumMock> enum_ = nullptr;
    void CheckAVDecH264(const std::shared_ptr<VideoCapsMock> &videoCaps) const;
    void CheckAVDecH263(const std::shared_ptr<VideoCapsMock> &videoCaps) const;
    void CheckAVDecMpeg2Video(const std::shared_ptr<VideoCapsMock> &videoCaps) const;
    void CheckAVDecMpeg4(const std::shared_ptr<VideoCapsMock> &videoCaps) const;
    void CheckAVEncMpeg4(const std::shared_ptr<VideoCapsMock> &videoCaps) const;
    void CheckVideoCaps(const std::shared_ptr<VideoCapsMock> &videoCaps) const;
    void CheckVideoCapsArray(const std::vector<std::shared_ptr<VideoCapsMock>> &videoCapsArray) const;
    void CheckAVDecMP3(const std::shared_ptr<AudioCapsMock> &audioCaps) const;
    void CheckAVDecAAC(const std::shared_ptr<AudioCapsMock> &audioCaps) const;
    void CheckAVDecVorbis(const std::shared_ptr<AudioCapsMock> &audioCaps) const;
    void CheckAVDecFlac(const std::shared_ptr<AudioCapsMock> &audioCaps) const;
    void CheckAVDecOpus(const std::shared_ptr<AudioCapsMock> &audioCaps) const;
    void CheckAVEncAAC(const std::shared_ptr<AudioCapsMock> &audioCaps) const;
    void CheckAVEncOpus(const std::shared_ptr<AudioCapsMock> &audioCaps) const;
    void CheckAudioCaps(const std::shared_ptr<AudioCapsMock> &audioCaps) const;
    void CheckAudioCapsArray(const std::vector<std::shared_ptr<AudioCapsMock>> &audioCapsArray) const;
    std::string codecMimeKey_;
    std::string bitrateKey_;
    std::string widthKey_;
    std::string heightKey_;
    std::string pixelFormatKey_;
    std::string frameRateKey_;
    std::string channelCountKey_;
    std::string sampleRateKey_;
};
}
}
#endif
