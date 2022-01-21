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

#include "avsharedmemorypool.h"
#include "avsharedmemorybase.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVShMemPool"};
    constexpr int32_t MAX_MEM_SIZE = 100 * 1024 * 1024;
}

namespace OHOS {
namespace Media {
AVSharedMemoryPool::AVSharedMemoryPool(const std::string &name) : name_(name)
{
    MEDIA_LOGD("enter ctor, 0x%{public}06" PRIXPTR ", name: %{public}s", FAKE_POINTER(this), name_.c_str());
}

AVSharedMemoryPool::~AVSharedMemoryPool()
{
    MEDIA_LOGD("enter dtor, 0x%{public}06" PRIXPTR ", name: %{public}s", FAKE_POINTER(this), name_.c_str());
    Reset();
}

int32_t AVSharedMemoryPool::Init(const InitializeOption &option)
{
    std::unique_lock<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET_LOG(!inited_, MSERR_INVALID_OPERATION, "already inited !");
    CHECK_AND_RETURN_RET_LOG(option.memSize < MAX_MEM_SIZE, MSERR_INVALID_VAL, "size invalid");

    option_ = option;
    if (option.preAllocMemCnt > option.maxMemCnt) {
        option_.preAllocMemCnt = option.maxMemCnt;
    }

    MEDIA_LOGI("name: %{public}s, init option: preAllocMemCnt = %{public}u, memSize = %{public}d, "
               "maxMemCnt = %{public}u, enableRemoteRefCnt = %{public}d, enableFixedSize = %{public}d",
               name_.c_str(), option_.preAllocMemCnt, option_.memSize, option_.maxMemCnt,
               option_.enableRemoteRefCnt, option_.enableFixedSize);

    for (uint32_t i = 0; i < option_.preAllocMemCnt; ++i) {
        auto memory = AllocMemory(option_.memSize);
        CHECK_AND_RETURN_RET_LOG(memory != nullptr, MSERR_NO_MEMORY, "alloc memory failed");
        idleList_.push_back(memory);
    }

    inited_ = true;
    return MSERR_OK;
}

AVSharedMemory *AVSharedMemoryPool::AllocMemory(int32_t size)
{
    AVSharedMemoryBase *memory = nullptr;

    if (!option_.enableRemoteRefCnt) {
        memory = new (std::nothrow) AVSharedMemoryBase(size, option_.flags, name_);
    }
    CHECK_AND_RETURN_RET_LOG(memory != nullptr, nullptr, "create object failed");

    if (memory->Init() != MSERR_OK) {
        delete memory;
        MEDIA_LOGE("init avsharedmemorybase failed");
    }

    return memory;
}

void AVSharedMemoryPool::ReleaseMemory(AVSharedMemory *memory)
{
    std::unique_lock<std::mutex> lock(mutex_);

    for (auto iter = busyList_.begin(); iter != busyList_.end(); ++iter) {
        if (*iter == memory) {
            busyList_.erase(iter);
            idleList_.push_back(memory);
            cond_.notify_all();
            MEDIA_LOGD("0x%{public}06" PRIXPTR " released back to pool %{public}s",
                       FAKE_POINTER(memory), name_.c_str());
            return;
        }
    }

    MEDIA_LOGE("0x%{public}06" PRIXPTR " is no longer managed by this pool", FAKE_POINTER(memory));
    delete memory;
}

bool AVSharedMemoryPool::DoAcquireMemory(int32_t size, AVSharedMemory **outMemory)
{
    MEDIA_LOGD("busylist size %{public}zu, idlelist size %{public}zu", busyList_.size(), idleList_.size());

    AVSharedMemory *result = nullptr;
    std::list<AVSharedMemory *>::iterator minSizeIdleMem = idleList_.end();
    int32_t minIdleSize = std::numeric_limits<int32_t>::max();

    for (auto iter = idleList_.begin(); iter != idleList_.end(); ++iter) {
        if ((*iter)->GetSize() >= size) {
            result = *iter;
            idleList_.erase(iter);
            break;
        }

        if ((*iter)->GetSize() < minIdleSize) {
            minIdleSize = (*iter)->GetSize();
            minSizeIdleMem = iter;
        }
    }

    if (result == nullptr) {
        auto totalCnt = busyList_.size() + idleList_.size();
        if (totalCnt < option_.maxMemCnt) {
            result = AllocMemory(size);
            CHECK_AND_RETURN_RET(result != nullptr, false);
        }

        if (!option_.enableFixedSize && minSizeIdleMem != idleList_.end()) {
            delete *minSizeIdleMem;
            idleList_.erase(minSizeIdleMem);
            result = AllocMemory(size);
            CHECK_AND_RETURN_RET(result != nullptr, false);
        }
    }

    *outMemory = result;
    return true;
}

bool AVSharedMemoryPool::CheckSize(int32_t size)
{
    if (size <= 0 && size != -1) {
        return false;
    }

    if (!option_.enableFixedSize && size == -1) {
        return false;
    }

    if (option_.enableFixedSize) {
        if (size > option_.memSize) {
            return false;
        }

        if (size <= 0 && size != -1) {
            return false;
        }
    }

    return true;
}

std::shared_ptr<AVSharedMemory> AVSharedMemoryPool::AcquireMemory(int32_t size)
{
    std::unique_lock<std::mutex> lock(mutex_);

    if (!CheckSize(size)) {
        MEDIA_LOGE("invalid size: %{public}d", size);
        return nullptr;
    }

    MEDIA_LOGD("acquire memory for size: %{public}d from pool %{public}s", size, name_.c_str());
    if (option_.enableFixedSize) {
        size = option_.memSize;
    }

    AVSharedMemory *memory = nullptr;
    do {
        bool execute = DoAcquireMemory(size, &memory);
        if (execute && memory == nullptr) {
            cond_.wait(lock, [this]() { return !idleList_.empty() || !inited_; });
        } else {
            break;
        }
    } while (memory == nullptr && inited_);

    if (memory == nullptr) {
        MEDIA_LOGE("acquire memory failed for size: %{public}d", size);
        return nullptr;
    }

    busyList_.push_back(memory);

    auto result = std::shared_ptr<AVSharedMemory>(memory, [weakPool = weak_from_this()](AVSharedMemory *memory) {
        std::shared_ptr<AVSharedMemoryPool> pool = weakPool.lock();
        if (pool != nullptr) {
            pool->ReleaseMemory(memory);
        } else {
            MEDIA_LOGI("release memory 0x%{public}06" PRIXPTR ", but the pool is destroyed", FAKE_POINTER(memory));
            delete memory;
        }
    });

    MEDIA_LOGD("0x%{public}06" PRIXPTR " acquired from pool", FAKE_POINTER(memory));
    return result;
}

void AVSharedMemoryPool::SignalMemoryReleased()
{
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.notify_all();
}

void AVSharedMemoryPool::Reset()
{
    MEDIA_LOGD("Reset");

    std::unique_lock<std::mutex> lock(mutex_);
    for (auto &memory : idleList_) {
        delete memory;
        memory = nullptr;
    }
    idleList_.clear();
    inited_ = false;
    // for busylist, the memory will be released when the refcount of shared_ptr is zero.
}
}
}