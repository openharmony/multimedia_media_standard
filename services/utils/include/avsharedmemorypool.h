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

#ifndef AVSHAREDMEMORYPOOL_H
#define AVSHAREDMEMORYPOOL_H

#include <memory>
#include <list>
#include <mutex>
#include <condition_variable>
#include "nocopyable.h"
#include "avsharedmemorybase.h"

namespace OHOS {
namespace Media {
/**
 * @brief A simple pool implementation for shared memory.
 *
 * This pool support multi configuration:
 * @preAllocMemCnt: The number of memory blocks allocated when the pool is intialized.
 * @memSize: the size of the preallocated memory blocks.
 * @maxMemCnt: the total number of memory blocks in the pool.
 * @flags: the shared memory access property, refer to {@AVSharedMemory::Flags}.
 * @enableRemoteRefCnt: if true, the pool will allocate the RefCntSharedMemory, or AVSharedMemoryBase
 * @enableFixedSize: if true, the pool will allocate all memory block using the memSize. If the acquired
 *                  size is larger than the memSize, the acquire will failed. If false, the pool will
 *                  free the smallest idle memory block when there is no idle memory block that can
 *                  satisfy the acqiured size and reallocate a new memory block with the acquired size.
 */
class __attribute__((visibility("default"))) AVSharedMemoryPool
    : public std::enable_shared_from_this<AVSharedMemoryPool> {
public:
    AVSharedMemoryPool(const std::string &name);
    ~AVSharedMemoryPool();

    struct InitializeOption {
        uint32_t preAllocMemCnt = 0;
        int32_t memSize;
        uint32_t maxMemCnt;
        uint32_t flags = AVSharedMemory::Flags::FLAGS_READ_WRITE;
        bool enableRemoteRefCnt = false;
        bool enableFixedSize = true;
    };

    /**
     * @brief Initialize the pool and preallocate some memory blocks.
     */
    int32_t Init(const InitializeOption &option);

    /**
     * @brief Acquire a memory from the pool. If there is no available memory block, it will
     * be blocked until an available memory released back to pool.
     *
     * If the acquired memory need to be release back to pool, just set it to nullptr. If the
     * underlying memory is RefCntSharedMemory, make sure that the refcount is set to zero.
     *
     * @param size the expected memory size. if the enableFixedSize is configured, this param can be empty.
     * @return valid memory block if success, or nullptr.
     */
    std::shared_ptr<AVSharedMemory> AcquireMemory(int32_t size = -1);

    /**
     * @brief Waken up the waiters of pool if there are any memory blocks released bakc to pool. This
     * interface will be useful when the underlying memory is RefCntSharedMemory.
     */
    void SignalMemoryReleased();

    /**
     * @brief Free all memory blocks and reset the pool configuration. After reseted, all waiters of
     * the pool will be waken up and returned with a nullptr.
     */
    void Reset();

    std::string GetName()
    {
        return name_;
    }

    DISALLOW_COPY_AND_MOVE(AVSharedMemoryPool);

private:
    bool DoAcquireMemory(int32_t size, AVSharedMemory **outMemory);
    AVSharedMemory *AllocMemory(int32_t size);
    void ReleaseMemory(AVSharedMemory *memory);
    bool CheckSize(int32_t size);

    InitializeOption option_ {};
    std::list<AVSharedMemory *> idleList_;
    std::list<AVSharedMemory *> busyList_;
    std::mutex mutex_;
    std::condition_variable cond_;
    bool inited_ = false;
    std::string name_;
};
}
}

#endif