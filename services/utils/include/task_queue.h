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
#include <list>
#include <string>
#include <optional>
#include <type_traits>
#include "media_errors.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
/**
 * Simple Generalized Task Queues for Easier Implementation of Asynchronous Programming Models
 *
 * You can refer to following examples to use this utility.
 *
 * Example 1:
 * TaskQueue taskQ("your_task_queue_name");
 * taskQ.Start();
 * auto handler1 = std::make_shared<TaskHandler<int32_t>>([]() {
 *     // your job's detail code;
 * });
 * taskQ.EnqueueTask(handler1);
 * auto result = handler1->GetResult();
 * if (result.HasResult()) {
 *     MEDIA_LOGI("handler1 executed, result: %{public}d", result.Value());
 * } else {
 *     MEDIA_LOGI("handler1 not executed");
 * }
 *
 * Example 2:
 * TaskQueue taskQ("your_task_queue_name");
 * taskQ.Start();
 * auto handler2 = std::make_shared<TaskHandler<void>>([]() {
 *     // your job's detail code;
 * });
 * taskQ.EnqueueTask(handler2);
 * auto result = handler2->GetResult();
 * if (result.HasResult()) {
 *     MEDIA_LOGI("handler2 executed");
 * } else {
 *     MEDIA_LOGI("handler2 not executed");
 * }
 */

class TaskQueue;
template <typename T>
class TaskHandler;

template <typename T>
struct TaskResult {
    bool HasResult()
    {
        return val.has_value();
    }
    T Value()
    {
        return val.value();
    }
private:
    friend class TaskHandler<T>;
    std::optional<T> val;
};

template <>
struct TaskResult<void> {
    bool HasResult()
    {
        return executed;
    }
private:
    friend class TaskHandler<void>;
    bool executed = false;
};

class ITaskHandler {
public:
    struct Attribute {
        // periodic execute time, UINT64_MAX is not need to execute periodic.
        uint64_t periodicTimeUs_ { UINT64_MAX };
    };
    virtual ~ITaskHandler() = default;
    virtual void Execute() = 0;
    virtual void Cancel() = 0;
    virtual bool IsCanceled() = 0;
    virtual Attribute GetAttribute() const = 0;

private:
    // clear the internel executed or canceled state.
    virtual void Clear() = 0;
    friend class TaskQueue;
};

template <typename T>
class TaskHandler : public ITaskHandler, public NoCopyable {
public:
    TaskHandler(std::function<T(void)> task, ITaskHandler::Attribute attr = {}) : task_(task), attribute_(attr) {}
    ~TaskHandler() = default;

    void Execute() override
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (state_ != TaskState::IDLE) {
                return;
            }
            state_ = TaskState::RUNNING;
        }

        if constexpr (std::is_void_v<T>) {
            task_();
            std::unique_lock<std::mutex> lock(mutex_);
            state_ = TaskState::FINISHED;
            result_.executed = true;
        } else {
            T result = task_();
            std::unique_lock<std::mutex> lock(mutex_);
            state_ = TaskState::FINISHED;
            result_.val = result;
        }
        cond_.notify_all();
    }

    /*
     * After the GetResult called, the last execute result will be clear
     */
    TaskResult<T> GetResult()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while ((state_ != TaskState::FINISHED) && (state_ != TaskState::CANCELED)) {
            cond_.wait(lock);
        }

        return ClearResult();
    }

    void Cancel() override
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (state_ != RUNNING) {
            state_ = TaskState::CANCELED;
            cond_.notify_all();
        }
    }

    bool IsCanceled() override
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return state_ == TaskState::CANCELED;
    }

    ITaskHandler::Attribute GetAttribute() const override
    {
        return attribute_;
    }

private:
    TaskResult<T> ClearResult()
    {
        if (state_ == TaskState::FINISHED) {
            state_ = TaskState::IDLE;
            TaskResult<T> tmp;
            if constexpr (std::is_void_v<T>) {
                std::swap(tmp.executed, result_.executed);
            } else {
                result_.val.swap(tmp.val);
            }
            return tmp;
        }
        return result_;
    }

    void Clear() override
    {
        std::unique_lock<std::mutex> lock(mutex_);
        (void)ClearResult();
    }

    enum TaskState {
        IDLE,
        RUNNING,
        CANCELED,
        FINISHED,
    };

    TaskState state_ = TaskState::IDLE;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::function<T(void)> task_;
    TaskResult<T> result_;
    ITaskHandler::Attribute attribute_; // task execute attribute.
};

class __attribute__((visibility("default"))) TaskQueue : public NoCopyable {
public:
    explicit TaskQueue(const std::string &name) : name_(name) {}
    ~TaskQueue();

    int32_t Start();
    int32_t Stop() noexcept;

    // delayUs cannot be gt 10000000ULL.
    int32_t EnqueueTask(const std::shared_ptr<ITaskHandler> &task,
        bool cancelNotExecuted = false, uint64_t delayUs = 0ULL);

private:
    struct TaskHandlerItem {
        std::shared_ptr<ITaskHandler> task_ { nullptr };
        uint64_t executeTimeNs_ { 0ULL };
    };
    void TaskProcessor();
    void CancelNotExecutedTaskLocked();

    bool isExit_ = true;
    std::unique_ptr<std::thread> thread_;
    std::list<TaskHandlerItem> taskList_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::string name_;
};
} // namespace Media
} // namespace OHOS
#endif
