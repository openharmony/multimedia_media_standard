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

#include "video_source.h"
#include <unordered_map>
#include <gst/gst.h>
#include "errors.h"
#include "media_log.h"
#include "recorder_private_param.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoSource"};
}

namespace OHOS {
namespace Media {
enum VideoStreamType : int32_t {
    VIDEO_SRC_STREAM_ES_AVC,
    VIDEO_SRC_STREAM_YUV_420,
};

static const std::unordered_map<int32_t, int32_t> SOURCE_TYPE_STREAM_TYPE = {
    { VideoSourceType::VIDEO_SOURCE_SURFACE_ES, VideoStreamType::VIDEO_SRC_STREAM_ES_AVC },
    { VideoSourceType::VIDEO_SOURCE_SURFACE_YUV, VideoStreamType::VIDEO_SRC_STREAM_YUV_420 },
};

int32_t VideoSource::Init()
{
    auto iter = SOURCE_TYPE_STREAM_TYPE.find(desc_.type_);
    if (iter == SOURCE_TYPE_STREAM_TYPE.end()) {
        MEDIA_LOGE("unsupported video source type: %{public}d", desc_.type_);
        return ERR_INVALID_VALUE;
    }

    gstElem_ = gst_element_factory_make("surfacevideosrc", name_.c_str());
    if (gstElem_ == nullptr) {
        MEDIA_LOGE("Create videosource gst element failed! sourceId: %{public}d", desc_.handle_);
        return ERR_INVALID_OPERATION;
    }

    g_object_set(gstElem_, "stream-type", iter->second, nullptr);
    return ERR_OK;
}

int32_t VideoSource::Configure(const RecorderParam &recParam)
{
    int32_t ret = ConfigureVideoRectangle(recParam);
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    ret = ConfigureVideoEncFmt(recParam);
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    ret = ConfigureVideoBitRate(recParam);
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    ret = ConfigureVideoFrameRate(recParam);
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    ret = ConfigureCaptureRate(recParam);
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    return ERR_OK;
}

int32_t VideoSource::ConfigureVideoRectangle(const RecorderParam &recParam)
{
    if (recParam.type != RecorderPublicParamType::VID_RECTANGLE) {
        return ERR_OK;
    }

    const VidRectangle &param = static_cast<const VidRectangle &>(recParam);
    if (param.width <= 0 || param.height <= 0) {
        MEDIA_LOGE("invalid width or height: %{public}d * %{public}d", param.width, param.height);
        return ERR_INVALID_VALUE;
    }

    MEDIA_LOGI("configure video source width height: %{public}d * %{public}d", param.width, param.height);
    g_object_set(gstElem_, "surface-width", static_cast<uint32_t>(param.width),
                 "surface-height", static_cast<uint32_t>(param.height), nullptr);
    MarkParameter(param.type);
    width_ = param.width;
    height_ = param.height;

    return ERR_OK;
}

int32_t VideoSource::ConfigureVideoEncFmt(const RecorderParam &recParam)
{
    if (recParam.type != RecorderPublicParamType::VID_ENC_FMT) {
        return ERR_OK;
    }

    const VidEnc &param = static_cast<const VidEnc &>(recParam);
    if (param.encFmt < VideoCodecFormat::VIDEO_DEFAULT || param.encFmt >= VideoCodecFormat::VIDEO_CODEC_FORMAT_BUTT) {
        MEDIA_LOGE("Invalid video codec format: %{public}d", param.encFmt);
        return ERR_INVALID_VALUE;
    }

    int32_t encFmt = param.encFmt;
    if (encFmt == VideoCodecFormat::VIDEO_DEFAULT) {
        encFmt = VideoCodecFormat::H264;
    }

    if (encFmt != VideoCodecFormat::H264) {
        MEDIA_LOGE("Currently unsupported video codec format: %{public}d", encFmt);
        return ERR_INVALID_VALUE;
    }

    MarkParameter(RecorderPublicParamType::VID_ENC_FMT);
    encFmt_ = encFmt;
    return ERR_OK;
}

int32_t VideoSource::ConfigureVideoBitRate(const RecorderParam &recParam)
{
    if (recParam.type != RecorderPublicParamType::VID_BITRATE) {
        return ERR_OK;
    }

    const VidBitRate &param = static_cast<const VidBitRate &>(recParam);
    if (param.bitRate <= 0) {
        MEDIA_LOGE("Invalid video bitrate: %{public}d", param.bitRate);
        return ERR_INVALID_VALUE;
    }
    MarkParameter(RecorderPublicParamType::VID_BITRATE);
    bitRate_ = param.bitRate;
    return ERR_OK;
}

int32_t VideoSource::ConfigureVideoFrameRate(const RecorderParam &recParam)
{
    if (recParam.type != RecorderPublicParamType::VID_FRAMERATE) {
        return ERR_OK;
    }

    const VidFrameRate &param = static_cast<const VidFrameRate &>(recParam);
    if (param.frameRate <= 0) {
        MEDIA_LOGE("Invalid video frameRate: %{public}d", param.frameRate);
        return ERR_INVALID_VALUE;
    }
    MarkParameter(RecorderPublicParamType::VID_FRAMERATE);
    frameRate_ = param.frameRate;
    return ERR_OK;
}

int32_t VideoSource::ConfigureCaptureRate(const RecorderParam &recParam)
{
    if (recParam.type != RecorderPublicParamType::VID_CAPTURERATE) {
        return ERR_OK;
    }

    const CaptureRate &param = static_cast<const CaptureRate &>(recParam);
    if (param.capRate <= 0.0) {
        MEDIA_LOGE("Invalid video capture rate: %{public}lf", param.capRate);
        return ERR_INVALID_VALUE;
    }
    MEDIA_LOGD("Set capturerate success: %{public}lf", param.capRate);

    MarkParameter(recParam.type);
    capRate_ = param.capRate;
    return ERR_OK;
}

int32_t VideoSource::CheckConfigReady()
{
    std::set<int32_t> expectedParam = { RecorderPublicParamType::VID_ENC_FMT, RecorderPublicParamType::VID_RECTANGLE};

    if (!CheckAllParamsConfiged(expectedParam)) {
        MEDIA_LOGE("videosource required parameter not configured completely, failed !");
        return ERR_INVALID_OPERATION;
    }

    return ERR_OK;
}

int32_t VideoSource::GetParameter(RecorderParam &recParam)
{
    GValue val = G_VALUE_INIT;
    G_VALUE_TYPE(&val) = G_TYPE_POINTER;
    if (recParam.type == RecorderPrivateParamType::SURFACE) {
        if (g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "surface") == nullptr) {
            MEDIA_LOGE("gstelem has no surface property!");
            return ERR_INVALID_OPERATION;
        }
        g_object_get_property((GObject *)gstElem_, "surface", &val);
        gpointer surface = g_value_get_pointer(&val);
        if (surface == nullptr) {
            MEDIA_LOGE("surface is nullptr, failed !");
            return ERR_DEAD_OBJECT;
        }
        SurfaceParam &param = (SurfaceParam &)recParam;
        param.surface_ = (Surface *)surface;
    }
    return ERR_OK;
}

void VideoSource::Dump()
{
    MEDIA_LOGI("Video [sourceId = 0x%{public}x]: width = %{public}d, height = %{public}d, bitRate = %{public}d, "
               "encode format = %{public}d, frameRate = %{public}d, captureRate = %{public}d",
               desc_.handle_, width_, height_, bitRate_, encFmt_, frameRate_, capRate_);
}

REGISTER_RECORDER_ELEMENT(VideoSource);
}
}
