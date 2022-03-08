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

#ifndef PLAYBIN_TASK_MGR_H
#define PLAYBIN_TASK_MGR_H

#include <list>
#include <mutex>
#include "task_queue.h"

namespace OHOS {
namespace Media {
enum PlayBinTaskType : uint8_t {
    /**
     * A preemption task will be directly queued to the TaskQueue, regardless of the two-phase
     * task pending list.
     */
    PREEMPT,
    /**
     * The following types are treated as two-phase tasks. Such a task consists of two phases.
     * The first phase is initiated by the taskhandler corresponding to the task, and the second
     * phase needs to be manually marked. The next two-phase task is blocked until the second
     * stage of the currently executing two-phase task is marked.
     *
     * If a two-phase task of the same type already exists in the two-phase task pending list,
     * it will be replaced, regardless of the fact that the new task is a late entry.
     */
    STATE_CHANGE,
    SEEKING,
    RATE_CHANGE,
    BUTT,
};

class PlayBinTaskMgr {
public:
    PlayBinTaskMgr();
    ~PlayBinTaskMgr();

    int32_t Init();
    int32_t LaunchTask(const std::shared_ptr<ITaskHandler> &task, PlayBinTaskType type, uint64_t delayUs = 0);
    // only take effect when it is called at the task thread.
    int32_t MarkSecondPhase();
    PlayBinTaskType GetCurrTaskType();
    void ClearAllTask();
    int32_t Reset();

private:
    struct TwoPhaseTaskItem {
        PlayBinTaskType type;
        std::shared_ptr<ITaskHandler> task;
    };

    std::unique_ptr<TaskQueue> taskThread_;
    std::shared_ptr<ITaskHandler> currTwoPhaseTask_;
    PlayBinTaskType currTwoPhaseType_ = PlayBinTaskType::PREEMPT;
    std::list<TwoPhaseTaskItem> pendingTwoPhaseTasks_;
    bool isInited_ = false;
    std::thread::id taskThreadId_;
    std::mutex mutex_;
};
} // namespace Media
} // namespace OHOS
#endif // PLAYBIN_TASK_MGR_H
