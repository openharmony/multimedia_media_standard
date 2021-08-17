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

#include "task_queue.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "TaskQueue"};
}

namespace OHOS {
namespace Media {
TaskQueue::~TaskQueue()
{
    Stop();
}

int32_t TaskQueue::Start()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (thread_ != nullptr) {
        MEDIA_LOGW("Started already, ignore ! [%{public}s]", name_.c_str());
        return ERR_OK;
    }
    isExit_ = false;
    thread_ = std::make_unique<std::thread>(&TaskQueue::TaskProcessor, this);

    return ERR_OK;
}

int32_t TaskQueue::Stop()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (isExit_) {
        MEDIA_LOGI("Stopped already, ignore ! [%{public}s]", name_.c_str());
        return ERR_OK;
    }

    std::unique_ptr<std::thread> t;
    isExit_ = true;
    cond_.notify_all();
    std::swap(thread_, t);
    lock.unlock();

    if (t != nullptr && t->joinable()) {
        t->join();
    }

    lock.lock();
    CancelNotExecutedTaskLocked();
    return ERR_OK;
}

int32_t TaskQueue::EnqueueTask(std::shared_ptr<TaskHandler> task, bool cancelNotExecuted)
{
    if (task == nullptr) {
        return -1;
    }

    std::unique_lock<std::mutex> lock(mutex_);
    if (isExit_) {
        MEDIA_LOGE("Enqueue task when taskqueue is stopped, failed ! [%{public}s]", name_.c_str());
        return ERR_INVALID_OPERATION;
    }

    if (cancelNotExecuted) {
        CancelNotExecutedTaskLocked();
    }

    taskQ_.push(task);
    cond_.notify_all();

    return 0;
}

void TaskQueue::CancelNotExecutedTaskLocked()
{
    MEDIA_LOGI("All task not executed are being cancelled..........[%{public}s]", name_.c_str());
    while (!taskQ_.empty()) {
        std::shared_ptr<TaskHandler> task = taskQ_.front();
        taskQ_.pop();
        if (task != nullptr) {
            task->Cancel();
        }
    }
}

void TaskQueue::TaskProcessor()
{
    MEDIA_LOGI("Enter TaskProcessor [%{public}s]", name_.c_str());
    while (true) {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] { return isExit_ || !taskQ_.empty(); });
        if (isExit_) {
            MEDIA_LOGI("Exit TaskProcessor [%{public}s]", name_.c_str());
            return;
        }
        std::shared_ptr<TaskHandler> task = taskQ_.front();
        taskQ_.pop();
        lock.unlock();

        if (task != nullptr) {
            task->Execute();
        }
    }
}
}
}