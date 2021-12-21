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

#ifndef AVCODEC_COMMOM_H
#define AVCODEC_COMMOM_H

#include <string>
#include "format.h"

namespace OHOS {
namespace Media {
/**
 * @brief Buffer type of AVCodec
 *
 * @since 3.1
 * @version 3.1
 */
enum AVCodecBufferType : int32_t {
    AVCODEC_BUFFER_TYPE_NONE = 0,
    AVCODEC_BUFFER_TYPE_CODEC,
    AVCODEC_BUFFER_TYPE_KEY_FRAME,
};

/**
 * @brief Buffer flag of AVCodec
 *
 * @since 3.1
 * @version 3.1
 */
enum AVCodecBufferFlag : int32_t {
    AVCODEC_BUFFER_FLAG_NONE = 0,
    AVCODEC_BUFFER_FLAG_END_OF_STREAM,
};

/**
 * @brief Buffer info AVCodec
 *
 * @since 3.1
 * @version 3.1
 */
struct AVCodecBufferInfo {
    int64_t presentationTimeUs = 0;
    int32_t size = 0;
    int32_t offset = 0;
};

/**
 * @brief Callback of AVCodec
 *
 * @since 3.1
 * @version 3.1
 */
class AVCodecCallback {
public:
    virtual ~AVCodecCallback() = default;

    /**
     * @brief
     *
     * @param errorType
     * @param errorCode
     * @since 3.1
     * @version 3.1
     */
    virtual void OnError(AVCodecErrorType errorType, int32_t errorCode) = 0;

    /**
     * @brief
     *
     * @param format
     * @since 3.1
     * @version 3.1
     */
    virtual void OnOutputFormatChanged(const Format &format) = 0;

    /**
     * @brief
     *
     * @param index
     * @since 3.1
     * @version 3.1
     */
    virtual void OnInputBufferAvailable(uint32_t index) = 0;

    /**
     * @brief
     *
     * @param index
     * @param info
     * @param type
     * @param flag
     * @since 3.1
     * @version 3.1
     */
    virtual void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info,
                                         AVCodecBufferType type, AVCodecBufferFlag flag) = 0;
};

__attribute__((visibility("default"))) std::string AVCodecErrorTypeToString(AVCodecErrorType type);
} // Media
} // OHOS
#endif // AVCODEC_COMMOM_H
