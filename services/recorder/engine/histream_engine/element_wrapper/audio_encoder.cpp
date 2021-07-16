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

#include "audio_encoder.h"
#include <gst/gst.h>
#include "errors.h"
#include "media_log.h"
#include "recorder_private_param.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioEncoder"};
}

namespace OHOS {
namespace Media {
int32_t AudioEncoder::Init()
{
    gstElem_ = gst_element_factory_make("fdkaacenc", name_.c_str());
    if (gstElem_ == nullptr) {
        MEDIA_LOGE("Create audio encoder gst element failed! sourceId: %{public}d", desc_.handle_);
        return ERR_INVALID_OPERATION;
    }

    return ERR_OK;
}

int32_t AudioEncoder::Configure(const RecorderParam &recParam)
{
    if (recParam.type == RecorderPublicParamType::AUD_ENC_FMT) {
        const AudEnc &param = static_cast<const AudEnc&>(recParam);
        encoderFormat_ = param.encFmt;
        if (encoderFormat_ == AudioCodecFormat::AUDIO_DEFAULT)  {
            encoderFormat_ = AudioCodecFormat::AAC_LC;
        }
        if (encoderFormat_ != AudioCodecFormat::AAC_LC) {
            MEDIA_LOGE("Currently unsupported audio encode format: %{public}d", encoderFormat_);
            return ERR_INVALID_VALUE;
        }
        MEDIA_LOGI("Set audio encode format: %{public}d", encoderFormat_);
        MarkParameter(param.type);
    }

    if (recParam.type == RecorderPublicParamType::AUD_BITRATE) {
        const AudBitRate &param = static_cast<const AudBitRate&>(recParam);
        if (param.bitRate <= 0) {
            MEDIA_LOGE("Audio encode bitrate is invalid: %{public}d", param.bitRate);
            return ERR_INVALID_VALUE;
        }
        g_object_set(gstElem_, "bitrate", param.bitRate, nullptr);
        MEDIA_LOGI("Set audio bitrate: %{public}d", param.bitRate);
        MarkParameter(param.type);
    }

    return ERR_OK;
}

int32_t AudioEncoder::CheckConfigReady()
{
    std::set<int32_t> expectedParam = { RecorderPublicParamType::AUD_ENC_FMT };
    bool configed = CheckAllParamsConfiged(expectedParam);
    CHECK_AND_RETURN_RET(configed == true, ERR_INVALID_OPERATION);

    return ERR_OK;
}

int32_t AudioEncoder::SetParameter(const RecorderParam &recParam)
{
    (void)recParam;
    return ERR_OK;
}

int32_t AudioEncoder::GetParameter(RecorderParam &recParam)
{
    (void)recParam;
    return ERR_OK;
}

REGISTER_RECORDER_ELEMENT(AudioEncoder);
}
}
