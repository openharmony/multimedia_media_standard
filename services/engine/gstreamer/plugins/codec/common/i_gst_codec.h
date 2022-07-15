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

#ifndef I_GST_CODEC_H
#define I_GST_CODEC_H

#include <memory>
#include <gst/gst.h>
#include <vector>
#include "nocopyable.h"
#include "i_codec_params_mgr.h"
#include "i_codec_buffer_mgr.h"
#include "i_codec_common.h"

namespace OHOS {
namespace Media {
class IGstCodec {
public:
    virtual ~IGstCodec() = default;
    /**
     * @brief Init codec.
     *
     * This function must be called immediately after creation
     *
     * @return Returns GST_CODEC_OK if successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Init() = 0;

    /**
     * @brief Set a ICodecBufferMgr for the codec interface.
     *
     * Every codec need a special Inbuffermgr which must set.
     *
     * @since 1.0
     * @version 1.0
     */
    virtual void SetInBufferMgr(std::shared_ptr<ICodecBufferMgr> bufferMgr)
    {
        (void)bufferMgr;
        return;
    };

    /**
     * @brief Set a ICodecBufferMgr for the codec interface.
     *
     * Every codec need a special Outbuffermgr which must set.
     *
     * @since 1.0
     * @version 1.0
     */
    virtual void SetOutBufferMgr(std::shared_ptr<ICodecBufferMgr> bufferMgr)
    {
        (void)bufferMgr;
        return;
    };

    /**
     * @brief Set a ICodecParamsMgr for the codec interface.
     *
     * Every codec need a special ICodecParamsMgr which must set.
     *
     * @since 1.0
     * @version 1.0
     */
    virtual void SetParamsMgr(std::shared_ptr<ICodecParamsMgr> paramsMgr)
    {
        (void)paramsMgr;
        return;
    };

    /**
     * @brief Set param with key, and value is in element.
     *
     * Every codec need to set param before start.
     *
     * @return Returns GST_CODEC_OK successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetParameter(GstCodecParamKey key, GstElement *element) = 0;

    /**
     * @brief Update the value in element with key.
     *
     * Get parameter before set.
     *
     * @return Returns GST_CODEC_OK successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t GetParameter(GstCodecParamKey key, GstElement *element) = 0;

    /**
     * @brief Start codec, must have allocated or used buffers.
     *
     * @return Returns GST_CODEC_OK successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Start() = 0;

    /**
     * @brief Stop codec and then  must have free buffers.
     *
     * @return Returns GST_CODEC_OK successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Stop() = 0;

    /**
     * @brief Allocate input Buffers.
     *
     * @return Returns GST_CODEC_OK successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t AllocateInputBuffers() = 0;

    /**
     * @brief Use input Buffer which can be null, and the real buffer in the push.
     *
     * @return Returns GST_CODEC_OK successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t UseInputBuffers(std::vector<GstBuffer*> buffers) = 0;

    /**
     * @brief Send the input buffer filled with data to the codec.
     *
     * Codec will empty this buffer.
     *
     * @return Returns GST_CODEC_OK successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t PushInputBuffer(GstBuffer *buffer) = 0;

    /**
     * @brief Get the buffer which memory is empty.
     *
     * Get buffer which is empty by codec.
     *
     * @return Returns GST_CODEC_OK successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t PullInputBuffer(GstBuffer **buffer) = 0;

    /**
     * @brief Free Buffers free after stop.
     *
     * @return Returns GST_CODEC_OK successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t FreeInputBuffers() = 0;

    /**
     * @brief Allocate out Buffers.
     *
     * @return Returns GST_CODEC_OK successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t AllocateOutputBuffers() = 0;

    /**
     * @brief Use out Buffers allocate by surface or avshmem.
     *
     * @return Returns GST_CODEC_OK successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t UseOutputBuffers(std::vector<GstBuffer*> buffers) = 0;

    /**
     * @brief Send the out buffer which is empty to the codec, Codec will fill this buffer.
     *
     * Push ouput buffer which is empty.
     *
     * @return Returns GST_CODEC_OK successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t PushOutputBuffer(GstBuffer *buffer) = 0;

    /**
     * @brief Get the buffer which memory is full.
     *
     * Get ouput buffer which filled by codec.
     *
     * @return Returns GST_CODEC_OK successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t PullOutputBuffer(GstBuffer **buffer) = 0;

    /**
     * @brief Free Buffers free after stop.
     *
     * @return Returns GST_CODEC_OK successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t FreeOutputBuffers() = 0;

    /**
     * @brief Flush buffers.
     *
     * When we seek we need flush.
     *
     * @return Returns GST_CODEC_OK successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Flush(GstCodecDirect direct) = 0;

    /**
     * @brief Active or deactive port.
     *
     * When reconfig we need to deactive and then active.
     *
     * @return Returns GST_CODEC_OK successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t ActiveBufferMgr(GstCodecDirect direct, bool active) = 0;

    /**
     * @brief Init codec.
     *
     * This function must be called immediately before destroy
     *
     * @since 1.0
     * @version 1.0
     */
    virtual void Deinit() = 0;
};
} // namespace Media
} // namespace OHOS
#endif // I_GST_CODEC_H
