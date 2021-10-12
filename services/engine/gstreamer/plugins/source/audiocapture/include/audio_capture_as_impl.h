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

#ifndef AUDIO_CAPTURE_AS_IMPL_H
#define AUDIO_CAPTURE_AS_IMPL_H

#include "audio_capture.h"
#include "audio_capturer.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class AudioCaptureAsImpl : public AudioCapture {
public:
    AudioCaptureAsImpl();
    virtual ~AudioCaptureAsImpl();
    DISALLOW_COPY_AND_MOVE(AudioCaptureAsImpl);
    int32_t SetCaptureParameter(uint32_t bitrate, uint32_t channels, uint32_t sampleRate) override;
    int32_t GetCaptureParameter(uint32_t &bitrate, uint32_t &channels, uint32_t &sampleRate) override;
    int32_t GetSegmentInfo(uint64_t &start) override;
    int32_t StartAudioCapture() override;
    int32_t StopAudioCapture() override;
    int32_t PauseAudioCapture() override;
    int32_t ResumeAudioCapture() override;
    std::shared_ptr<AudioBuffer> GetBuffer() override;

private:
    std::unique_ptr<OHOS::AudioStandard::AudioCapturer> audioCapturer_ = nullptr;
    size_t bufferSize_ = 0; // minimum size of each buffer acquired from AudioServer
    uint64_t bufferDurationNs_ = 0; // each buffer
    uint64_t timestamp_ = 0;
    uint64_t pausedTime_ = 0; // the timestamp when audio pause called
    uint64_t resumeTime_ = 0; // the timestamp when audio resume called
    uint32_t pausedCount_ = 0; // the paused count times
    uint64_t persistTime_ = 0;
    uint64_t totalPauseTime_ = 0;
};
}  // namespace Media
}  // namespace OHOS

#endif // AUDIO_CAPTURE_AS_IMPL_H
