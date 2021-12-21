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

#include "processor_vdec_impl.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "ProcessorVdecImpl"};
}

namespace OHOS {
namespace Media {
ProcessorVdecImpl::ProcessorVdecImpl()
{
}

ProcessorVdecImpl::~ProcessorVdecImpl()
{
}

int32_t ProcessorVdecImpl::ProcessMandatory(const Format &format)
{
    CHECK_AND_RETURN_RET(format.GetIntValue("width", width_) == true, MSERR_INVALID_VAL);
    CHECK_AND_RETURN_RET(format.GetIntValue("height", height_) == true, MSERR_INVALID_VAL);
    int32_t pixel = 0;
    CHECK_AND_RETURN_RET(format.GetIntValue("pixel_format", pixel) == true, MSERR_INVALID_VAL);
    CHECK_AND_RETURN_RET(format.GetIntValue("frame_rate", frameRate_) == true, MSERR_INVALID_VAL);

    MEDIA_LOGD("width:%{public}d, height:%{public}d, pixel:%{public}d, frameRate:%{public}d",
        width_, height_, pixel, frameRate_);

    VideoPixelFormat pixelFormat = VIDEO_PIXEL_FORMAT_YUVI420;
    CHECK_AND_RETURN_RET(MapVideoPixelFormat(pixel, pixelFormat) == MSERR_OK, MSERR_INVALID_VAL);
    pixelFormat_ = PixelFormatToString(pixelFormat);

    return MSERR_OK;
}

int32_t ProcessorVdecImpl::ProcessOptional(const Format &format)
{
    return MSERR_OK;
}

std::shared_ptr<ProcessorConfig> ProcessorVdecImpl::GetInputPortConfig()
{
    GstCaps *caps = nullptr;
    switch (codecName_) {
        case CODEC_MIMIE_TYPE_VIDEO_MPEG2:
            caps = gst_caps_new_simple("video/mpeg",
                "width", G_TYPE_INT, width_,
                "height", G_TYPE_INT, height_,
                "mpegversion", G_TYPE_INT, 2,
                "systemstream", G_TYPE_BOOLEAN, FALSE, nullptr);
            break;
        case CODEC_MIMIE_TYPE_VIDEO_MPEG4:
            caps = gst_caps_new_simple("video/mpeg",
                "width", G_TYPE_INT, width_,
                "height", G_TYPE_INT, height_,
                "mpegversion", G_TYPE_INT, 4,
                "systemstream", G_TYPE_BOOLEAN, FALSE, nullptr);
            break;
        case CODEC_MIMIE_TYPE_VIDEO_H263:
            caps = gst_caps_new_simple("video/x-h263",
                "width", G_TYPE_INT, width_,
                "height", G_TYPE_INT, height_,
                "variant", G_TYPE_STRING, "itu", nullptr);
            break;
        case CODEC_MIMIE_TYPE_VIDEO_AVC:
            caps = gst_caps_new_simple("video/x-h264",
                "width", G_TYPE_INT, width_,
                "height", G_TYPE_INT, height_,
                "alignment", G_TYPE_STRING, "nal",
                "stream-format", G_TYPE_STRING, "byte-stream",
                "systemstream", G_TYPE_BOOLEAN, FALSE, nullptr);
            break;
        case CODEC_MIMIE_TYPE_VIDEO_HEVC:
            caps = gst_caps_new_simple("video/x-h265",
                "width", G_TYPE_INT, width_,
                "height", G_TYPE_INT, height_,
                "alignment", G_TYPE_STRING, "nal",
                "stream-format", G_TYPE_STRING, "byte-stream",
                "systemstream", G_TYPE_BOOLEAN, FALSE, nullptr);
            break;
        default:
            break;
    }
    CHECK_AND_RETURN_RET_LOG(caps != nullptr, nullptr, "Unsupported format");

    auto config = std::make_shared<ProcessorConfig>(caps);
    if (config == nullptr) {
        MEDIA_LOGE("No memory");
        gst_caps_unref(caps);
        return nullptr;
    }

    if (codecName_ == CODEC_MIMIE_TYPE_VIDEO_AVC) {
        config->needCodecData_ = true;
    }
    return config;
}

std::shared_ptr<ProcessorConfig> ProcessorVdecImpl::GetOutputPortConfig()
{
    GstCaps *caps = gst_caps_new_simple("video/x-raw",
        "width", G_TYPE_INT, width_,
        "height", G_TYPE_INT, height_,
        "format", G_TYPE_STRING, pixelFormat_.c_str(), nullptr);
    CHECK_AND_RETURN_RET_LOG(caps != nullptr, nullptr, "No memory");

    auto config = std::make_shared<ProcessorConfig>(caps);
    if (config == nullptr) {
        MEDIA_LOGE("No memory");
        gst_caps_unref(caps);
        return nullptr;
    }
    return config;
}
} // Media
} // OHOS
