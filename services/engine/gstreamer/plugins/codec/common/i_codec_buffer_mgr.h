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

#ifndef I_CODEC_BUFFER_MGR_H
#define I_CODEC_BUFFER_MGR_H

#include <gst/gst.h>
#include <vector>
#include "nocopyable.h"
#include "i_codec_common.h"

namespace OHOS {
namespace Media {
class ICodecBufferMgr {
public:
    virtual ~ICodecBufferMgr() = default;

    /**
     * @brief Allocate input buffers.
     *
     * This inner memory of the kernel and should not be exposed to the outside.
     *
     * @return Returns GST_CODEC_OK successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t AllocateBuffers() = 0;

    /**
     * @brief Use input buffer which can be null, and the real buffer in the push.
     *
     * Usebuffers of ashmem and bufferhandle.
     *
     * @return Returns GST_CODEC_OK successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t UseBuffers(std::vector<GstBuffer*> buffers) = 0;

    /**
     * @brief Send the input buffer or ouput buffer.
     *
     * Codec will empty this buffer or fill this buffer.
     *
     * @return Returns GST_CODEC_OK successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t PushBuffer(GstBuffer *buffer) = 0;

    /**
     * @brief Get the buffer.
     *
     * Get the buffer which is filled or empty.
     *
     * @return Returns GST_CODEC_OK successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t PullBuffer(GstBuffer **buffer) = 0;

    /**
     * @brief Free Buffers free after stop.
     *
     * @return Returns GST_CODEC_OK successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t FreeBuffers() = 0;

    /**
     * @brief Flush buffers.
     *
     * @return Returns GST_CODEC_OK successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Flush(bool enable) = 0;
};
} // namespace Media
} // namespace OHOS
#endif // I_CODEC_BUFFER_MGR_H
