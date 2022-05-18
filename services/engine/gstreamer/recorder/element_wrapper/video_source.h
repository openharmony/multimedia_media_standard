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

#ifndef VIDEO_SOURCE_H
#define VIDEO_SOURCE_H

#include "recorder_element.h"

namespace OHOS {
namespace Media {
class VideoSource : public RecorderElement {
public:
    using RecorderElement::RecorderElement;
    ~VideoSource() = default;

    int32_t Init() override;
    int32_t Configure(const RecorderParam &recParam) override;
    int32_t CheckConfigReady() override;
    int32_t GetParameter(RecorderParam &recParam) override;
    void Dump() override;

private:
    void SetCaps(int32_t width, int32_t height);
    int32_t ConfigureVideoRectangle(const RecorderParam &recParam);
    int32_t ConfigureVideoFrameRate(const RecorderParam &recParam);
    int32_t ConfigureCaptureRate(const RecorderParam &recParam);

    int32_t width_ = 0;
    int32_t height_ = 0;
    int32_t frameRate_ = 0;
    double capRate_ = 0;
    int32_t streamType_ = 0;
};
} // namespace Media
} // namespace OHOS
#endif // VIDEO_SOURCE_H
