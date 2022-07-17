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

#include <string_view>
#include "inner_msg_define.h"

namespace OHOS {
namespace Media {
namespace PlayBin {
enum PlayBinMsgType : int32_t {
    PLAYBIN_MSG_SEEK_DONE = INNER_MSG_COMMON_END + 1,
    PLAYBIN_MSG_SPEED_DONE,
    PLAYBIN_MSG_ELEM_SETUP,
    PLAYBIN_MSG_ELEM_UNSETUP,
};

enum PlayBinState : int32_t {
    PLAYBIN_STATE_ERROR,
    PLAYBIN_STATE_IDLE,
    PLAYBIN_STATE_PREPARING,
    PLAYBIN_STATE_PREPARED,
    PLAYBIN_STATE_PLAYING,
    PLAYBIN_STATE_PAUSED,
    PLAYBIN_STATE_STOPPED,
    PLAYBIN_STATE_BUTT,
};

std::string_view StringifyPlayBinState(PlayBinState state);
}
}
}

#endif