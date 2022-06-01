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

#include "media_capability_utils.h"
#include "media_capability_vcaps_napi.h"
#include "avcodec_list.h"
#include "media_log.h"
#include "media_errors.h"
#include "recorder_profiles.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaCapsNapiUtil"};
}

namespace OHOS {
namespace Media {
napi_status AddCodecInfo(napi_env env, napi_value result, std::shared_ptr<AVCodecInfo> info)
{
    CHECK_AND_RETURN_RET(info != nullptr, napi_generic_failure);

    napi_value obj = nullptr;
    napi_status status = napi_create_object(env, &obj);
    CHECK_AND_RETURN_RET(status == napi_ok, status);

    (void)CommonNapi::SetPropertyString(env, obj, "name", info->GetName());
    (void)CommonNapi::SetPropertyInt32(env, obj, "type", static_cast<int32_t>(info->GetType()));
    (void)CommonNapi::SetPropertyString(env, obj, "mimeType", info->GetMimeType());
    (void)CommonNapi::SetPropertyInt32(env, obj, "isHardwareAccelerated",
        static_cast<int32_t>(info->IsHardwareAccelerated()));
    (void)CommonNapi::SetPropertyInt32(env, obj, "isSoftwareOnly", static_cast<int32_t>(info->IsSoftwareOnly()));
    (void)CommonNapi::SetPropertyInt32(env, obj, "isVendor", static_cast<int32_t>(info->IsVendor()));

    napi_value nameStr = nullptr;
    status = napi_create_string_utf8(env, "codecInfo", NAPI_AUTO_LENGTH, &nameStr);
    CHECK_AND_RETURN_RET(status == napi_ok, napi_generic_failure);

    status = napi_set_property(env, result, nameStr, obj);
    CHECK_AND_RETURN_RET(status == napi_ok, napi_generic_failure);

    return napi_ok;
}

napi_status MediaJsAudioCapsDynamic::GetJsResult(napi_env env, napi_value &result)
{
    auto codecList = AVCodecListFactory::CreateAVCodecList();
    CHECK_AND_RETURN_RET(codecList != nullptr, napi_generic_failure);

    std::vector<std::shared_ptr<AudioCaps>> audioCaps;
    if (isDecoder_) {
        audioCaps = codecList->GetAudioDecoderCaps();
    } else {
        audioCaps = codecList->GetAudioEncoderCaps();
    }

    napi_status status = napi_create_object(env, &result);
    CHECK_AND_RETURN_RET(status == napi_ok, napi_generic_failure);

    for (auto it = audioCaps.begin(); it != audioCaps.end(); it++) {
        CHECK_AND_CONTINUE((*it) != nullptr);

        auto info = (*it)->GetCodecInfo();
        CHECK_AND_CONTINUE(info != nullptr);

        if (info->GetName() != name_) {
            continue;
        }

        (void)AddCodecInfo(env, result, info);

        Range range = (*it)->GetSupportedBitrate();
        (void)CommonNapi::AddRangeProperty(env, result, "supportedBitrate", range.minVal, range.maxVal);

        range = (*it)->GetSupportedChannel();
        (void)CommonNapi::AddRangeProperty(env, result, "supportedChannel", range.minVal, range.maxVal);

        range = (*it)->GetSupportedComplexity();
        (void)CommonNapi::AddRangeProperty(env, result, "supportedComplexity", range.minVal, range.maxVal);

        std::vector<int32_t> vec = (*it)->GetSupportedFormats();
        (void)CommonNapi::AddArrayProperty(env, result, "supportedFormats", vec);

        vec = (*it)->GetSupportedSampleRates();
        (void)CommonNapi::AddArrayProperty(env, result, "supportedSampleRates", vec);

        vec = (*it)->GetSupportedProfiles();
        (void)CommonNapi::AddArrayProperty(env, result, "supportedProfiles", vec);

        vec = (*it)->GetSupportedLevels();
        (void)CommonNapi::AddArrayProperty(env, result, "supportedLevels", vec);
    }

    return napi_ok;
}

napi_status MediaJsAudioCapsStatic::GetJsResult(napi_env env, napi_value &result)
{
    auto codecList = AVCodecListFactory::CreateAVCodecList();
    CHECK_AND_RETURN_RET(codecList != nullptr, napi_generic_failure);

    std::vector<std::shared_ptr<AudioCaps>> audioCaps;
    if (isDecoder_) {
        audioCaps = codecList->GetAudioDecoderCaps();
    } else {
        audioCaps = codecList->GetAudioEncoderCaps();
    }

    napi_status status = napi_create_array_with_length(env, audioCaps.size(), &result);
    CHECK_AND_RETURN_RET(status == napi_ok, status);

    int32_t index = 0;
    for (auto it = audioCaps.begin(); it != audioCaps.end(); it++) {
        CHECK_AND_CONTINUE((*it) != nullptr);

        napi_value obj = nullptr;
        status = napi_create_object(env, &obj);
        CHECK_AND_CONTINUE(status == napi_ok);

        Range range = (*it)->GetSupportedBitrate();
        (void)CommonNapi::AddRangeProperty(env, obj, "supportedBitrate", range.minVal, range.maxVal);

        range = (*it)->GetSupportedChannel();
        (void)CommonNapi::AddRangeProperty(env, obj, "supportedChannel", range.minVal, range.maxVal);

        range = (*it)->GetSupportedComplexity();
        (void)CommonNapi::AddRangeProperty(env, obj, "supportedComplexity", range.minVal, range.maxVal);

        std::vector<int32_t> vec = (*it)->GetSupportedFormats();
        (void)CommonNapi::AddArrayProperty(env, obj, "supportedFormats", vec);

        vec = (*it)->GetSupportedSampleRates();
        (void)CommonNapi::AddArrayProperty(env, obj, "supportedSampleRates", vec);

        vec = (*it)->GetSupportedProfiles();
        (void)CommonNapi::AddArrayProperty(env, obj, "supportedProfiles", vec);

        vec = (*it)->GetSupportedLevels();
        (void)CommonNapi::AddArrayProperty(env, obj, "supportedLevels", vec);

        auto codecInfo = (*it)->GetCodecInfo();
        if (codecInfo != nullptr) {
            (void)AddCodecInfo(env, obj, codecInfo);
        }

        (void)napi_set_element(env, result, index, obj);
        index++;
    }

    return napi_ok;
}

napi_status MediaJsVideoCapsStatic::GetJsResult(napi_env env, napi_value &result)
{
    auto codecList = AVCodecListFactory::CreateAVCodecList();
    CHECK_AND_RETURN_RET(codecList != nullptr, napi_generic_failure);

    std::vector<std::shared_ptr<VideoCaps>> caps;
    if (isDecoder_) {
        caps = codecList->GetVideoDecoderCaps();
    } else {
        caps = codecList->GetVideoEncoderCaps();
    }

    CHECK_AND_RETURN_RET(napi_create_array_with_length(env, caps.size(), &result) == napi_ok, napi_generic_failure);

    int32_t index = 0;
    for (auto it = caps.begin(); it != caps.end(); it++) {
        CHECK_AND_CONTINUE((*it) != nullptr);

        napi_value videoCaps = MediaCapabilityVCapsNapi::Create(env, *it);
        CHECK_AND_CONTINUE(videoCaps != nullptr);

        (void)napi_set_element(env, result, index, videoCaps);
        index++;
    }

    return napi_ok;
}

napi_status MediaJsVideoCapsDynamic::GetJsResult(napi_env env, napi_value &result)
{
    auto codecList = AVCodecListFactory::CreateAVCodecList();
    CHECK_AND_RETURN_RET(codecList != nullptr, napi_generic_failure);

    std::vector<std::shared_ptr<VideoCaps>> caps;
    if (isDecoder_) {
        caps = codecList->GetVideoDecoderCaps();
    } else {
        caps = codecList->GetVideoEncoderCaps();
    }

    for (auto it = caps.begin(); it != caps.end(); it++) {
        CHECK_AND_CONTINUE((*it) != nullptr);
        auto info = (*it)->GetCodecInfo();
        CHECK_AND_CONTINUE(info != nullptr);
        if (info->GetName() != name_) {
            continue;
        }

        result = MediaCapabilityVCapsNapi::Create(env, *it);
        CHECK_AND_RETURN_RET(result != nullptr, napi_generic_failure);
        break;
    }

    return napi_ok;
}

napi_status MediaJsAudioRecorderCapsArray::GetJsResult(napi_env env, napi_value &result)
{
    CHECK_AND_RETURN_RET(!value_.empty(), napi_generic_failure);

    napi_status status = napi_create_array_with_length(env, value_.size(), &result);
    CHECK_AND_RETURN_RET(status == napi_ok, status);

    int32_t index = 0;
    for (auto it = value_.begin(); it != value_.end(); it++) {
        CHECK_AND_CONTINUE((*it) != nullptr);

        napi_value obj = nullptr;
        status = napi_create_object(env, &obj);
        CHECK_AND_CONTINUE(status == napi_ok);

        std::string outString = (*it)->containerFormatType;
        (void)CommonNapi::AddStringProperty(env, obj, "outputFormat", outString);

        outString = (*it)->mimeType;
        (void)CommonNapi::AddStringProperty(env, obj, "audioEncoderMime", outString);

        Range range = (*it)->bitrate;
        (void)CommonNapi::AddRangeProperty(env, obj, "bitrateRange", range.minVal, range.maxVal);

        std::vector<int32_t> vec = (*it)->sampleRate;
        (void)CommonNapi::AddArrayProperty(env, obj, "sampleRates", vec);

        range = (*it)->channels;
        (void)CommonNapi::AddRangeProperty(env, obj, "channelRange", range.minVal, range.maxVal);

        (void)napi_set_element(env, result, index, obj);
        index++;
    }

    return status;
}

napi_status MediaJsVideoRecorderCapsArray::GetJsResult(napi_env env, napi_value &result)
{
    CHECK_AND_RETURN_RET(!value_.empty(), napi_generic_failure);

    napi_status status = napi_create_array_with_length(env, value_.size(), &result);
    CHECK_AND_RETURN_RET(status == napi_ok, status);

    int32_t index = 0;
    for (auto it = value_.begin(); it != value_.end(); it++) {
        CHECK_AND_CONTINUE((*it) != nullptr);

        napi_value obj = nullptr;
        status = napi_create_object(env, &obj);
        CHECK_AND_CONTINUE(status == napi_ok);

        std::string outString = (*it)->containerFormatType;
        (void)CommonNapi::AddStringProperty(env, obj, "outputFormat", outString);

        outString = (*it)->audioEncoderMime;
        (void)CommonNapi::AddStringProperty(env, obj, "audioEncoderMime", outString);

        Range range = (*it)->audioBitrateRange;
        (void)CommonNapi::AddRangeProperty(env, obj, "audioBitrateRange", range.minVal, range.maxVal);

        std::vector<int32_t> vec = (*it)->audioSampleRates;
        (void)CommonNapi::AddArrayProperty(env, obj, "audioSampleRates", vec);

        range = (*it)->audioChannelRange;
        (void)CommonNapi::AddRangeProperty(env, obj, "audioChannelRange", range.minVal, range.maxVal);

        range = (*it)->videoBitrateRange;
        (void)CommonNapi::AddRangeProperty(env, obj, "videoBitrateRange", range.minVal, range.maxVal);

        range = (*it)->videoFramerateRange;
        (void)CommonNapi::AddRangeProperty(env, obj, "videoFramerateRange", range.minVal, range.maxVal);

        outString = (*it)->videoEncoderMime;
        (void)CommonNapi::AddStringProperty(env, obj, "videoEncoderMime", outString);

        range = (*it)->videoWidthRange;
        (void)CommonNapi::AddRangeProperty(env, obj, "videoWidthRange", range.minVal, range.maxVal);

        range = (*it)->videoHeightRange;
        (void)CommonNapi::AddRangeProperty(env, obj, "videoHeightRange", range.minVal, range.maxVal);

        (void)napi_set_element(env, result, index, obj);
        index++;
    }

    return status;
}

napi_status MediaJsVideoRecorderProfile::GetJsResult(napi_env env, napi_value &result)
{
    napi_status ret = napi_ok;
    bool setRet = true;
    CHECK_AND_RETURN_RET(value_ != nullptr, napi_generic_failure);
    CHECK_AND_RETURN_RET((ret = napi_create_object(env, &result)) == napi_ok, ret);

    setRet = CommonNapi::SetPropertyInt32(env, result, "audioBitrate", value_->audioBitrate);
    CHECK_AND_RETURN_RET(setRet == true, napi_generic_failure);
    setRet = CommonNapi::SetPropertyInt32(env, result, "audioChannels", value_->audioChannels);
    CHECK_AND_RETURN_RET(setRet == true, napi_generic_failure);
    setRet = CommonNapi::SetPropertyString(env, result, "audioCodec", value_->audioCodec);
    CHECK_AND_RETURN_RET(setRet == true, napi_generic_failure);
    setRet = CommonNapi::SetPropertyInt32(env, result, "audioSampleRate", value_->audioSampleRate);
    CHECK_AND_RETURN_RET(setRet == true, napi_generic_failure);
    setRet = CommonNapi::SetPropertyInt32(env, result, "durationTime", value_->durationTime);
    CHECK_AND_RETURN_RET(setRet == true, napi_generic_failure);
    setRet = CommonNapi::SetPropertyString(env, result, "fileFormat", value_->containerFormatType);
    CHECK_AND_RETURN_RET(setRet == true, napi_generic_failure);
    setRet = CommonNapi::SetPropertyInt32(env, result, "qualityLevel", value_->qualityLevel);
    CHECK_AND_RETURN_RET(setRet == true, napi_generic_failure);
    setRet = CommonNapi::SetPropertyInt32(env, result, "videoBitrate", value_->videoBitrate);
    CHECK_AND_RETURN_RET(setRet == true, napi_generic_failure);
    setRet = CommonNapi::SetPropertyString(env, result, "videoCodec", value_->videoCodec);
    CHECK_AND_RETURN_RET(setRet == true, napi_generic_failure);
    setRet = CommonNapi::SetPropertyInt32(env, result, "videoFrameWidth", value_->videoFrameWidth);
    CHECK_AND_RETURN_RET(setRet == true, napi_generic_failure);
    setRet = CommonNapi::SetPropertyInt32(env, result, "videoFrameHeight", value_->videoFrameHeight);
    CHECK_AND_RETURN_RET(setRet == true, napi_generic_failure);
    setRet = CommonNapi::SetPropertyInt32(env, result, "videoFrameRate", value_->videoFrameRate);
    CHECK_AND_RETURN_RET(setRet == true, napi_generic_failure);

    return ret;
}

bool MediaCapabilityUtil::ExtractAudioRecorderProfile(napi_env env, napi_value profile, AudioRecorderProfile &result)
{
    CHECK_AND_RETURN_RET(profile != nullptr, false);
    bool ret = true;
    result.containerFormatType = CommonNapi::GetPropertyString(env, profile, "outputFormat");
    result.audioCodec = CommonNapi::GetPropertyString(env, profile, "audioEncoderMime");
    ret = CommonNapi::GetPropertyInt32(env, profile, "bitrate", result.audioBitrate);
    CHECK_AND_RETURN_RET(ret != false, false);
    ret = CommonNapi::GetPropertyInt32(env, profile, "sampleRate", result.audioSampleRate);
    CHECK_AND_RETURN_RET(ret != false, false);
    ret = CommonNapi::GetPropertyInt32(env, profile, "channel", result.audioChannels);
    CHECK_AND_RETURN_RET(ret != false, false);
    return ret;
}
} // namespace Media
} // namespace OHOS
