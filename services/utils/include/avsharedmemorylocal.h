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

#ifndef AVSHAREDMEMORYLOCAL_H
#define AVSHAREDMEMORYLOCAL_H

#include <string>
#include "nocopyable.h"
#include "avsharedmemory.h"

namespace OHOS {
namespace Media {
class AVSharedMemoryLocal : public AVSharedMemory, public NoCopyable {
public:
    AVSharedMemoryLocal(int32_t size, uint32_t flags, const std::string &name);
    ~AVSharedMemoryLocal();

    int32_t Init();
    std::string GetName()
    {
        return name_;
    }
    uint8_t *GetBase() override;
    int32_t GetSize() override;
    uint32_t GetFlags() override;

private:
    uint8_t *base_;
    int32_t size_;
    uint32_t flags_;
    std::string name_;
};
} // namespace Media
} // namespace OHOS

#endif