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

#include "native_avmemory.h"
#include "native_avmagic.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "OH_AVMemory"};
}

using namespace OHOS::Media;

OH_AVMemory::OH_AVMemory(const std::shared_ptr<OHOS::Media::AVSharedMemory> &mem)
    : AVObjectMagic(AVMagic::MEDIA_MAGIC_SHARED_MEMORY), memory_(mem)
{
}

OH_AVMemory::~OH_AVMemory()
{
}

bool OH_AVMemory::IsEqualMemory(const std::shared_ptr<OHOS::Media::AVSharedMemory> &mem)
{
    return (mem == memory_) ? true : false;
}

uint8_t *OH_AVMemory_GetAddr(struct OH_AVMemory *mem)
{
    CHECK_AND_RETURN_RET_LOG(mem != nullptr, nullptr, "input mem is nullptr!");
    CHECK_AND_RETURN_RET_LOG(mem->magic_ == AVMagic::MEDIA_MAGIC_SHARED_MEMORY, nullptr, "magic error!");
    CHECK_AND_RETURN_RET_LOG(mem->memory_ != nullptr, nullptr, "memory is nullptr!");
    return mem->memory_->GetBase();
}

int32_t OH_AVMemory_GetSize(struct OH_AVMemory *mem)
{
    CHECK_AND_RETURN_RET_LOG(mem != nullptr, -1, "input mem is nullptr!");
    CHECK_AND_RETURN_RET_LOG(mem->magic_ == AVMagic::MEDIA_MAGIC_SHARED_MEMORY, -1, "magic error!");
    CHECK_AND_RETURN_RET_LOG(mem->memory_ != nullptr, -1, "memory is nullptr!");
    return mem->memory_->GetSize();
}
