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

#ifndef AVFCODEC_INFO_NATIVE_MOCK_H
#define AVFCODEC_INFO_NATIVE_MOCK_H

#include "avcodec_mock.h"
#include "avcodec_info.h"

namespace OHOS {
namespace Media {
const std::map<AVCodecTypeMock, AVCodecType>  AVCODEC_TYPE_INFOS = {
    {AVCODEC_TYPE_MOCK_NONE, AVCODEC_TYPE_NONE},
    {AVCODEC_TYPE_MOCK_VIDEO_ENCODER, AVCODEC_TYPE_VIDEO_ENCODER},
    {AVCODEC_TYPE_MOCK_VIDEO_DECODER, AVCODEC_TYPE_VIDEO_DECODER},
    {AVCODEC_TYPE_MOCK_AUDIO_ENCODER, AVCODEC_TYPE_AUDIO_ENCODER},
    {AVCODEC_TYPE_MOCK_AUDIO_DECODER, AVCODEC_TYPE_AUDIO_DECODER},
};

struct RangeNativeMock : RangeMock {
public:
    explicit RangeNativeMock(Range const &range) : range_(range) {}
    RangeNativeMock() = default;
    int32_t minVal;
    int32_t maxVal;
    Range range_;
};

class AVCodecInfoNativeMock : public AVCodecInfoMock {
public:
    explicit AVCodecInfoNativeMock(std::shared_ptr<AVCodecInfo> codecInfo) : codecInfo_(codecInfo) {}
    AVCodecInfoNativeMock() = default;
    std::string GetName();
    int32_t GetType();
    std::string GetMimeType();
    bool IsHardwareAccelerated();
    bool IsSoftwareOnly();
    bool IsVendor();

private:
    std::shared_ptr<AVCodecInfo> codecInfo_ = nullptr;
};

class VideoCapsNativeMock : public VideoCapsMock {
public:
    explicit VideoCapsNativeMock(std::shared_ptr<VideoCaps> videoCaps) : videoCaps_(videoCaps) {}
    VideoCapsNativeMock() = default;
    std::shared_ptr<AVCodecInfoMock> GetCodecInfo();
    RangeMock GetSupportedBitrate();
    std::vector<int32_t> GetSupportedFormats();
    int32_t GetSupportedHeightAlignment();
    int32_t GetSupportedWidthAlignment();
    RangeMock GetSupportedWidth();
    RangeMock GetSupportedHeight();
    std::vector<int32_t> GetSupportedProfiles();
    std::vector<int32_t> GetSupportedLevels();
    RangeMock GetSupportedEncodeQuality();
    bool IsSizeSupported(int32_t width, int32_t height);
    RangeMock GetSupportedFrameRate();
    RangeMock GetSupportedFrameRatesFor(int32_t width, int32_t height);
    bool IsSizeAndRateSupported(int32_t width, int32_t height, double frameRate);
    RangeMock GetPreferredFrameRate(int32_t width, int32_t height);
    std::vector<int32_t> GetSupportedBitrateMode();
    RangeMock GetSupportedQuality();
    RangeMock GetSupportedComplexity();
    bool IsSupportDynamicIframe();

private:
    std::shared_ptr<VideoCaps> videoCaps_ = nullptr;
};

class AudioCapsNativeMock : public AudioCapsMock {
public:
    explicit AudioCapsNativeMock(std::shared_ptr<AudioCaps> audioCaps) : audioCaps_(audioCaps) {}
    AudioCapsNativeMock() = default;
    std::shared_ptr<AVCodecInfoMock> GetCodecInfo();
    RangeMock GetSupportedBitrate();
    RangeMock GetSupportedChannel();
    std::vector<int32_t> GetSupportedFormats();
    std::vector<int32_t> GetSupportedSampleRates();
    std::vector<int32_t> GetSupportedProfiles();
    std::vector<int32_t> GetSupportedLevels();
    RangeMock GetSupportedComplexity();
private:
    std::shared_ptr<AudioCaps> audioCaps_ = nullptr;
};
} // Media
} // OHOS
#endif // AVFCODEC_INFO_NATIVE_MOCK_H