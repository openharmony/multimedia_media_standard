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

#ifndef AVCODEC_VIDEO_ENCODER_H
#define AVCODEC_VIDEO_ENCODER_H

#include "avcodec_common.h"
#include "avsharedmemory.h"
#include "format.h"
#include "surface.h"

namespace OHOS {
namespace Media {
class VideoEncoder {
public:
    virtual ~VideoEncoder() = default;

    /**
     * @brief Configure the encoder.
     *
     * @param format The format of the input data and the desired format of the output data.
     * @return Returns {@link MSERR_OK} if success; returns an error code otherwise.
     * @since 3.1
     * @version 3.1
     */
    virtual int32_t Configure(const Format &format) = 0;

    /**
     * @brief Prepare for decoding.
     *
     * This function must be called after {@link Configure} and before {@link Start}
     *
     * @return Returns {@link MSERR_OK} if success; returns an error code otherwise.
     * @since 3.1
     * @version 3.1
     */
    virtual int32_t Prepare() = 0;

    /**
     * @brief Start decoding.
     *
     * @return Returns {@link MSERR_OK} if success; returns an error code otherwise.
     * @since 3.1
     * @version 3.1
     */
    virtual int32_t Start() = 0;

    /**
     * @brief Stop decoding.
     *
     * This function must be called during running
     *
     * @return Returns {@link MSERR_OK} if success; returns an error code otherwise.
     * @since 3.1
     * @version 3.1
     */
    virtual int32_t Stop() = 0;

    /**
     * @brief Flush both input and output buffers of the encoder.
     *
     * @return Returns {@link MSERR_OK} if success; returns an error code otherwise.
     * @since 3.1
     * @version 3.1
     */
    virtual int32_t Flush() = 0;

    /**
     * @brief Restores the encoder to the initial state.
     *
     * @return Returns {@link MSERR_OK} if success; returns an error code otherwise.
     * @since 3.1
     * @version 3.1
     */
    virtual int32_t Reset() = 0;

    /**
     * @brief Releases encoder resources. All methods are unavailable after calling this.
     *
     * @return Returns {@link MSERR_OK} if success; returns an error code otherwise.
     * @since 3.1
     * @version 3.1
     */
    virtual int32_t Release() = 0;

    /**
     * @brief Obtains the surface from encoder.
     *
     * This function can only be called after {@link Configure} and before {@link Prepare}
     *
     * @return Returns the pointer to the surface.
     * @since 3.1
     * @version 3.1
     */
    virtual sptr<Surface> CreateInputSurface() = 0;

    /**
     * @brief Returns a {@link AVSharedMemory} object for a input buffer index that contains the data.
     *
     * This function must be called during running
     *
     * @param index The index of the input buffer.
     * @return Returns {@link AVSharedMemory} if success; returns nullptr otherwise.
     * @since 3.1
     * @version 3.1
     */
    virtual std::shared_ptr<AVSharedMemory> GetInputBuffer(uint32_t index) = 0;

    /**
     * @brief Submits input buffer to encoder.
     *
     * This function must be called during running
     *
     * @param index The index of the input buffer.
     * @param info The info of the input buffer. For details, see {@link AVCodecBufferInfo}
     * @param flag The flag of the input buffer. For details, see {@link AVCodecBufferFlag}
     * @return Returns {@link MSERR_OK} if success; returns an error code otherwise.
     * @since 3.1
     * @version 3.1
     */
    virtual int32_t QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) = 0;

    /**
     * @brief Returns a {@link AVSharedMemory} object for a output buffer index that contains the data.
     *
     * This function must be called during running
     *
     * @param index The index of the output buffer.
     * @return Returns {@link AVSharedMemory} if success; returns nullptr otherwise.
     * @since 3.1
     * @version 3.1
     */
    virtual std::shared_ptr<AVSharedMemory> GetOutputBuffer(uint32_t index) = 0;

    /**
     * @brief Gets the format of the output data.
     *
     * This function must be called after {@link Configure}
     *
     * @param format
     * @return Returns {@link MSERR_OK} if success; returns an error code otherwise.
     * @since 3.1
     * @version 3.1
     */
    virtual int32_t GetOutputFormat(Format &format) = 0;

    /**
     * @brief Returns the output buffer to the encoder.
     *
     * This function must be called during running
     *
     * @param index The index of the output buffer.
     * @return Returns {@link MSERR_OK} if success; returns an error code otherwise.
     * @since 3.1
     * @version 3.1
     */
    virtual int32_t ReleaseOutputBuffer(uint32_t index) = 0;

    /**
     * @brief Sets the parameters to the encoder.
     *
     * This function must be called after {@link Configure}
     *
     * @param format The parameters.
     * @return Returns {@link MSERR_OK} if success; returns an error code otherwise.
     * @since 3.1
     * @version 3.1
     */
    virtual int32_t SetParameter(const Format &format) = 0;

    /**
     * @brief Registers a encoder listener.
     *
     * This function must be called before {@link Configure}
     *
     * @param callback Indicates the encoder listener to register. For details, see {@link AVCodecCallback}.
     * @return Returns {@link MSERR_OK} if success; returns an error code otherwise.
     * @since 1.0
     * @version 3.1
     */
    virtual int32_t SetCallback(const std::shared_ptr<AVCodecCallback> &callback) = 0;
};

class __attribute__((visibility("default"))) VideoEncoderFactory {
public:
    /**
     * @brief Instantiate the preferred encoder of the given mime type.
     *
     * @param mime The mime type.
     * @return Returns the preferred encoder.
     * @since 3.1
     * @version 3.1
     */
    static std::shared_ptr<VideoEncoder> CreateByMime(const std::string &mime);

    /**
     * @brief Instantiates the designated encoder.
     *
     * @param name The encoder's name.
     * @return Returns the designated encoder.
     * @since 3.1
     * @version 3.1
     */
    static std::shared_ptr<VideoEncoder> CreateByName(const std::string &name);
private:
    VideoEncoderFactory() = default;
    ~VideoEncoderFactory() = default;
};
} // Media
} // OHOS
#endif // AVCODEC_VIDEO_ENCODER_H