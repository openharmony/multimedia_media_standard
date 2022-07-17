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

#ifndef PLAYBIN_ACTION_EXECUTOR_H
#define PLAYBIN_ACTION_EXECUTOR_H

#include <list>
#include <memory>
#include "i_playbin_ctrler.h"
#include "playbin_async_action.h"

namespace OHOS {
namespace Media {
namespace PlayBin {
using Action = std::function<void()>;

// 添加一个定时器，用于周期性的检查还有多少异步Action没有执行，当前正在进行中的异步任务是哪个，执行了多久，等等维测信息
class ActionExecutor : public ActionObserver {
public:
    ActionExecutor(const std::shared_ptr<TaskQueue> &workLooper);
    ~ActionExecutor() = default;

    int32_t Enqueue(const std::shared_ptr<AsyncAction> &action);
    int32_t Enqueue(const Action &action);
    void HandleMessage(const InnerMessage &msg);
    int32_t Reset();

private:
    void OnActionDone(const AsyncAction &doneAction) override;

    std::shared_ptr<TaskQueue> workLooper_;
    std::list<std::shared_ptr<AsyncAction>> pendingActionList_;
    std::shared_ptr<AsyncAction> currAction_;
};
}
}
}

#endif