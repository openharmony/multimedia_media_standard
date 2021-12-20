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

#include "avcodeclist_engine_gst_impl.h"
#include "avcodec_ability_singleton.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecListEngineGstImpl"};
}

namespace OHOS {
namespace Media {
AVCodecListEngineGstImpl::AVCodecListEngineGstImpl()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecListEngineGstImpl::~AVCodecListEngineGstImpl()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

bool AVCodecListEngineGstImpl::IsSupportMimeType(const Format &format, const CapabilityData &data)
{
    std::string targetMimeType;
    if (!format.GetStringValue("codec_mime", targetMimeType)) {
        MEDIA_LOGD("Get MimeType form format failed");
        return false;
    }
    if (data.mimeType != targetMimeType) {
        return false;
    }
    return true;
}

bool AVCodecListEngineGstImpl::IsSupportBitrate(const Format &format, const CapabilityData &data)
{
    int32_t targetBitrate;
    if (!format.GetIntValue("bitrate", targetBitrate)) {
        MEDIA_LOGD("The bitrate of the format are not specified"); 
        return true;
    }
    if (data.bitrate.minVal > targetBitrate || data.bitrate.maxVal < targetBitrate) {
        return false;
    }
    return true;
}

bool AVCodecListEngineGstImpl::IsSupportSize(const Format &format, const CapabilityData &data)
{
    int32_t targetWidth;
    int32_t targetHeight;
    if ((!format.GetIntValue("width", targetWidth)) || (!format.GetIntValue("height", targetHeight))) {
        MEDIA_LOGD("The width and height of the format are not specified");
        return true;
    }
    if (data.width.minVal > targetWidth || data.width.maxVal < targetWidth
        || data.height.minVal > targetHeight || data.height.maxVal < targetHeight) {
        return false;
    }
    return true;
}

bool AVCodecListEngineGstImpl::IsSupportPixelFormat(const Format &format, const CapabilityData &data)
{
    if (data.codecType == AVCODEC_TYPE_AUDIO_ENCODER || data.codecType == AVCODEC_TYPE_AUDIO_DECODER) {
        return false;
    }
    int32_t targetPixelFormat;
    if (!format.GetIntValue("pixel_format", targetPixelFormat)) {
        MEDIA_LOGD("The pixel_format of the format are not specified");
        return true;
    }
    if (find(data.format.begin(), data.format.end(), targetPixelFormat) == data.format.end()) {
        return false;
    }
    return true;
}

bool AVCodecListEngineGstImpl::IsSupportFrameRate(const Format &format, const CapabilityData &data)
{
    int32_t targetFrameRate;
    if (!format.GetIntValue("frame_rate", targetFrameRate)) {
        MEDIA_LOGD("The frame_rate of the format are not specified"); 
        return true;
    }
    if (data.frameRate.minVal * 100 > targetFrameRate || data.frameRate.maxVal * 100 < targetFrameRate) {
        return false;
    }
    return true;
}

bool AVCodecListEngineGstImpl::IsSupportChannel(const Format &format, const CapabilityData &data)
{
    if (data.codecType == AVCODEC_TYPE_VIDEO_ENCODER || data.codecType == AVCODEC_TYPE_VIDEO_DECODER) {
        return false;
    }
    int32_t targetChannel;
    if (!format.GetIntValue("channel_count", targetChannel)) {
        MEDIA_LOGD("The channel_count of the format are not specified"); 
        return true;
    }
    if (data.channels.minVal > targetChannel || data.channels.maxVal < targetChannel) {
        return false;
    }
    return true;
}

bool AVCodecListEngineGstImpl::IsSupportSampleRate(const Format &format, const CapabilityData &data)
{
    if (data.codecType == AVCODEC_TYPE_VIDEO_ENCODER || data.codecType == AVCODEC_TYPE_VIDEO_DECODER) {
        return false;
    }
    int32_t targetSampleRate;
    if (!format.GetIntValue("samplerate", targetSampleRate)) {
        MEDIA_LOGD("The samplerate of the format are not specified"); 
        return true;
    }

    if (find(data.sampleRate.begin(), data.sampleRate.end(), targetSampleRate) == data.sampleRate.end()) {
        return false;
    }
    return true;
}

bool AVCodecListEngineGstImpl::IsSupportMediaType(const Format &format, const CapabilityData &data)
{
    int32_t targetMediaType;
    format.GetIntValue("track_type", targetMediaType);
    if ((data.codecType == AVCODEC_TYPE_VIDEO_ENCODER || data.codecType == AVCODEC_TYPE_VIDEO_DECODER)
        && targetMediaType != MEDIA_TYPE_VID) {
        return false;
    }

    if ((data.codecType == AVCODEC_TYPE_AUDIO_ENCODER || data.codecType == AVCODEC_TYPE_AUDIO_DECODER)
        && targetMediaType != MEDIA_TYPE_AUD) {
        return false;
    }
    return true;
}


std::string AVCodecListEngineGstImpl::FindTargetCodec(const Format &format,
    const std::vector<CapabilityData> &capabilityDataArray, const AVCodecType &codecType)
{
    for (auto iter = capabilityDataArray.begin(); iter != capabilityDataArray.end(); ++iter) {
        if ((*iter).codecType != codecType || !IsSupportMimeType(format, *iter) ||
            !IsSupportBitrate(format, *iter) || !IsSupportSize(format, *iter) ||
            !IsSupportPixelFormat(format, *iter) || !IsSupportFrameRate(format, *iter)  ||
            !IsSupportSampleRate(format, *iter) || !IsSupportChannel(format, *iter) ||
            !IsSupportMediaType(format, *iter)) {
            continue;
        }
        return (*iter).codecName;
    }
    return "";
}

std::string AVCodecListEngineGstImpl::FindVideoDecoder(const Format &format)
{
    std::vector<CapabilityData> capabilityDataArray = GetCodecCapabilityInfos();
    return FindTargetCodec(format, capabilityDataArray, AVCODEC_TYPE_VIDEO_DECODER);
}

std::string AVCodecListEngineGstImpl::FindVideoEncoder(const Format &format)
{
    std::vector<CapabilityData> capabilityDataArray = GetCodecCapabilityInfos();
    return FindTargetCodec(format, capabilityDataArray, AVCODEC_TYPE_VIDEO_ENCODER);
}

std::string AVCodecListEngineGstImpl::FindAudioDecoder(const Format &format)
{
    std::vector<CapabilityData> capabilityDataArray = GetCodecCapabilityInfos();
    return FindTargetCodec(format, capabilityDataArray, AVCODEC_TYPE_AUDIO_DECODER);
}

std::string AVCodecListEngineGstImpl::FindAudioEncoder(const Format &format)
{
    std::vector<CapabilityData> capabilityDataArray = GetCodecCapabilityInfos();
    return FindTargetCodec(format, capabilityDataArray, AVCODEC_TYPE_AUDIO_ENCODER);
}


std::vector<CapabilityData> AVCodecListEngineGstImpl::GetCodecCapabilityInfos()
{
    AVCodecAbilitySingleton& codecAbilityInstance = AVCodecAbilitySingleton::GetInstance();
    
    bool ret = codecAbilityInstance.ParseCodecXml();
    if (!ret) {
        MEDIA_LOGD("ParseCodecXml failed");
    }

    return codecAbilityInstance.capabilityDataArray_;
}
} // Media
} // OHOS