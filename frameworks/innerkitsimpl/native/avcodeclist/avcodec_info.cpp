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

#include "avcodec_info.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CodecInfoImpl"};
}
namespace OHOS {
namespace Media {
VideoCaps::VideoCaps(CapabilityData &capabilityData)
    : data_(capabilityData)
{
    MEDIA_LOGD("VideoCaps:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

VideoCaps::~VideoCaps()
{
    MEDIA_LOGD("VideoCaps:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

std::shared_ptr<AVCodecInfo> VideoCaps::GetCodecInfo()
{
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>((data_));
    CHECK_AND_RETURN_RET_LOG(codecInfo != nullptr, nullptr, "create codecInfo failed");

    return codecInfo;
}

Range VideoCaps::GetSupportedBitrate() 
{
    return data_.bitrate;
}

std::vector<int32_t> VideoCaps::GetSupportedFormats()
{
    std::vector<int32_t> format = data_.format;
    CHECK_AND_RETURN_RET_LOG(format.size() != 0, format, "GetSupportedFormats failed: format is null");
    return format;
}

int32_t VideoCaps::GetSupportedHeightAlignment()
{
    return data_.alignment.maxVal;
}

int32_t VideoCaps::GetSupportedWidthAlignment()
{
    return data_.alignment.minVal;
}

Range VideoCaps::GetSupportedWidth() 
{
    return data_.width;
}

Range VideoCaps::GetSupportedHeight()  
{
    return data_.height;
}

std::vector<int32_t> VideoCaps::GetSupportedProfiles()
{
    std::vector<int32_t> profiles = data_.profiles;
    CHECK_AND_RETURN_RET_LOG(profiles.size() != 0, profiles, "GetSupportedProfiles failed: profiles is null");
    return profiles;
}

std::vector<int32_t> VideoCaps::GetSupportedLevels()
{
    std::vector<int32_t> levels = data_.levels;
    CHECK_AND_RETURN_RET_LOG(levels.size() != 0, levels, "GetSupportedLevels failed: levels is null");
    return levels;
}

Range VideoCaps::GetSupportedEncodeQuality()
{
    return data_.encodeQuality;
}

bool VideoCaps::IsSizeSupported(int32_t width, int32_t height) 
{
    if (data_.width.minVal > width || data_.width.maxVal < width ||
        data_.height.minVal > height || data_.height.maxVal < height) {
            return false;
    }
    return true;
}

Range VideoCaps::GetSupportedFrameRate()
{
    return data_.frameRate;
}

Range VideoCaps::GetSupportedFrameRatesFor(int32_t width, int32_t height)
{
    Range range;
    if (!IsSizeSupported(width, height)) {
        MEDIA_LOGD("The %{public}s can not support of:%{public}d * %{public}d", data_.codecName.c_str(), width, height);
        return range;
    }
    // get supported frame rate range for the specified width and height
    return range;
}

bool VideoCaps::IsSizeAndRateSupported(int32_t width, int32_t height, double frameRate)
{
    if (!IsSizeSupported(width, height)) {
        MEDIA_LOGD("The %{public}s can not support of:%{public}d * %{public}d", data_.codecName.c_str(), width, height);
        return false;
    }
    if (data_.frameRate.minVal > frameRate || data_.frameRate.maxVal < frameRate) {
        MEDIA_LOGD("The %{public}s can not support frameRate:%{public}lf", data_.codecName.c_str(), frameRate);
        return false;
    }
    return true;
}

Range VideoCaps::GetPreferredFrameRate(int32_t width, int32_t height)
{
    Range range;
    if (!IsSizeSupported(width, height)) {
        MEDIA_LOGD("The %{public}s can not support of:%{public}d * %{public}d", data_.codecName.c_str(), width, height);
        return range;
    }
    // get preferred frame rate range for the specified width and height
    return range;
}

std::vector<int32_t> VideoCaps::GetSupportedBitrateMode() 
{
    std::vector<int32_t> bitrateMode = data_.bitrateMode;
    CHECK_AND_RETURN_RET_LOG(bitrateMode.size() != 0, bitrateMode, "GetSupportedBitrateMode failed: get null");
    return bitrateMode;
}

Range VideoCaps::GetSupportedQuality()
{
    return data_.quality;
}

Range VideoCaps::GetSupportedComplexity()
{
    return data_.complexity;
}

bool VideoCaps::IsSupportDynamicIframe()
{
    return false;
}

AudioCaps::AudioCaps(CapabilityData &capabilityData)
    : data_(capabilityData)
{
    MEDIA_LOGD("AudioCaps:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AudioCaps::~AudioCaps()
{
    MEDIA_LOGD("AudioCaps:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

std::shared_ptr<AVCodecInfo> AudioCaps::GetCodecInfo() 
{
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>((data_));
    CHECK_AND_RETURN_RET_LOG(codecInfo != nullptr, nullptr, "create codecInfo failed");
    return codecInfo;
}

Range AudioCaps::GetSupportedBitrate() 
{
    return data_.bitrate;
}

Range AudioCaps::GetSupportedChannel()
{
    return data_.channels;
}

std::vector<int32_t> AudioCaps::GetSupportedFormats()
{
    std::vector<int32_t> format = data_.format;
    CHECK_AND_RETURN_RET_LOG(format.size() != 0, format, "GetSupportedFormats failed: format is null");
    return format;
}

std::vector<int32_t> AudioCaps::GetSupportedSampleRates()
{
    std::vector<int32_t> sampleRate = data_.sampleRate;
    CHECK_AND_RETURN_RET_LOG(sampleRate.size() != 0, sampleRate, "GetSupportedSampleRates failed: sampleRate is null");
    return sampleRate;
}

std::vector<int32_t> AudioCaps::GetSupportedProfiles()
{
    std::vector<int32_t> profiles = data_.profiles;
    CHECK_AND_RETURN_RET_LOG(profiles.size() != 0, profiles, "GetSupportedProfiles failed: profiles is null");
    return profiles;
}

std::vector<int32_t> AudioCaps::GetSupportedLevels()
{
    std::vector<int32_t> levels = data_.levels;
    CHECK_AND_RETURN_RET_LOG(levels.size() != 0, levels, "GetSupportedLevels failed: levels is null");
    return levels;
}

Range AudioCaps::GetSupportedComplexity()
{
    return data_.complexity;
}

AVCodecInfo::AVCodecInfo(CapabilityData &capabilityData)
    : data_(capabilityData)
{
    MEDIA_LOGD("AVCodecInfo:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecInfo::~AVCodecInfo()
{
    MEDIA_LOGD("AVCodecInfo:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

std::string AVCodecInfo::GetName()
{
    std::string name = data_.codecName;
    CHECK_AND_RETURN_RET_LOG(name != "", "", "get codec name is null");
    return name;
}

AVCodecType AVCodecInfo::GetType()
{
    AVCodecType codecType = AVCodecType(data_.codecType);
    CHECK_AND_RETURN_RET_LOG(codecType != AVCODEC_TYPE_NONE, AVCODEC_TYPE_NONE, "can not find codec type");
    return codecType;
}

std::string AVCodecInfo::GetMimeType()
{
    std::string mimeType = data_.mimeType;
    CHECK_AND_RETURN_RET_LOG(mimeType != "", "", "get mimeType is null");
    return mimeType;
}

bool AVCodecInfo::IsHardwareAccelerated()
{
    // get from hdi plugin
    return false;
}

bool AVCodecInfo::IsSoftwareOnly()
{
    // get from hdi plugin
    return true;
}

bool AVCodecInfo::IsVendor()
{
    return data_.isVendor;
}
}
}