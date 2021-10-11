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

#include "playbin_task_mgr.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayBinTaskMgr"};
}

namespace OHOS {
namespace Media {
PlayBinTaskMgr::PlayBinTaskMgr() : taskThread_("playbin_task_mgr")
{
    MEDIA_LOGD("enter ctor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
}

PlayBinTaskMgr::~PlayBinTaskMgr()
{
    MEDIA_LOGD("enter dtor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
    (void)Reset();
}

int32_t PlayBinTaskMgr::Init()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (isInited_) {
        return MSERR_OK;
    }

    int32_t ret = taskThread_.Start();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "task thread start failed");

    auto dummyTask = std::make_shared<TaskHandler<void>>([this]() {
        taskThreadId_ = std::this_thread::get_id();
    });

    ret = taskThread_.EnqueueTask(dummyTask);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "try get thread id failed");

    auto result = dummyTask->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "try get thread id failed");

    isInited_ = true;

    return MSERR_OK;
}

int32_t PlayBinTaskMgr::LaunchTask(const std::shared_ptr<ITaskHandler> &task, PlayBinTaskType type, uint64_t delayUs)
{
    if (type >= PlayBinTaskType::BUTT) {
        MEDIA_LOGE("invalid task type");
        return MSERR_INVALID_VAL;
    }

    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(isInited_, MSERR_INVALID_OPERATION, "not init");

    if (type == PlayBinTaskType::PREEMPT) {
        int32_t ret = taskThread_.EnqueueTask(task);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "launch preempt task failed");
        return MSERR_OK;
    }

    if (currTwoPhaseTask_ == nullptr) {
        int32_t ret = taskThread_.EnqueueTask(task);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "launch two phase task failed");
        currTwoPhaseTask_ = task;
        currTwoPhaseType_ = type;
        return MSERR_OK;
    }

    MEDIA_LOGI("current two phase task is in processing, pending the new tow phase task, type: %{public}hhu", type);
    for (auto &item : pendingTwoPhaseTasks_)  {
        if (item.type == type) {
            item.task = task;
            MEDIA_LOGI("replace the old two phase task");
            return MSERR_OK;
        }
    }

    pendingTwoPhaseTasks_.push_back({type, task});
    return MSERR_OK;
}

int32_t PlayBinTaskMgr::MarkSecondPhase()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(isInited_, MSERR_INVALID_OPERATION, "not init");

    if (std::this_thread::get_id() != taskThreadId_) {
        MEDIA_LOGE("not in the task thread, ignored");
        return MSERR_INVALID_OPERATION;
    }

    currTwoPhaseTask_ = nullptr;
    currTwoPhaseType_ = PlayBinTaskType::PREEMPT;

    if (pendingTwoPhaseTasks_.empty()) {
        return MSERR_OK;
    }

    auto item = pendingTwoPhaseTasks_.front();
    pendingTwoPhaseTasks_.pop_front();
    currTwoPhaseTask_ = item.task;
    currTwoPhaseType_ = item.type;

    int32_t ret = taskThread_.EnqueueTask(currTwoPhaseTask_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret,
        "execute the stack top task failed, type: %{public}hhu", item.type);

    MEDIA_LOGI("launch the two phase task, type: %{public}hhu", item.type);
    return MSERR_OK;
}

PlayBinTaskType PlayBinTaskMgr::GetCurrTaskType()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return currTwoPhaseType_;
}

void PlayBinTaskMgr::ClearAllTask()
{
    MEDIA_LOGD("enter");
    std::unique_lock<std::mutex> lock(mutex_);

    currTwoPhaseTask_ = nullptr;
    currTwoPhaseType_ = PlayBinTaskType::PREEMPT;
    pendingTwoPhaseTasks_.clear();

    auto dummyTask = std::make_shared<TaskHandler<void>>([]() {
        MEDIA_LOGI("execute dummy task...");
    });
    (void)taskThread_.EnqueueTask(dummyTask, true, 0);
    MEDIA_LOGD("exit");
}

int32_t PlayBinTaskMgr::Reset()
{
    MEDIA_LOGD("enter");
    std::unique_lock<std::mutex> lock(mutex_);

    int32_t ret = taskThread_.Stop();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "stop task thread failed");

    currTwoPhaseTask_ = nullptr;
    currTwoPhaseType_ = PlayBinTaskType::PREEMPT;
    pendingTwoPhaseTasks_.clear();
    isInited_ = false;

    MEDIA_LOGD("exit");
    return MSERR_OK;
}
}
}
