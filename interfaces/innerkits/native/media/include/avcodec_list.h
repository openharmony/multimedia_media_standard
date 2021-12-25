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

#ifndef AVCODEC_LIST_H
#define AVCODEC_LIST_H

#include <cstdint>
#include <memory>
#include "avcodec_info.h"

namespace OHOS {
namespace Media {
/**
 * @brief Error type of AVCodecList
 *
 * @since 3.1
 * @version 3.1
 */
enum AVCodecListErrorType : int32_t {
    AVCODECLIST_ERROR_INTERNAL,
    AVCODECLIST_ERROR_EXTEND_START = 0X10000,
};

class AVCodecList {
public:
    virtual ~AVCodecList() = default;

    /**
     * @brief Find the supported video decoder name by format(usually contains video decoder MIME).
     * @param format Indicates a media description which contains required video decoder capability.
     * @return  Returns video decoder name, if not find, return empty string.
     * @since 1.0
     * @version 3.1
     */
    virtual std::string FindVideoDecoder(const Format &format) = 0;

    /**
     * @brief Find the supported video encoder name by format(usually contains video encoder MIME).
     * @param format Indicates a media description which contains required video encoder capability.
     * @return  Returns video encoder name, if not find, return empty string.
     * @since 1.0
     * @version 3.1
     */
    virtual std::string FindVideoEncoder(const Format &format) = 0;

    /**
     * @brief Find the supported audio decoder name by format(usually contains audio decoder MIME).
     * @param format Indicates a media description which contains required audio decoder capability.
     * @return  Returns audio decoder name, if not find, return empty string.
     * @since 1.0
     * @version 3.1
     */
    virtual std::string FindAudioDecoder(const Format &format) = 0;

    /**
     * @brief Find the supported audio encoder name by format(usually contains audio encoder MIME).
     * @param format Indicates a media description which contains required audio encoder capability.
     * @return  Returns audio encoder name, if not find, return empty string.
     * @since 1.0
     * @version 3.1
     */
    virtual std::string FindAudioEncoder(const Format &format) = 0;

    /**
     * @brief Get the supported video decoder capabilities.
     * @return Returns an array of supported video decoder capability.
     * @since 1.0
     * @version 3.1
     */
    virtual std::vector<std::shared_ptr<VideoCaps>> GetVideoDecoderCaps() = 0;

    /**
     * @brief Get the supported video encoder capabilities.
     * @return Returns an array of supported video encoder capability.
     * @since 1.0
     * @version 3.1
     */
    virtual std::vector<std::shared_ptr<VideoCaps>> GetVideoEncoderCaps() = 0;

    /**
     * @brief Get the supported audio decoder capabilities.
     * @return Returns an array of supported audio decoder capability.
     * @since 1.0
     * @version 3.1
     */
    virtual std::vector<std::shared_ptr<AudioCaps>> GetAudioDecoderCaps() = 0;

    /**
     * @brief Get the supported audio encoder capabilities.
     * @return Returns an array of supported audio encoder capability.
     * @since 1.0
     * @version 3.1
     */
    virtual std::vector<std::shared_ptr<AudioCaps>> GetAudioEncoderCaps() = 0;
};

class __attribute__((visibility("default"))) AVCodecListFactory {
public:

    static std::shared_ptr<AVCodecList> CreateAVCodecList();
private:
    AVCodecListFactory() = default;
    ~AVCodecListFactory() = default;
};
} // Media
} // OHOS
#endif // AVCODEC_LIST_H
