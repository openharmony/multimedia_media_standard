/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "avmemory.h"

namespace OHOS {
namespace Media {
AVMemory::AVMemory(size_t capacity)
{
    base_ = new(std::nothrow) uint8_t[capacity];
    ownership_ = true;
}

AVMemory::AVMemory(uint8_t *base, size_t capacity)
{
    base_ = base;
    capacity_ = capacity;
}

AVMemory::~AVMemory()
{
    if (ownership_ && base_ != nullptr) {
        delete [] base_;
        base_ = nullptr;
    }
}
} // namespace Media
} // namespace OHOS
