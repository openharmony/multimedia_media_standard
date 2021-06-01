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

#ifndef VOLUME_CONTROL_H
#define VOLUME_CONTROL_H

#include "audio_renderer_sink.h"
#include "audio_svc_manager.h"
#include "audio_types.h"
#include <memory>

namespace OHOS {
namespace Media {
class VolumeControl {
public:
    VolumeControl();
    virtual ~VolumeControl() = default;
    bool Init(void);
    int32_t SetVolume(float lVolume, float rVolume);
    int32_t GetVolume(void);
    int32_t SaveVolume(float lVolume, float rVolume);
    int32_t SetCurrentVolume(void);
    void SetRenderer(AudioRendererSink *renderer);
    float GetAvgVolume(float lVolume, float rVolume);
    float VolumeToLevel(float Volume);
    int32_t VolumeInit(void);
    int32_t GetMaxVolume(void);
    int32_t GetMinVolume(void);

    AudioRendererSink *mAudioRendererSink_;
    AudioSvcManager *mAudioSvcMgr_;
private:
    int32_t mMaxVolLevel_;
    int32_t mMinVolLevel_;
    float mCurVolLevel_;
};
}  // namespace Media
}  // namespace OHOS

#endif  // VOLUME_CONTROL_H

