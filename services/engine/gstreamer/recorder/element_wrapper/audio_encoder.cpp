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
#include "media_errors.h"
#include "media_log.h"
#include "recorder_private_param.h"
#include "i_recorder_engine.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioEncoder"};
}

namespace OHOS {
namespace Media {
int32_t AudioEncoder::Init()
{
    gstElem_ = gst_element_factory_make("avenc_aac", name_.c_str());
    if (gstElem_ == nullptr) {
        MEDIA_LOGE("Create audio encoder gst element failed! sourceId: %{public}d", desc_.handle_);
        return MSERR_INVALID_OPERATION;
    }

    MEDIA_LOGI("use avenc_aac");

    return MSERR_OK;
}

int32_t AudioEncoder::Configure(const RecorderParam &recParam)
{
    if (recParam.type == RecorderPublicParamType::AUD_ENC_FMT) {
        const AudEnc &param = static_cast<const AudEnc &>(recParam);
        encoderFormat_ = param.encFmt;
        if (encoderFormat_ == AudioCodecFormat::AUDIO_DEFAULT)  {
            encoderFormat_ = AudioCodecFormat::AAC_LC;
        }
        if (encoderFormat_ != AudioCodecFormat::AAC_LC) {
            MEDIA_LOGE("Currently unsupported audio encode format: %{public}d", encoderFormat_);
            return MSERR_INVALID_VAL;
        }
        MEDIA_LOGI("Set audio encode format: %{public}d", encoderFormat_);
        MarkParameter(param.type);
    }

    if (recParam.type == RecorderPublicParamType::AUD_BITRATE) {
        const AudBitRate &param = static_cast<const AudBitRate &>(recParam);
        if (param.bitRate <= 0) {
            MEDIA_LOGE("Audio encode bitrate is invalid: %{public}d", param.bitRate);
            return MSERR_INVALID_VAL;
        }
        g_object_set(gstElem_, "bitrate", param.bitRate, nullptr);
        MEDIA_LOGI("Set audio bitrate: %{public}d", param.bitRate);
        MarkParameter(param.type);
    }

    return MSERR_OK;
}

int32_t AudioEncoder::CheckConfigReady()
{
    std::set<int32_t> expectedParam = { RecorderPublicParamType::AUD_ENC_FMT };
    bool configed = CheckAllParamsConfiged(expectedParam);
    CHECK_AND_RETURN_RET(configed == true, MSERR_INVALID_OPERATION);

    return MSERR_OK;
}

RecorderMsgProcResult AudioEncoder::DoProcessMessage(GstMessage &rawMsg, RecorderMessage &prettyMsg)
{
    if (GST_MESSAGE_TYPE(&rawMsg) != GST_MESSAGE_WARNING) {
        return RecorderMsgProcResult::REC_MSG_PROC_IGNORE;
    }

    RecorderMsgProcResult ret = RecorderMsgProcResult::REC_MSG_PROC_IGNORE;
    prettyMsg.type = REC_MSG_ERROR;
    prettyMsg.code = IRecorderEngineObs::ErrorType::ERROR_INTERNAL;
    prettyMsg.detail = MSERR_AUD_ENC_FAILED;

    GstWarningMsgParser parser(rawMsg);
    CHECK_AND_RETURN_RET(parser.InitCheck(), RecorderMsgProcResult::REC_MSG_PROC_FAILED);

    MEDIA_LOGE("[WARNING] %{public}s, debug: %{public}s", parser.GetErr()->message, parser.GetDbg());
    if (parser.GetErr()->domain == GST_CORE_ERROR) {
        if (parser.GetErr()->code == GST_CORE_ERROR_NEGOTIATION) {
            MEDIA_LOGE("negotiation error");
            ret = RecorderMsgProcResult::REC_MSG_PROC_OK;
        }
    }
    if (parser.GetErr()->domain == GST_STREAM_ERROR) {
        if (parser.GetErr()->code == GST_STREAM_ERROR_ENCODE)  {
            MEDIA_LOGE("encode error");
            ret = RecorderMsgProcResult::REC_MSG_PROC_OK;
        }
    }

    return ret;
}

void AudioEncoder::Dump()
{
    MEDIA_LOGI("Audio [sourceId = 0x%{public}x]: encode format = %{public}d", desc_.handle_, encoderFormat_);
}

REGISTER_RECORDER_ELEMENT(AudioEncoder);
} // namespace Media
} // namespace OHOS
