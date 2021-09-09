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

#ifndef AVSHAREDMEMORYBASE_H
#define AVSHAREDMEMORYBASE_H

#include <string>
#include "nocopyable.h"
#include "avsharedmemory.h"

namespace OHOS {
namespace Media {
class __attribute__((visibility("default"))) AVSharedMemoryBase : public AVSharedMemory {
public:
    // only used for local process
    AVSharedMemoryBase(int32_t size, uint32_t flags, const std::string &name);
    // only used for remote process
    AVSharedMemoryBase(int32_t fd, int32_t size, uint32_t flags, const std::string &name);
    ~AVSharedMemoryBase();

    int32_t Init();
    int32_t GetFd() const;
    std::string GetName() const
    {
        return name_;
    }
    uint8_t *GetBase() override;
    int32_t GetSize() override;
    uint32_t GetFlags() override;

    DISALLOW_COPY_AND_MOVE(AVSharedMemoryBase);

private:
    int32_t MapMemory(bool isRemote);
    void Close() noexcept;

    uint8_t *base_;
    int32_t size_;
    uint32_t flags_;
    std::string name_;
    int32_t fd_;
};
}
}

#endif