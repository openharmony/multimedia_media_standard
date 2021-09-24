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

#ifndef PLAYBIN_MSG_DEFINE_H
#define PLAYBIN_MSG_DEFINE_H

#include <functional>
#include <any>

namespace OHOS {
namespace Media {
/**
 * Keep consistent with the define at player.h
 */
enum PlayBinMsgType : int32_t {
    PLAYBIN_MSG_ERROR = 0,
    PLAYBIN_MSG_SEEKDONE = 1,
    PLAYBIN_MSG_EOS,
    PLAYBIN_MSG_STATE_CHANGE,
    PLAYBIN_MSG_POSITION_UPDATE,
    PLAYBIN_MSG_SUBTYPE,
    PLAYBIN_MSG_VOLUME_CHANGE,
    PLAYBIN_MSG_EXTRA_FORMAT,
};

enum PlayBinMsgSubType : int32_t {
    PLAYBIN_SUB_MSG_UNKONWN = 0,
    PLAYBIN_SUB_MSG_VIDEO_RENDING_START,
    PLAYBIN_SUB_MSG_BUFFERING_START,
    PLAYBIN_SUB_MSG_BUFFERING_END,
    PLAYBIN_SUB_MSG_VIDEO_SIZE_CHANGED,
    PLAYBIN_SUB_MSG_BUFFER_PERCENT,
    PLAYBIN_SUB_MSG_WARNING,
    PLAYBIN_SUB_MSG_EXTEND_START = 0x1000,
};

enum PlayBinState : int32_t {
    PLAYBIN_STATE_IDLE,
    PLAYBIN_STATE_INITIALIZED,
    PLAYBIN_STATE_PREPARING,
    PLAYBIN_STATE_PREPARED,
    PLAYBIN_STATE_PLAYING,
    PLAYBIN_STATE_PAUSED,
    PLAYBIN_STATE_STOPPED,
    PLAYBIN_STATE_EOS,
};

struct PlayBinMessage {
    int32_t type; // info, error
    int32_t subType; // detail type
    int32_t code;
    std::any extra;
};

using PlayBinMsgNotifier = std::function<void(const PlayBinMessage &)>;
}
}

#endif