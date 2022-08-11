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

#ifndef AVMEMORY_NATIVE_MOCK_H
#define AVMEMORY_NATIVE_MOCK_H

#include "avcodec_mock.h"
#include "avsharedmemory.h"

namespace OHOS {
namespace Media {
class AVMemoryNativeMock : public AVMemoryMock {
public:
    explicit AVMemoryNativeMock(std::shared_ptr<AVSharedMemory> mem) : memory_(mem) {}
    uint8_t *GetAddr() const override;
    int32_t GetSize() const override;
    uint32_t GetFlags() const override;

private:
    std::shared_ptr<AVSharedMemory> memory_ = nullptr;
};
} // Media
} // OHOS
#endif // AVMEMORY_NATIVE_MOCK_H