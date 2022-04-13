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

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include "audio_capture.h"
#include "audio_capturer.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
struct AudioCacheCtrl {
    std::mutex captureMutex_;
    std::condition_variable captureCond_;
    std::condition_variable pauseCond_;
    std::queue<std::shared_ptr<AudioBuffer>> captureQueue_;
    uint64_t timestamp_ = 0;
    uint64_t pausedTime_ = 0; // the timestamp when audio pause called
    uint64_t resumeTime_ = 0; // the timestamp when audio resume called
    uint32_t pausedCount_ = 0; // the paused count times
    uint64_t persistTime_ = 0;
    uint64_t totalPauseTime_ = 0;
};

class AudioCaptureAsImpl : public AudioCapture, public NoCopyable {
public:
    AudioCaptureAsImpl();
    virtual ~AudioCaptureAsImpl();

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

    // audio cache
    enum AudioRecorderState : int32_t {
        RECORDER_INITIALIZED = 0,
        RECORDER_RUNNING,
        RECORDER_PAUSED,
        RECORDER_RESUME,
        RECORDER_STOP,
    };

    void GetAudioCaptureBuffer();
    std::unique_ptr<AudioCacheCtrl> audioCacheCtrl_;
    std::unique_ptr<std::thread> captureLoop_;
    std::mutex pauseMutex_;
    std::atomic<int32_t> curState_ = RECORDER_INITIALIZED;
};
} // namespace Media
} // namespace OHOS

#endif // AUDIO_CAPTURE_AS_IMPL_H
