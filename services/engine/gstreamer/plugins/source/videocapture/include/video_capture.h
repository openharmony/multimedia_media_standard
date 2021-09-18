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

#ifndef VIDEO_CAPTURE_H
#define VIDEO_CAPTURE_H

#include <memory>
#include "surface.h"
#include "common_utils.h"

namespace OHOS {
namespace Media {
class VideoCapture {
public:
    virtual ~VideoCapture() = default;

    /**
     * @brief Prepares for capturing video.
     *
     * This function must be called before {@link Start}.
     *
     * @return Returns {@link SUCCESS} if the preparation is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Prepare() = 0;

    /**
     * @brief Starts capturing video.
     *
     * This function must be called after {@link Prepare}.
     *
     * @return Returns {@link SUCCESS} if the recording is started; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Start() = 0;

    /**
     * @brief Pauses capturing video.
     *
     * After {@link Start} is called, you can call this function to pause capturing video. The video source streams
     * are not paused, and source data is discarded.
     *
     * @return Returns {@link SUCCESS} if the recording is paused; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Pause() = 0;

    /**
     * @brief Resumes capturing video.
     *
     * You can call this function to resume capturing after {@link Pause} is called.
     *
     * @return Returns {@link SUCCESS} if the recording is resumed; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Resume() = 0;

    /**
     * @brief Stops capturing video.
     *
     * @return Returns {@link SUCCESS} if the recording is stopped; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Stop() = 0;

    /**
     * @brief Sets the width of the video frame to capture.
     *
     * This function must be called before {@link Prepare}.
     *
     * @param width Indicates the video width to set.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetVideoWidth(uint32_t width) = 0;

    /**
     * @brief Sets the height of the video frame to capture.
     *
     * This function must be called before {@link Prepare}.
     *
     * @param height Indicates the video height to set.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetVideoHeight(uint32_t height) = 0;

    /**
     * @brief Obtains the surface of the video source.
     *
     * @return Returns the pointer to the surface.
     * @since 1.0
     * @version 1.0
     */
    virtual sptr<Surface> GetSurface() = 0;

    /**
     * @brief Gets the codec buffer.
     *
     * This function must be called after {@link StartAudioCapture} but before {@link StopAudioCapture}.
     *
     * @return If the video source provides raw encoded data, this function will return {@link EsAvcCodecBuffer};
     * return nullptr otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual std::shared_ptr<EsAvcCodecBuffer> GetCodecBuffer() = 0;

    /**
     * @brief Gets the video frame buffer.
     *
     * This function must be called after {@link StartAudioCapture} but before {@link StopAudioCapture}.
     *
     * @return If the video source provides raw encoded data, this function will return {@link VideoFrameBuffer};
     * return nullptr otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual std::shared_ptr<VideoFrameBuffer> GetFrameBuffer() = 0;
};
}  // namespace Media
}  // namespace OHOS
#endif // VIDEO_CAPTURE_H
