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

#include "video_encoder.h"
#include <gst/gst.h>
#include "media_errors.h"
#include "media_log.h"
#include "recorder_private_param.h"
#include "i_recorder_engine.h"
#include "avcodeclist_engine_gst_impl.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoEncoder"};
    constexpr uint32_t DEFAULT_I_FRAME_INTERVAL = 25;
}

namespace OHOS {
namespace Media {
int32_t VideoEncoder::Init()
{
    MEDIA_LOGI("VideoEncoder Init");
    return MSERR_OK;
}

std::string VideoEncoder::GetEncorderName(std::string_view mimeType)
{
    auto codecList = std::make_unique<AVCodecListEngineGstImpl>();
    CHECK_AND_RETURN_RET(codecList != nullptr, "");

    Format format;
    format.PutStringValue("codec_mime", mimeType);

    std::string pluginName = codecList->FindVideoEncoder(format);
    MEDIA_LOGI("Found plugin nmae: %{public}s", pluginName.c_str());
    return pluginName;
}

int32_t VideoEncoder::CreateMpegElement()
{
    if (gstElem_ != nullptr) {
        gst_object_unref(gstElem_);
        gstElem_ = nullptr;
    }

    std::string encorderName = GetEncorderName(CodecMimeType::VIDEO_MPEG4);
    gstElem_ = gst_element_factory_make(encorderName.c_str(), name_.c_str());
    if (gstElem_ == nullptr) {
        MEDIA_LOGE("Create mpeg encoder gst_element failed! sourceId: %{public}d", desc_.handle_);
        return MSERR_INVALID_OPERATION;
    }

    MEDIA_LOGI("use %{public}s", encorderName.c_str());
    return MSERR_OK;
}

int32_t VideoEncoder::CreateH264Element()
{
    if (gstElem_ != nullptr) {
        gst_object_unref(gstElem_);
        gstElem_ = nullptr;
    }

    std::string encorderName = GetEncorderName(CodecMimeType::VIDEO_AVC);
    gstElem_ = gst_element_factory_make(encorderName.c_str(), name_.c_str());
    if (gstElem_ == nullptr) {
        MEDIA_LOGE("Create h264 encoder gst_element failed! sourceId: %{public}d", desc_.handle_);
        return MSERR_INVALID_OPERATION;
    }
    g_object_set(gstElem_, "i-frame-interval", DEFAULT_I_FRAME_INTERVAL, nullptr);

    MEDIA_LOGI("use %{public}s", encorderName.c_str());
    return MSERR_OK;
}

int32_t VideoEncoder::Configure(const RecorderParam &recParam)
{
    if (recParam.type == RecorderPublicParamType::VID_ENC_FMT) {
        const VidEnc &param = static_cast<const VidEnc &>(recParam);
        encoderFormat_ = param.encFmt;
        switch (encoderFormat_) {
            case VideoCodecFormat::VIDEO_DEFAULT:
            case VideoCodecFormat::MPEG4: {
                int32_t ret = CreateMpegElement();
                CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "create Mpeg4 encoder failed");
                break;
            }
            case VideoCodecFormat::H264: {
                int32_t ret = CreateH264Element();
                CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "create H264 encoder failed");
                break;
            }
            default:
                MEDIA_LOGE("Currently unsupported video encode format: %{public}d", encoderFormat_);
                return MSERR_INVALID_VAL;
        }
        MEDIA_LOGI("Set video encode format: %{public}d", encoderFormat_);
        MarkParameter(param.type);
    }

    if (recParam.type == RecorderPublicParamType::VID_BITRATE) {
        const VidBitRate &param = static_cast<const VidBitRate &>(recParam);
        if (param.bitRate <= 0) {
            MEDIA_LOGE("video encode bitrate is invalid: %{public}d", param.bitRate);
            return MSERR_INVALID_VAL;
        }
        bitRate_ = param.bitRate;
        g_object_set(gstElem_, "bitrate", param.bitRate, nullptr);
        MEDIA_LOGI("Set video bitrate: %{public}d", param.bitRate);
        MarkParameter(param.type);
    }

    return MSERR_OK;
}

int32_t VideoEncoder::CheckConfigReady()
{
    std::set<int32_t> expectedParam = { RecorderPublicParamType::VID_ENC_FMT };
    bool configed = CheckAllParamsConfiged(expectedParam);
    CHECK_AND_RETURN_RET(configed == true, MSERR_INVALID_OPERATION);

    return MSERR_OK;
}

RecorderMsgProcResult VideoEncoder::DoProcessMessage(GstMessage &rawMsg, RecorderMessage &prettyMsg)
{
    if (GST_MESSAGE_TYPE(&rawMsg) != GST_MESSAGE_WARNING) {
        return RecorderMsgProcResult::REC_MSG_PROC_IGNORE;
    }

    RecorderMsgProcResult ret = RecorderMsgProcResult::REC_MSG_PROC_IGNORE;
    prettyMsg.type = REC_MSG_ERROR;
    prettyMsg.code = IRecorderEngineObs::ErrorType::ERROR_INTERNAL;
    prettyMsg.detail = MSERR_VID_ENC_FAILED;

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

void VideoEncoder::Dump()
{
    MEDIA_LOGI("video [sourceId = 0x%{public}x]: encode format = %{public}d bitrate = %{public}d",
        desc_.handle_, encoderFormat_, bitRate_);
}

REGISTER_RECORDER_ELEMENT(VideoEncoder);
} // namespace Media
} // namespace OHOS