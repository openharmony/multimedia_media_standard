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
#include "media_errors.h"
#include "media_log.h"
#include "recorder_private_param.h"
#include "common_utils.h"

namespace {
using namespace OHOS::Media;
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoSource"};
constexpr uint32_t DEFAULT_FRAME_RATE = 25;
static const std::unordered_map<int32_t, int32_t> SOURCE_TYPE_STREAM_TYPE = {
    { VideoSourceType::VIDEO_SOURCE_SURFACE_ES, VideoStreamType::VIDEO_STREAM_TYPE_ES_AVC },
    { VideoSourceType::VIDEO_SOURCE_SURFACE_YUV, VideoStreamType::VIDEO_STREAM_TYPE_YUV_420 },
};
}

namespace OHOS {
namespace Media {
int32_t VideoSource::Init()
{
    auto iter = SOURCE_TYPE_STREAM_TYPE.find(desc_.type_);
    if (iter == SOURCE_TYPE_STREAM_TYPE.end()) {
        MEDIA_LOGE("unsupported video source type: %{public}d", desc_.type_);
        return MSERR_INVALID_VAL;
    }

    gstElem_ = gst_element_factory_make("videocapturesrc", name_.c_str());
    if (gstElem_ == nullptr) {
        MEDIA_LOGE("Create videosource gst element failed! sourceId: %{public}d", desc_.handle_);
        return MSERR_INVALID_OPERATION;
    }

    streamType_ = iter->second;
    g_object_set(gstElem_, "stream-type", iter->second, nullptr);
    return MSERR_OK;
}

int32_t VideoSource::Configure(const RecorderParam &recParam)
{
    int32_t ret = ConfigureVideoRectangle(recParam);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    ret = ConfigureVideoFrameRate(recParam);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    ret = ConfigureCaptureRate(recParam);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    return MSERR_OK;
}

void VideoSource::SetCaps(int32_t width, int32_t height)
{
    GstCaps *caps = nullptr;
    if (streamType_ == VideoStreamType::VIDEO_STREAM_TYPE_ES_AVC) {
        caps = gst_caps_new_simple("video/x-h264",
            "width", G_TYPE_INT, width,
            "height", G_TYPE_INT, height,
            "framerate", GST_TYPE_FRACTION, DEFAULT_FRAME_RATE, 1,
            "alignment", G_TYPE_STRING, "nal",
            "stream-format", G_TYPE_STRING, "byte-stream",
            nullptr);
    } else {
        caps = gst_caps_new_simple("video/x-raw",
            "format", G_TYPE_STRING, "NV21",
            "width", G_TYPE_INT, width,
            "height", G_TYPE_INT, height,
            "framerate", GST_TYPE_FRACTION, DEFAULT_FRAME_RATE, 1,
            nullptr);
    }

    g_object_set(gstElem_, "caps", caps, nullptr);
    gst_caps_unref(caps);
}

int32_t VideoSource::ConfigureVideoRectangle(const RecorderParam &recParam)
{
    if (recParam.type != RecorderPublicParamType::VID_RECTANGLE) {
        return MSERR_OK;
    }

    const VidRectangle &param = static_cast<const VidRectangle &>(recParam);
    if (param.width <= 0 || param.height <= 0) {
        MEDIA_LOGE("invalid width or height: %{public}d * %{public}d", param.width, param.height);
        return MSERR_INVALID_VAL;
    }

    MEDIA_LOGI("configure video source width height: %{public}d * %{public}d", param.width, param.height);
    g_object_set(gstElem_, "surface-width", static_cast<uint32_t>(param.width),
                 "surface-height", static_cast<uint32_t>(param.height), nullptr);

    SetCaps(param.width, param.height);

    MarkParameter(param.type);
    width_ = param.width;
    height_ = param.height;

    return MSERR_OK;
}

int32_t VideoSource::ConfigureVideoFrameRate(const RecorderParam &recParam)
{
    if (recParam.type != RecorderPublicParamType::VID_FRAMERATE) {
        return MSERR_OK;
    }

    const VidFrameRate &param = static_cast<const VidFrameRate &>(recParam);
    if (param.frameRate <= 0) {
        MEDIA_LOGE("Invalid video frameRate: %{public}d", param.frameRate);
        return MSERR_INVALID_VAL;
    }
    MarkParameter(RecorderPublicParamType::VID_FRAMERATE);
    g_object_set(gstElem_, "frame-rate", param.frameRate, nullptr);
    frameRate_ = param.frameRate;
    return MSERR_OK;
}

int32_t VideoSource::ConfigureCaptureRate(const RecorderParam &recParam)
{
    if (recParam.type != RecorderPublicParamType::VID_CAPTURERATE) {
        return MSERR_OK;
    }

    const CaptureRate &param = static_cast<const CaptureRate &>(recParam);
    if (param.capRate < 0.0) {
        MEDIA_LOGE("Invalid video capture rate: %{public}lf", param.capRate);
        return MSERR_INVALID_VAL;
    }
    MEDIA_LOGD("Set capturerate success: %{public}lf", param.capRate);

    MarkParameter(recParam.type);
    capRate_ = param.capRate;
    return MSERR_OK;
}

int32_t VideoSource::CheckConfigReady()
{
    std::set<int32_t> expectedParam = { RecorderPublicParamType::VID_RECTANGLE };

    if (!CheckAllParamsConfigured(expectedParam)) {
        MEDIA_LOGE("videosource required parameter not configured completely, failed !");
        return MSERR_INVALID_OPERATION;
    }

    return MSERR_OK;
}

int32_t VideoSource::GetParameter(RecorderParam &recParam)
{
    GValue val = G_VALUE_INIT;
    G_VALUE_TYPE(&val) = G_TYPE_POINTER;
    if (recParam.type == RecorderPrivateParamType::SURFACE) {
        if (g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "surface") == nullptr) {
            MEDIA_LOGE("gstelem has no surface property!");
            return MSERR_INVALID_OPERATION;
        }
        g_object_get_property((GObject *)gstElem_, "surface", &val);
        gpointer surface = g_value_get_pointer(&val);
        if (surface == nullptr) {
            MEDIA_LOGE("surface is nullptr, failed !");
            return MSERR_INVALID_VAL;
        }
        SurfaceParam &param = (SurfaceParam &)recParam;
        param.surface_ = (Surface *)surface;
    }
    return MSERR_OK;
}

void VideoSource::Dump()
{
    MEDIA_LOGI("Video [sourceId = 0x%{public}x]: width = %{public}d, height = %{public}d "
        "frameRate = %{public}d", desc_.handle_, width_, height_, frameRate_);
}

REGISTER_RECORDER_ELEMENT(VideoSource);
} // namespace Media
} // namespace OHOS
