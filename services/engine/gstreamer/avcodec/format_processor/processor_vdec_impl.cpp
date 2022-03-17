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
    constexpr uint32_t MAX_SIZE = 3150000; // 3MB
    constexpr uint32_t MAX_WIDTH = 8000;
    constexpr uint32_t MAX_HEIGHT = 5000;
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
    CHECK_AND_RETURN_RET(format.GetIntValue("pixel_format", pixelFormat_) == true, MSERR_INVALID_VAL);
    CHECK_AND_RETURN_RET(format.GetIntValue("frame_rate", frameRate_) == true, MSERR_INVALID_VAL);
    MEDIA_LOGD("width:%{public}d, height:%{public}d, pixel:%{public}d, frameRate:%{public}d",
        width_, height_, pixelFormat_, frameRate_);

    gstPixelFormat_ = PixelFormatToGst(static_cast<VideoPixelFormat>(pixelFormat_));

    return MSERR_OK;
}

int32_t ProcessorVdecImpl::ProcessOptional(const Format &format)
{
    if (format.GetValueType(std::string_view("max_input_size")) == FORMAT_TYPE_INT32) {
        (void)format.GetIntValue("max_input_size", maxInputSize_);
    }

    return MSERR_OK;
}

std::shared_ptr<ProcessorConfig> ProcessorVdecImpl::GetInputPortConfig()
{
    CHECK_AND_RETURN_RET(width_ > 0 && width_ < MAX_WIDTH, nullptr);
    CHECK_AND_RETURN_RET(height_ > 0 && height_ < MAX_HEIGHT, nullptr);

    GstCaps *caps = nullptr;
    switch (codecName_) {
        case CODEC_MIMIE_TYPE_VIDEO_MPEG2:
            caps = gst_caps_new_simple("video/mpeg",
                "width", G_TYPE_INT, width_, "height", G_TYPE_INT, height_,
                "mpegversion", G_TYPE_INT, 2, "systemstream", G_TYPE_BOOLEAN, FALSE, nullptr);
            break;
        case CODEC_MIMIE_TYPE_VIDEO_MPEG4:
            caps = gst_caps_new_simple("video/mpeg",
                "width", G_TYPE_INT, width_, "height", G_TYPE_INT, height_,
                "mpegversion", G_TYPE_INT, 4, "systemstream", G_TYPE_BOOLEAN, FALSE, nullptr);
            break;
        case CODEC_MIMIE_TYPE_VIDEO_H263:
            caps = gst_caps_new_simple("video/x-h263",
                "width", G_TYPE_INT, width_, "height", G_TYPE_INT, height_,
                "variant", G_TYPE_STRING, "itu", nullptr);
            break;
        case CODEC_MIMIE_TYPE_VIDEO_AVC:
            caps = gst_caps_new_simple("video/x-h264",
                "width", G_TYPE_INT, width_, "height", G_TYPE_INT, height_,
                "framerate", GST_TYPE_FRACTION, frameRate_, 1,
                "alignment", G_TYPE_STRING, "nal", "stream-format", G_TYPE_STRING, "byte-stream",
                "systemstream", G_TYPE_BOOLEAN, FALSE, nullptr);
            break;
        case CODEC_MIMIE_TYPE_VIDEO_HEVC:
            caps = gst_caps_new_simple("video/x-h265",
                "width", G_TYPE_INT, width_, "height", G_TYPE_INT, height_,
                "alignment", G_TYPE_STRING, "nal", "stream-format", G_TYPE_STRING, "byte-stream",
                "systemstream", G_TYPE_BOOLEAN, FALSE, nullptr);
            break;
        default:
            break;
    }
    CHECK_AND_RETURN_RET_LOG(caps != nullptr, nullptr, "Unsupported format");

    auto config = std::make_shared<ProcessorConfig>(caps, false);
    if (config == nullptr) {
        gst_caps_unref(caps);
        return nullptr;
    }

    config->needCodecData_ = (codecName_ == CODEC_MIMIE_TYPE_VIDEO_AVC && isSoftWare_);
    if (maxInputSize_ > 0) {
        config->bufferSize_ = (maxInputSize_ > MAX_SIZE) ? MAX_SIZE : static_cast<uint32_t>(maxInputSize_);
    } else {
        config->bufferSize_ = CompressedBufSize(width_, height_, false, codecName_);
    }

    return config;
}

std::shared_ptr<ProcessorConfig> ProcessorVdecImpl::GetOutputPortConfig()
{
    CHECK_AND_RETURN_RET(width_ > 0 && width_ < MAX_WIDTH, nullptr);
    CHECK_AND_RETURN_RET(height_ > 0 && height_ < MAX_HEIGHT, nullptr);

    GstCaps *caps = gst_caps_new_simple("video/x-raw",
        "format", G_TYPE_STRING, gstPixelFormat_.c_str(), nullptr);
    CHECK_AND_RETURN_RET_LOG(caps != nullptr, nullptr, "No memory");

    auto config = std::make_shared<ProcessorConfig>(caps, false);
    if (config == nullptr) {
        MEDIA_LOGE("No memory");
        gst_caps_unref(caps);
        return nullptr;
    }

    constexpr uint32_t alignment = 16;
    config->bufferSize_ = PixelBufferSize(static_cast<VideoPixelFormat>(pixelFormat_),
        static_cast<uint32_t>(width_), static_cast<uint32_t>(height_), alignment);

    return config;
}
} // namespace Media
} // namespace OHOS
