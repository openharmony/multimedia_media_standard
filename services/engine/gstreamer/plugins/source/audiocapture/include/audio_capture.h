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

#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include <memory>
#include "common_utils.h"

namespace OHOS {
namespace Media {
class AudioCapture {
public:
    virtual ~AudioCapture() = default;

    /**
     * @brief Sets the encoding bit rate, number of audio channels and sampling rate for recording.
     *
     * This function must be called before {@link StartAudioCapture}.
     *
     * @param bitrate Indicates the audio encoding bit rate, in bit/s.
     * @param channels Indicates the number of audio channels to set.
     * @param sampleRate Indicates the sampling rate of the audio per second.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetCaptureParameter(uint32_t bitrate, uint32_t channels, uint32_t sampleRate) = 0;

    /**
     * @brief Gets the encoding bit rate, number of audio channels and sampling rate for recording.
     *
     * @param bitrate Indicates the audio encoding bit rate, in bit/s.
     * @param channels Indicates the number of audio channels to set.
     * @param sampleRate Indicates the sampling rate of the audio per second.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t GetCaptureParameter(uint32_t &bitrate, uint32_t &channels, uint32_t &sampleRate) = 0;

    /**
     * @brief Gets the start time of audio segment.
     *
     * This function must be called after {@link StartAudioCapture}.
     *
     * @param bitrate Indicates the start time of audio segment, in nanosecond.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t GetSegmentInfo(uint64_t &start) = 0;

    /**
     * @brief Starts capturing audio.
     *
     * This function must be called after {@link SetCaptureParameter}.
     *
     * @return Returns {@link SUCCESS} if the recording is started; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t StartAudioCapture() = 0;

    /**
     * @brief Stops capturing audio.
     *
     * This function must be called after {@link StartAudioCapture}.
     *
     * @return Returns {@link SUCCESS} if the recording is started; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t StopAudioCapture() = 0;

    /**
     * @brief Pause capturing audio.
     *
     * This function must be called after {@link StartAudioCapture}.
     *
     * @return Returns {@link SUCCESS} if the recording is started; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t PauseAudioCapture() = 0;

    /**
     * @brief Resume capturing audio.
     *
     * This function must be called after {@link PauseAudioCapture}.
     *
     * @return Returns {@link SUCCESS} if the recording is started; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t ResumeAudioCapture() = 0;

    /**
     * @brief Gets the audio frame buffer.
     *
     * This function must be called after {@link StartAudioCapture} but before {@link StopAudioCapture}.
     *
     * @return Returns {@link AudioBuffer} under normal circumstances; returns nullptr otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual std::shared_ptr<AudioBuffer> GetBuffer() = 0;
};
} // namespace Media
} // namespace OHOS
#endif // AUDIO_CAPTURE_H
