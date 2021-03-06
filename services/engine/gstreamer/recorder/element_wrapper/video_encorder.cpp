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

#include "video_encorder.h"
#include <gst/gst.h>
#include "media_errors.h"
#include "media_log.h"
#include "recorder_private_param.h"
#include "i_recorder_engine.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoEncoder"};
}

namespace OHOS {
namespace Media {
int32_t VideoEncorder::Init()
{
    MEDIA_LOGI("VideoEncorder Init");
    return MSERR_OK;
}

int32_t VideoEncorder::CreateElement()
{
    if (gstElem_ != nullptr) {
        gst_object_unref(gstElem_);
        gstElem_ = nullptr;
    }
    gstElem_ = gst_element_factory_make("avenc_mpeg4", name_.c_str());
    if (gstElem_ == nullptr) {
        MEDIA_LOGE("Create avenc_mpeg4 gst_element failed! sourceId: %{public}d", desc_.handle_);
        return MSERR_INVALID_OPERATION;
    }

    MEDIA_LOGI("use avenc_mpeg4");

    return MSERR_OK;
}

int32_t VideoEncorder::Configure(const RecorderParam &recParam)
{
    if (recParam.type == RecorderPublicParamType::VID_ENC_FMT) {
        const VidEnc &param = static_cast<const VidEnc &>(recParam);
        encoderFormat_ = param.encFmt;
        switch (encoderFormat_) {
            case VideoCodecFormat::VIDEO_DEFAULT:
            case VideoCodecFormat::MPEG4: {
                int32_t ret = CreateElement();
                CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "create video encorder failed");
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

int32_t VideoEncorder::CheckConfigReady()
{
    std::set<int32_t> expectedParam = { RecorderPublicParamType::VID_ENC_FMT };
    bool configed = CheckAllParamsConfiged(expectedParam);
    CHECK_AND_RETURN_RET(configed == true, MSERR_INVALID_OPERATION);

    return MSERR_OK;
}

RecorderMsgProcResult VideoEncorder::DoProcessMessage(GstMessage &rawMsg, RecorderMessage &prettyMsg)
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

void VideoEncorder::Dump()
{
    MEDIA_LOGI("video [sourceId = 0x%{public}x]: encode format = %{public}d bitrate = %{public}d",
        desc_.handle_, encoderFormat_, bitRate_);
}

REGISTER_RECORDER_ELEMENT(VideoEncorder);
}
}