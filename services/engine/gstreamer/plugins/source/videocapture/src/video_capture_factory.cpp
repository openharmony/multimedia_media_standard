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

#include "video_capture_factory.h"
#include <cstdlib>
#include <memory>
#include "video_capture_sf_es_avc_impl.h"
#include "video_capture_sf_yuv_impl.h"

namespace OHOS {
namespace Media {
std::unique_ptr<VideoCapture> VideoCaptureFactory::CreateVideoCapture(VideoStreamType streamType)
{
    if (streamType == VIDEO_STREAM_TYPE_ES_AVC) {
        return std::make_unique<VideoCaptureSfEsAvcImpl>();
    }

    if (streamType == VIDEO_STREAM_TYPE_YUV_420) {
        return std::make_unique<VideoCaptureSfYuvImpl>();
    }

    return nullptr;
}
} // namespace Media
} // namespace OHOS
