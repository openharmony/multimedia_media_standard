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

#ifndef PLAYER_SERVER_TASK_MGR_H
#define PLAYER_SERVER_TASK_MGR_H

#include <list>
#include <mutex>
#include "task_queue.h"

namespace OHOS {
namespace Media {
enum PlayerServerTaskType : uint8_t {
    /**
     * The following types are treated as two-phase tasks. Such a task consists of two phases.
     * The first phase is initiated by the taskhandler corresponding to the task, and the second
     * phase needs to be manually marked. The next two-phase task is blocked until the second
     * stage of the currently executing two-phase task is marked.
     *
     * If a two-phase task of the same type (except for STATE_CHANGE) already exists in the two-phase task pending list,
     * it will be replaced, regardless of the fact that the new task is a late entry.
     */
    STATE_CHANGE,
    SEEKING,
    RATE_CHANGE,
    BUTT,
};

class PlayerServerTaskMgr {
public:
    PlayerServerTaskMgr();
    ~PlayerServerTaskMgr();

    int32_t Init();
    int32_t LaunchTask(const std::shared_ptr<ITaskHandler> &task, PlayerServerTaskType type, uint64_t delayUs = 0);
    // only take effect when it is called at the task thread.
    int32_t MarkTaskDone();
    PlayerServerTaskType GetCurrTaskType();
    void ClearAllTask();
    int32_t Reset();

private:
    struct TwoPhaseTaskItem {
        PlayerServerTaskType type;
        std::shared_ptr<ITaskHandler> task;
    };

    std::unique_ptr<TaskQueue> taskThread_;
    std::shared_ptr<ITaskHandler> currTwoPhaseTask_;
    PlayerServerTaskType currTwoPhaseType_ = PlayerServerTaskType::BUTT;
    std::list<TwoPhaseTaskItem> pendingTwoPhaseTasks_;
    bool isInited_ = false;
    std::thread::id taskThreadId_;
    std::mutex mutex_;
};
}
}

#endif // PLAYER_SERVER_TASK_MGR_H
