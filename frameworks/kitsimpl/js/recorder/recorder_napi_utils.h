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

#ifndef RECORDER_NAPI_UTILS_H
#define RECORDER_NAPI_UTILS_H

#include "av_common.h"
#include "avcodec_info.h"
#include "avcontainer_types.h"
#include "recorder.h"

namespace OHOS {
namespace Media {
int32_t MapMimeToAudioCodecFormat(const std::string &mime, AudioCodecFormat &audioCodecFormat);
int32_t MapMimeToVideoCodecFormat(const std::string &mime, VideoCodecFormat &audioCodecFormat);
int32_t MapExtensionNameToOutputFormat(const std::string &extension, OutputFormatType &type);
}
}

#endif
