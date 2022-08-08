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

#include "avcodec_info_native_mock.h"

namespace OHOS {
namespace Media {
std::string AVCodecInfoNativeMock::GetName() const
{
    if (codecInfo_ != nullptr) {
        return codecInfo_->GetName();
    }
    return nullptr;
}

int32_t AVCodecInfoNativeMock::GetType() const
{
    if (codecInfo_ != nullptr) {
        return codecInfo_->GetType();
    }
    return -1;
}

std::string AVCodecInfoNativeMock::GetMimeType() const
{
    if (codecInfo_ != nullptr) {
        return codecInfo_->GetMimeType();
    }
    return "";
}

bool AVCodecInfoNativeMock::IsHardwareAccelerated() const
{
    if (codecInfo_ != nullptr) {
        return codecInfo_->IsHardwareAccelerated();
    }
    return false;
}

bool AVCodecInfoNativeMock::IsSoftwareOnly() const
{
    if (codecInfo_ != nullptr) {
        return codecInfo_->IsSoftwareOnly();
    }
    return false;
}

bool AVCodecInfoNativeMock::IsVendor() const
{
    if (codecInfo_ != nullptr) {
        return codecInfo_->IsVendor();
    }
    return false;
}

std::shared_ptr<AVCodecInfoMock> VideoCapsNativeMock::GetCodecInfo() const
{
    if (videoCaps_ != nullptr) {
        return std::make_shared<AVCodecInfoNativeMock>(videoCaps_->GetCodecInfo());
    }
    return nullptr;
}

RangeMock VideoCapsNativeMock::GetSupportedBitrate() const
{
    RangeMock bitrate = RangeMock();
    if (videoCaps_ != nullptr) {
        bitrate = RangeNativeMock(videoCaps_->GetSupportedBitrate());
    }
    return bitrate;
}

std::vector<int32_t> VideoCapsNativeMock::GetSupportedFormats() const
{
    std::vector<int32_t> formats;
    if (videoCaps_ != nullptr) {
        formats = videoCaps_->GetSupportedFormats();
    }
    return formats;
}

int32_t VideoCapsNativeMock::GetSupportedHeightAlignment() const
{
    if (videoCaps_ != nullptr) {
        return videoCaps_->GetSupportedHeightAlignment();
    }
    return -1;
}

int32_t VideoCapsNativeMock::GetSupportedWidthAlignment() const
{
    if (videoCaps_ != nullptr) {
        return videoCaps_->GetSupportedWidthAlignment();
    }
    return -1;
}

RangeMock VideoCapsNativeMock::GetSupportedWidth() const
{
    RangeMock width = RangeMock();
    if (videoCaps_ != nullptr) {
        width = RangeNativeMock(videoCaps_->GetSupportedWidth());
    }
    return width;
}

RangeMock VideoCapsNativeMock::GetSupportedHeight() const
{
    RangeMock height = RangeMock();
    if (videoCaps_ != nullptr) {
        height = RangeNativeMock(videoCaps_->GetSupportedHeight());
    }
    return height;
}

std::vector<int32_t> VideoCapsNativeMock::GetSupportedProfiles() const
{
    std::vector<int32_t> profiles;
    if (videoCaps_ != nullptr) {
        profiles = videoCaps_->GetSupportedProfiles();
    }
    return profiles;
}

std::vector<int32_t> VideoCapsNativeMock::GetSupportedLevels() const
{
    std::vector<int32_t> levels;
    if (videoCaps_ != nullptr) {
        levels = videoCaps_->GetSupportedLevels();
    }
    return levels;
}

RangeMock VideoCapsNativeMock::GetSupportedEncodeQuality() const
{
    RangeMock quality = RangeMock();
    if (videoCaps_ != nullptr) {
        quality = RangeNativeMock(videoCaps_->GetSupportedEncodeQuality());
    }
    return quality;
}

bool VideoCapsNativeMock::IsSizeSupported(int32_t width, int32_t height) const
{
    if (videoCaps_ != nullptr) {
        return videoCaps_->IsSizeSupported(width, height);
    }
    return false;
}

RangeMock VideoCapsNativeMock::GetSupportedFrameRate() const
{
    RangeMock frameRates = RangeMock();
    if (videoCaps_ != nullptr) {
        frameRates = RangeNativeMock(videoCaps_->GetSupportedFrameRate());
    }
    return frameRates;
}

RangeMock VideoCapsNativeMock::GetSupportedFrameRatesFor(int32_t width, int32_t height) const
{
    RangeMock frameRates = RangeMock();
    if (videoCaps_ != nullptr) {
        frameRates = RangeNativeMock(videoCaps_->GetSupportedFrameRatesFor(width, height));
    }
    return frameRates;
}

bool VideoCapsNativeMock::IsSizeAndRateSupported(int32_t width, int32_t height, double frameRate) const
{
    if (videoCaps_ != nullptr) {
        return videoCaps_->IsSizeAndRateSupported(width, height, frameRate);
    }
    return false;
}

RangeMock VideoCapsNativeMock::GetPreferredFrameRate(int32_t width, int32_t height) const
{
    RangeMock frameRate = RangeMock();
    if (videoCaps_ != nullptr) {
        frameRate = RangeNativeMock(videoCaps_->GetPreferredFrameRate(width, height));
    }
    return frameRate;
}

std::vector<int32_t> VideoCapsNativeMock::GetSupportedBitrateMode() const
{
    std::vector<int32_t> bitrateMode;
    if (videoCaps_ != nullptr) {
        bitrateMode = videoCaps_->GetSupportedBitrateMode();
    }
    return bitrateMode;
}

RangeMock VideoCapsNativeMock::GetSupportedQuality() const
{
    RangeMock quality = RangeMock();
    if (videoCaps_ != nullptr) {
        quality = RangeNativeMock(videoCaps_->GetSupportedQuality());
    }
    return quality;
}

RangeMock VideoCapsNativeMock::GetSupportedComplexity() const
{
    RangeMock complexity = RangeMock();
    if (videoCaps_ != nullptr) {
        complexity = RangeNativeMock(videoCaps_->GetSupportedComplexity());
    }
    return complexity;
}

bool VideoCapsNativeMock::IsSupportDynamicIframe() const
{
    if (videoCaps_ != nullptr) {
        return videoCaps_->IsSupportDynamicIframe();
    }
    return false;
}

std::shared_ptr<AVCodecInfoMock> AudioCapsNativeMock::GetCodecInfo() const
{
    if (audioCaps_ != nullptr) {
        return std::make_shared<AVCodecInfoNativeMock>(audioCaps_->GetCodecInfo());
    }
    return nullptr;
}

RangeMock AudioCapsNativeMock::GetSupportedBitrate() const
{
    RangeMock bitrate = RangeMock();
    if (audioCaps_ != nullptr) {
        bitrate = RangeNativeMock(audioCaps_->GetSupportedBitrate());
    }
    return bitrate;
}

RangeMock AudioCapsNativeMock::GetSupportedChannel() const
{
    RangeMock channal = RangeMock();
    if (audioCaps_ != nullptr) {
        channal = RangeNativeMock(audioCaps_->GetSupportedChannel());
    }
    return channal;
}

std::vector<int32_t> AudioCapsNativeMock::GetSupportedFormats() const
{
    std::vector<int32_t> formats;
    if (audioCaps_ != nullptr) {
        formats = audioCaps_->GetSupportedFormats();
    }
    return formats;
}

std::vector<int32_t> AudioCapsNativeMock::GetSupportedSampleRates() const
{
    std::vector<int32_t> sampleRates;
    if (audioCaps_ != nullptr) {
        sampleRates = audioCaps_->GetSupportedSampleRates();
    }
    return sampleRates;
}

std::vector<int32_t> AudioCapsNativeMock::GetSupportedProfiles() const
{
    std::vector<int32_t> profiles;
    if (audioCaps_ != nullptr) {
        profiles = audioCaps_->GetSupportedProfiles();
    }
    return profiles;
}

std::vector<int32_t> AudioCapsNativeMock::GetSupportedLevels() const
{
    std::vector<int32_t> levels;
    if (audioCaps_ != nullptr) {
        levels = audioCaps_->GetSupportedLevels();
    }
    return levels;
}

RangeMock AudioCapsNativeMock::GetSupportedComplexity() const
{
    RangeMock complexity = RangeMock();
    if (audioCaps_ != nullptr) {
        complexity = RangeNativeMock(audioCaps_->GetSupportedComplexity());
    }
    return complexity;
}
} // Media
} // OHOS