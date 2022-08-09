/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "enum_native_mock.h"

namespace OHOS {
namespace Media {
std::string EnumNativeMock::GetMediaDescriptionKey(const MediaDescriptionKeyMock &key) const
{
    std::string ret;
    if (MEDIA_DESCRIPTION_KEY_INFOS.find(key) != MEDIA_DESCRIPTION_KEY_INFOS.end()) {
        ret = MEDIA_DESCRIPTION_KEY_INFOS.find(key)->second;
    }
    return ret;
}

int32_t EnumNativeMock::GetVideoPixelFormat(const VideoPixelFormatMock &key) const
{
    int32_t ret = 0;
    if (VIDEO_PIXEL_FORMAT_INFOS.find(key) != VIDEO_PIXEL_FORMAT_INFOS.end()) {
        ret = VIDEO_PIXEL_FORMAT_INFOS.find(key)->second;
    }
    return ret;
}

std::string EnumNativeMock::GetCodecMimeType(const CodecMimeTypeMock &key) const
{
    std::string ret;
    if (CODEC_MIME_INFOS.find(key) != CODEC_MIME_INFOS.end()) {
        ret = CODEC_MIME_INFOS.find(key)->second;
    }
    return ret;
}
} // Media
} // OHOS