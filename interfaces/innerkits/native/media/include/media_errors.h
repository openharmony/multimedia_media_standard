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
#ifndef MEDIA_ERRORS_H
#define MEDIA_ERRORS_H

#include "errors.h"
#include <map>
#include <string>

namespace OHOS {
namespace Media {
using MSErrCode = ErrCode;

// bit 28~21 is subsys, bit 20~16 is Module. bit 15~0 is code
constexpr MSErrCode MS_MODULE = 0X01000;
constexpr MSErrCode MS_ERR_OFFSET = ErrCodeOffset(SUBSYS_MULTIMEDIA, MS_MODULE);
enum MediaServiceErrCode : ErrCode {
    MSERR_OK                = ERR_OK,
    MSERR_NO_MEMORY         = MS_ERR_OFFSET + ENOMEM, // no memory
    MSERR_INVALID_OPERATION = MS_ERR_OFFSET + ENOSYS, // opertation not be permitted
    MSERR_INVALID_VAL       = MS_ERR_OFFSET + EINVAL, // invalid argument
    MSERR_UNKNOWN           = MS_ERR_OFFSET + 0x200,  // unkown error.
    MSERR_SERVICE_DIED,                               // meida service died
    MSERR_CREATE_REC_ENGINE_FAILED,                   // create recorder engine failed.
    MSERR_CREATE_PLAYER_ENGINE_FAILED,                // create player engine failed.
    MSERR_INVALID_STATE,                              // the state is not support this operation.
    MSERR_UNSUPPORT,                                  // unsupport interface.
    MSERR_UNSUPPORT_AUD_SRC_TYPE,                     // unsupport audio source type.
    MSERR_UNSUPPORT_AUD_SAMPLE_RATE,                  // unsupport audio sample rate.
    MSERR_UNSUPPORT_AUD_CHANNEL_NUM,                  // unsupport audio channel.
    MSERR_UNSUPPORT_AUD_ENC_TYPE,                     // unsupport audio encoder type.
    MSERR_UNSUPPORT_AUD_PARAMS,                       // unsupport audio params(other params).
    MSERR_UNSUPPORT_VID_SRC_TYPE,                     // unsupport video source type.
    MSERR_UNSUPPORT_VID_ENC_TYPE,                     // unsupport video encoder type.
    MSERR_UNSUPPORT_VID_PARAMS,                       // unsupport video params(other params).
    MSERR_UNSUPPORT_CONTAINER_TYPE,                   // unsupport container format type.
    MSERR_UNSUPPORT_PROTOCOL_TYPE,                    // unsupport protocol type.
    MSERR_UNSUPPORT_VID_DEC_TYPE,                     // unsupport video decoder type.
    MSERR_UNSUPPORT_AUD_DEC_TYPE,                     // unsupport audio decoder type.
    MSERR_AUD_ENC_FAILED,                             // audio encode failed.
    MSERR_VID_ENC_FAILED,                             // video encode failed.
    MSERR_AUD_DEC_FAILED,                             // audio decode failed.
    MSERR_VID_DEC_FAILED,                             // video decode failed.
    MSERR_MUXER_FAILED,                               // stream muxer failed.
    MSERR_DEMUXER_FAILED,                             // stream demuxer or parser failed.
    MSERR_OPEN_FILE_FAILED,                           // open file failed.
    MSERR_FILE_ACCESS_FAILED,                         // read or write file failed.
    MSERR_SEEK_FAILED,                                // audio/video seek failed.
    MSERR_NETWORK_TIMEOUT,                            // network timeout.
    MSERR_NOT_FIND_CONTAINER,                         // not find a demuxer.
    MSERR_EXTEND_START      = MS_ERR_OFFSET + 0xF000, // extend err start.
};

__attribute__((visibility("default"))) std::string MSErrorToString(MediaServiceErrCode code);
} // namespace Media
} // namespace OHOS
#endif // define MEDIA_ERRORS_H