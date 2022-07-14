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

#ifndef AUDIO_SOURCE_H
#define AUDIO_SOURCE_H

#include "recorder_element.h"

namespace OHOS {
namespace Media {
class AudioSource : public RecorderElement {
public:
    using RecorderElement::RecorderElement;
    ~AudioSource() = default;

    int32_t Init() override;
    int32_t Configure(const RecorderParam &recParam) override;
    int32_t CheckConfigReady() override;
    int32_t Prepare() override;
    int32_t Stop() override;
    void Dump() override;

private:
    int32_t ConfigAudioSampleRate(const RecorderParam &recParam);
    int32_t ConfigAudioChannels(const RecorderParam &recParam);
    int32_t ConfigAudioBitRate(const RecorderParam &recParam);
    int32_t ConfigAppInfo(const RecorderParam &recParam);

    int32_t sampleRate_;
    int32_t channels_;
    int32_t bitRate_;
};
} // namespace Media
} // namespace OHOS
#endif // AUDIO_SOURCE_H
