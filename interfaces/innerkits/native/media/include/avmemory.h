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

#ifndef AVMEMORY_H
#define AVMEMORY_H

#include <cstdint>
#include <cstring>
#include "nocopyable.h"

namespace OHOS {
namespace Media {
/**
 * @brief Provides a wrap for raw byte buffer.
 */
class __attribute__((visibility("default"))) AVMemory {
public:
    /**
     * @brief Construct a new AVMemory object with specified capacity, the raw buffer will be allocated.
     */
    explicit AVMemory(size_t capacity);

    /**
     * @brief Construct a new AVMemory object with specified raw buffer address and capacity.
     */
    AVMemory(uint8_t *base, size_t capacity);

    ~AVMemory();

    uint8_t *Base() const
    {
        return base_;
    }

    uint8_t *Data() const
    {
        return base_ + offset_;
    }

    size_t Capacity() const
    {
        return capacity_;
    }

    size_t Size() const
    {
        return size_;
    }

    size_t Offset() const
    {
        return offset_;
    }

    void SetRange(size_t offset, size_t size)
    {
        offset_ = offset;
        size_ = size;
    }

    DISALLOW_COPY_AND_MOVE(AVMemory);

private:
    uint8_t *base_ = nullptr;
    size_t offset_ = 0;
    size_t size_ = 0;
    size_t capacity_ = 0;
    bool ownership = false;
};
} // namespace Media
} // namespace OHOS
#endif // AVMEMORY_H