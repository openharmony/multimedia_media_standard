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

#ifndef I_CODEC_COMMON_H
#define I_CODEC_COMMON_H
#include <memory>

namespace OHOS {
namespace Media {
enum GstCodecRet : int32_t {
    GST_CODEC_ERROR = -1,
    GST_CODEC_OK,
    GST_CODEC_FORMAT_CHANGE,
    GST_CODEC_EOS,
    GST_CODEC_FLUSH,
    GST_CODEC_NO_BUFFER
};

enum GstCodecDirect : int32_t {
    GST_CODEC_INPUT = 1,
    GST_CODEC_OUTPUT = 2,
    GST_CODEC_ALL = 3,
};

enum GstCompressionFormat : int32_t {
    GST_AVC,
    GST_HEVC,
};

enum GstCodecParamKey : int32_t {
    GST_VIDEO_INPUT_COMMON,
    GST_VIDEO_OUTPUT_COMMON,
    GST_VIDEO_FORMAT,
    GST_VIDEO_SURFACE_INIT,
    GST_STATIC_BITRATE,
    GST_DYNAMIC_BITRATE,
    GST_REQUEST_I_FRAME,
    GST_VENDOR,
    GST_VIDEO_ENCODER_CONFIG,
    GST_DYNAMIC_FRAME_RATE,
};
} // namespace Media
} // namespace OHOS
#endif // I_CODEC_COMMON_H
