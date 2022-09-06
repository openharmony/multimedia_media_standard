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

#ifndef INNER_MSG_DEFINE_H
#define INNER_MSG_DEFINE_H

#include <memory>
#include <any>
#include <functional>

namespace OHOS {
namespace Media {
enum InnerMsgType : int32_t {
    INNER_MSG_UNKNOWN,
    INNER_MSG_ERROR,
    INNER_MSG_WARNING,
    INNER_MSG_INFO,
    INNER_MSG_EOS,
    INNER_MSG_STATE_CHANGED,
    INNER_MSG_DURATION_CHANGED,
    INNER_MSG_RESOLUTION_CHANGED,
    INNER_MSG_ASYNC_DONE,
    INNER_MSG_BUFFERING,
    INNER_MSG_BUFFERING_TIME,
    INNER_MSG_BUFFERING_USED_MQ_NUM,
    INNER_MSG_POSITION_UPDATE,
    INNER_MSG_VIDEO_ROTATION,
};

struct InnerMessage {
    int32_t type;
    int32_t detail1;
    int32_t detail2;
    std::any extend;
};

using InnerMsgNotifier = std::function<void(const InnerMessage &)>;
} // namespace Media
} // namespace OHOS
#endif // INNER_MSG_DEFINE_H