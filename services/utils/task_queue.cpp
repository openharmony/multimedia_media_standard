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
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "TaskQueue"};
}

namespace OHOS {
namespace Media {
TaskQueue::~TaskQueue()
{
    (void)Stop();
}

int32_t TaskQueue::Start()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (thread_ != nullptr) {
        MEDIA_LOGW("Started already, ignore ! [%{public}s]", name_.c_str());
        return MSERR_OK;
    }
    isExit_ = false;
    thread_ = std::make_unique<std::thread>(&TaskQueue::TaskProcessor, this);

    return MSERR_OK;
}

int32_t TaskQueue::Stop() noexcept
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (isExit_) {
        MEDIA_LOGI("Stopped already, ignore ! [%{public}s]", name_.c_str());
        return MSERR_OK;
    }

    if (std::this_thread::get_id() == thread_->get_id()) {
        MEDIA_LOGI("Stop at the task thread, reject");
        return MSERR_INVALID_OPERATION;
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
    return MSERR_OK;
}

// cancelNotExecuted = false, delayUs = 0ULL.
int32_t TaskQueue::EnqueueTask(const std::shared_ptr<ITaskHandler> &task, bool cancelNotExecuted, uint64_t delayUs)
{
    constexpr uint64_t MAX_DELAY_US = 10000000ULL; // max delay.

    CHECK_AND_RETURN_RET_LOG(task != nullptr, MSERR_INVALID_VAL,
        "Enqueue task when taskqueue task is nullptr.[%{public}s]", name_.c_str());

    task->Clear();

    CHECK_AND_RETURN_RET_LOG(delayUs < MAX_DELAY_US, MSERR_INVALID_VAL,
        "Enqueue task when taskqueue delayUs[%{public}" PRIu64 "] is >= max delayUs[ %{public}" PRIu64
        "], invalid! [%{public}s]",
        delayUs, MAX_DELAY_US, name_.c_str());

    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(!isExit_, MSERR_INVALID_OPERATION,
        "Enqueue task when taskqueue is stopped, failed ! [%{public}s]", name_.c_str());

    if (cancelNotExecuted) {
        CancelNotExecutedTaskLocked();
    }

    // 1000 is ns to us.
    constexpr uint32_t US_TO_NS = 1000;
    uint64_t curTimeNs = static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    CHECK_AND_RETURN_RET_LOG(curTimeNs < UINT64_MAX - delayUs * US_TO_NS, MSERR_INVALID_OPERATION,
        "Enqueue task but timestamp is overflow, why? [%{public}s]", name_.c_str());

    uint64_t executeTimeNs = delayUs * US_TO_NS + curTimeNs;
    auto iter = std::find_if(taskList_.begin(), taskList_.end(), [executeTimeNs](const TaskHandlerItem &item) {
        return (item.executeTimeNs_ > executeTimeNs);
    });
    (void)taskList_.insert(iter, {task, executeTimeNs});
    cond_.notify_all();

    return 0;
}

void TaskQueue::CancelNotExecutedTaskLocked()
{
    MEDIA_LOGI("All task not executed are being cancelled..........[%{public}s]", name_.c_str());
    while (!taskList_.empty()) {
        std::shared_ptr<ITaskHandler> task = taskList_.front().task_;
        taskList_.pop_front();
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
        cond_.wait(lock, [this] { return isExit_ || !taskList_.empty(); });
        if (isExit_) {
            MEDIA_LOGI("Exit TaskProcessor [%{public}s]", name_.c_str());
            return;
        }
        TaskHandlerItem item = taskList_.front();
        uint64_t curTimeNs = static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
        if (curTimeNs >= item.executeTimeNs_) {
            taskList_.pop_front();
        } else {
            uint64_t diff =  item.executeTimeNs_ - curTimeNs;
            (void)cond_.wait_for(lock, std::chrono::nanoseconds(diff));
            continue;
        }
        lock.unlock();

        if (item.task_ == nullptr || item.task_->IsCanceled()) {
            MEDIA_LOGD("task is nullptr or task canceled. [%{public}s]", name_.c_str());
            continue;
        }

        item.task_->Execute();
        if (item.task_->GetAttribute().periodicTimeUs_ == UINT64_MAX) {
            continue;
        }
        int32_t res = EnqueueTask(item.task_, false, item.task_->GetAttribute().periodicTimeUs_);
        if (res != MSERR_OK) {
            MEDIA_LOGW("enqueue periodic task failed:%d, why? [%{public}s]", res, name_.c_str());
        }
    }
    MEDIA_LOGI("Leave TaskProcessor [%{public}s]", name_.c_str());
}
}
}
