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

#include "media_errors.h"
#include <map>
#include <string>

namespace OHOS {
namespace Media {
const std::map<MediaServiceErrCode, std::string> MSERRCODE_INFOS = {
    {MSERR_OK, "success"},
    {MSERR_NO_MEMORY, "no memory"},
    {MSERR_INVALID_OPERATION, "opertation not be permitted"},
    {MSERR_INVALID_VAL, "invalid argument"},
    {MSERR_UNKNOWN, "unkown error"},
    {MSERR_SERVICE_DIED, "meida service died"},
    {MSERR_CREATE_REC_ENGINE_FAILED, "create recorder engine failed"},
    {MSERR_CREATE_PLAYER_ENGINE_FAILED, "create player engine failed"},
    {MSERR_CREATE_AVMETADATAHELPER_ENGINE_FAILED, "create avmetadatahelper engine failed"},
    {MSERR_INVALID_STATE, "the state is not support this operation"},
    {MSERR_UNSUPPORT, "unsupport interface"},
    {MSERR_UNSUPPORT_AUD_SRC_TYPE, "unsupport audio source type"},
    {MSERR_UNSUPPORT_AUD_SAMPLE_RATE, "unsupport audio sample rate"},
    {MSERR_UNSUPPORT_AUD_CHANNEL_NUM, "unsupport audio channel"},
    {MSERR_UNSUPPORT_AUD_ENC_TYPE, "unsupport audio encoder type"},
    {MSERR_UNSUPPORT_AUD_PARAMS, "unsupport audio params(other params)"},
    {MSERR_UNSUPPORT_VID_SRC_TYPE, "unsupport video source type"},
    {MSERR_UNSUPPORT_VID_ENC_TYPE, "unsupport video encoder type"},
    {MSERR_UNSUPPORT_VID_PARAMS, "unsupport video params(other params)"},
    {MSERR_UNSUPPORT_CONTAINER_TYPE, "unsupport container format type"},
    {MSERR_UNSUPPORT_PROTOCOL_TYPE, "unsupport protocol type"},
    {MSERR_UNSUPPORT_VID_DEC_TYPE, "unsupport video decoder type"},
    {MSERR_UNSUPPORT_AUD_DEC_TYPE, "unsupport audio decoder type"},
    {MSERR_AUD_ENC_FAILED, "audio encode failed"},
    {MSERR_VID_ENC_FAILED, "video encode failed"},
    {MSERR_AUD_DEC_FAILED, "audio decode failed"},
    {MSERR_VID_DEC_FAILED, "video decode failed"},
    {MSERR_MUXER_FAILED, "stream muxer failed"},
    {MSERR_DEMUXER_FAILED, "stream demuxer or parser failed"},
    {MSERR_OPEN_FILE_FAILED, "open file failed"},
    {MSERR_FILE_ACCESS_FAILED, "read or write file failed"},
    {MSERR_START_FAILED, "audio or video start failed"},
    {MSERR_PAUSE_FAILED, "audio or video pause failed"},
    {MSERR_STOP_FAILED, "audio or video stop failed"},
    {MSERR_SEEK_FAILED, "audio or video seek failed"},
    {MSERR_NETWORK_TIMEOUT, "network timeout"},
    {MSERR_NOT_FIND_CONTAINER, "not find a demuxer"},
    {MSERR_EXTEND_START, "extend start error code"},
};

const std::map<MediaServiceErrCode, MediaServiceExtErrCode> MSERRCODE_TO_EXTERRORCODE = {
    {MSERR_OK,                                  MSERR_EXT_OK},
    {MSERR_NO_MEMORY,                           MSERR_EXT_NO_MEMORY},
    {MSERR_INVALID_OPERATION,                   MSERR_EXT_OPERATE_NOT_PERMIT},
    {MSERR_INVALID_VAL,                         MSERR_EXT_INVALID_VAL},
    {MSERR_UNKNOWN,                             MSERR_EXT_UNKNOWN},
    {MSERR_SERVICE_DIED,                        MSERR_EXT_SERVICE_DIED},
    {MSERR_CREATE_REC_ENGINE_FAILED,            MSERR_EXT_UNKNOWN},
    {MSERR_CREATE_PLAYER_ENGINE_FAILED,         MSERR_EXT_UNKNOWN},
    {MSERR_INVALID_STATE,                       MSERR_EXT_INVALID_STATE},
    {MSERR_UNSUPPORT,                           MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_AUD_SRC_TYPE,              MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_AUD_SAMPLE_RATE,           MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_AUD_CHANNEL_NUM,           MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_AUD_ENC_TYPE,              MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_AUD_PARAMS,                MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_VID_SRC_TYPE,              MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_VID_ENC_TYPE,              MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_VID_PARAMS,                MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_CONTAINER_TYPE,            MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_PROTOCOL_TYPE,             MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_VID_DEC_TYPE,              MSERR_EXT_UNSUPPORT},
    {MSERR_UNSUPPORT_AUD_DEC_TYPE,              MSERR_EXT_UNSUPPORT},
    {MSERR_AUD_ENC_FAILED,                      MSERR_EXT_UNKNOWN},
    {MSERR_VID_ENC_FAILED,                      MSERR_EXT_UNKNOWN},
    {MSERR_AUD_DEC_FAILED,                      MSERR_EXT_UNKNOWN},
    {MSERR_VID_DEC_FAILED,                      MSERR_EXT_UNKNOWN},
    {MSERR_MUXER_FAILED,                        MSERR_EXT_UNKNOWN},
    {MSERR_DEMUXER_FAILED,                      MSERR_EXT_UNKNOWN},
    {MSERR_OPEN_FILE_FAILED,                    MSERR_EXT_UNKNOWN},
    {MSERR_FILE_ACCESS_FAILED,                  MSERR_EXT_UNKNOWN},
    {MSERR_START_FAILED,                        MSERR_EXT_UNKNOWN},
    {MSERR_PAUSE_FAILED,                        MSERR_EXT_UNKNOWN},
    {MSERR_STOP_FAILED,                         MSERR_EXT_UNKNOWN},
    {MSERR_SEEK_FAILED,                         MSERR_EXT_UNKNOWN},
    {MSERR_NETWORK_TIMEOUT,                     MSERR_EXT_TIMEOUT},
    {MSERR_NOT_FIND_CONTAINER,                  MSERR_EXT_UNSUPPORT},
    {MSERR_EXTEND_START,                        MSERR_EXT_EXTEND_START},
};

const std::map<MediaServiceExtErrCode, std::string> MSEXTERRCODE_INFOS = {
    {MSERR_EXT_OK, "success"},
    {MSERR_EXT_NO_MEMORY, "no memory"},
    {MSERR_EXT_OPERATE_NOT_PERMIT, "opertation not be permitted"},
    {MSERR_EXT_INVALID_VAL, "invalid argument"},
    {MSERR_EXT_IO, "IO error"},
    {MSERR_EXT_TIMEOUT, "network timeout"},
    {MSERR_EXT_UNKNOWN, "unkown error"},
    {MSERR_EXT_SERVICE_DIED, "meida service died"},
    {MSERR_EXT_INVALID_STATE, "the state is not support this operation"},
    {MSERR_EXT_UNSUPPORT, "unsupport interface"},
    {MSERR_EXT_EXTEND_START, "extend err start"},
};

std::string MSErrorToString(MediaServiceErrCode code)
{
    if (MSERRCODE_INFOS.count(code) != 0) {
        return MSERRCODE_INFOS.at(code);
    }

    if (code > MSERR_EXTEND_START) {
        return "extend error:" + std::to_string(static_cast<int32_t>(code - MSERR_EXTEND_START));
    }

    return "invalid error code:" + std::to_string(static_cast<int32_t>(code));
}

std::string MSExtErrorToString(MediaServiceExtErrCode code)
{
    if (MSEXTERRCODE_INFOS.count(code) != 0) {
        return MSEXTERRCODE_INFOS.at(code);
    }

    if (code > MSERR_EXT_EXTEND_START) {
        return "extend error:" + std::to_string(static_cast<int32_t>(code - MSERR_EXTEND_START));
    }

    return "invalid error code:" + std::to_string(static_cast<int32_t>(code));
}

std::string MSErrorToExtErrorString(MediaServiceErrCode code)
{
    if (MSERRCODE_INFOS.count(code) != 0 && MSERRCODE_TO_EXTERRORCODE.count(code) != 0) {
        MediaServiceExtErrCode extCode = MSERRCODE_TO_EXTERRORCODE.at(code);
        if (MSEXTERRCODE_INFOS.count(extCode) != 0) {
            return MSEXTERRCODE_INFOS.at(extCode);
        }
    }

    return "unkown error";
}

MediaServiceExtErrCode MSErrorToExtError(MediaServiceErrCode code)
{
    if (MSERRCODE_INFOS.count(code) != 0 && MSERRCODE_TO_EXTERRORCODE.count(code) != 0) {
        return MSERRCODE_TO_EXTERRORCODE.at(code);
    }

    return MSERR_EXT_UNKNOWN;
}
} // namespace Media
} // namespace OHOS
