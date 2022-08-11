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

#ifndef AVFCODEC_INFO_NATIVE_MOCK_H
#define AVFCODEC_INFO_NATIVE_MOCK_H

#include "avcodec_mock.h"
#include "avcodec_info.h"

namespace OHOS {
namespace Media {
const std::map<AVCodecTypeMock, AVCodecType> AVCODEC_TYPE_INFOS = {
    {AVCODEC_TYPE_MOCK_NONE, AVCODEC_TYPE_NONE},
    {AVCODEC_TYPE_MOCK_VIDEO_ENCODER, AVCODEC_TYPE_VIDEO_ENCODER},
    {AVCODEC_TYPE_MOCK_VIDEO_DECODER, AVCODEC_TYPE_VIDEO_DECODER},
    {AVCODEC_TYPE_MOCK_AUDIO_ENCODER, AVCODEC_TYPE_AUDIO_ENCODER},
    {AVCODEC_TYPE_MOCK_AUDIO_DECODER, AVCODEC_TYPE_AUDIO_DECODER},
};

class AVCodecInfoNativeMock : public AVCodecInfoMock {
public:
    explicit AVCodecInfoNativeMock(std::shared_ptr<AVCodecInfo> codecInfo) : codecInfo_(codecInfo) {}
    AVCodecInfoNativeMock() = default;
    std::string GetName() const override;
    int32_t GetType() const override;
    std::string GetMimeType() const override;
    bool IsHardwareAccelerated() const override;
    bool IsSoftwareOnly() const override;
    bool IsVendor() const override;

private:
    std::shared_ptr<AVCodecInfo> codecInfo_ = nullptr;
};

class VideoCapsNativeMock : public VideoCapsMock {
public:
    explicit VideoCapsNativeMock(std::shared_ptr<VideoCaps> videoCaps) : videoCaps_(videoCaps) {}
    VideoCapsNativeMock() = default;
    std::shared_ptr<AVCodecInfoMock> GetCodecInfo() const override;
    RangeMock GetSupportedBitrate() const override;
    std::vector<int32_t> GetSupportedFormats() const override;
    int32_t GetSupportedHeightAlignment() const override;
    int32_t GetSupportedWidthAlignment() const override;
    RangeMock GetSupportedWidth() const override;
    RangeMock GetSupportedHeight() const override;
    std::vector<int32_t> GetSupportedProfiles() const override;
    std::vector<int32_t> GetSupportedLevels() const override;
    RangeMock GetSupportedEncodeQuality() const override;
    bool IsSizeSupported(int32_t width, int32_t height) const override;
    RangeMock GetSupportedFrameRate() const override;
    RangeMock GetSupportedFrameRatesFor(int32_t width, int32_t height) const override;
    bool IsSizeAndRateSupported(int32_t width, int32_t height, double frameRate) const override;
    RangeMock GetPreferredFrameRate(int32_t width, int32_t height) const override;
    std::vector<int32_t> GetSupportedBitrateMode() const override;
    RangeMock GetSupportedQuality() const override;
    RangeMock GetSupportedComplexity() const override;
    bool IsSupportDynamicIframe() const override;

private:
    std::shared_ptr<VideoCaps> videoCaps_ = nullptr;
};

class AudioCapsNativeMock : public AudioCapsMock {
public:
    explicit AudioCapsNativeMock(std::shared_ptr<AudioCaps> audioCaps) : audioCaps_(audioCaps) {}
    AudioCapsNativeMock() = default;
    std::shared_ptr<AVCodecInfoMock> GetCodecInfo() const override;
    RangeMock GetSupportedBitrate() const override;
    RangeMock GetSupportedChannel() const override;
    std::vector<int32_t> GetSupportedFormats() const override;
    std::vector<int32_t> GetSupportedSampleRates() const override;
    std::vector<int32_t> GetSupportedProfiles() const override;
    std::vector<int32_t> GetSupportedLevels() const override;
    RangeMock GetSupportedComplexity() const override;
private:
    std::shared_ptr<AudioCaps> audioCaps_ = nullptr;
};
} // Media
} // OHOS
#endif // AVFCODEC_INFO_NATIVE_MOCK_H