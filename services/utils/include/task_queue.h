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

#ifndef RECORDER_TASK_QUEUE_H
#define RECORDER_TASK_QUEUE_H

#include <thread>
#include <condition_variable>
#include <mutex>
#include <functional>
#include <queue>
#include <string>
#include "errors.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class TaskHandler {
public:
    TaskHandler(std::function<int32_t(void)> task, int32_t defaultResult = ERR_OK)
        : task_(task), result_(defaultResult)
    {
    }
    ~TaskHandler() = default;

    void Execute()
    {
        int32_t result = task_();
        {
            std::unique_lock<std::mutex> lock(mutex_);
            isFinished_ = true;
            result_ = result;
        }
        cond_.notify_all();
    }

    int32_t GetResult()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!isFinished_) {
            cond_.wait(lock);
        }
        return result_;
    }

    void Cancel()
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            isFinished_ = true;
        }
        cond_.notify_all();
    }

    DISALLOW_COPY_AND_MOVE(TaskHandler);

private:
    bool isFinished_ = false;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::function<int32_t(void)> task_;
    int32_t result_;
};

class __attribute__((visibility("default"))) TaskQueue {
public:
    explicit TaskQueue(const std::string &name) : name_(name) {}
    ~TaskQueue();

    int32_t Start();
    int32_t Stop();
    int32_t EnqueueTask(std::shared_ptr<TaskHandler> task, bool cancelNotExecuted = false);

    DISALLOW_COPY_AND_MOVE(TaskQueue);

private:
    void TaskProcessor();
    void CancelNotExecutedTaskLocked();

    bool isExit_ = true;
    std::unique_ptr<std::thread> thread_;
    std::queue<std::shared_ptr<TaskHandler>> taskQ_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::string name_;
};
}
}
#endif