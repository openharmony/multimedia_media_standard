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

#include "avsharedmemorylocal.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVSharedMemoryLocal"};
    constexpr uint32_t MAX_ALLOWED_SIZE = 30 * 1024 * 1024;
}

namespace OHOS {
namespace Media {
std::shared_ptr<AVSharedMemory> AVSharedMemory::Create(int32_t size, uint32_t flags, const std::string &name)
{
    std::shared_ptr<AVSharedMemoryLocal> memory = std::make_shared<AVSharedMemoryLocal>(size, flags, name);
    int32_t ret = memory->Init();
    if (ret != MSERR_OK) {
        MEDIA_LOGE("Create avsharedmemory failed, ret = %{public}d", ret);
        return nullptr;
    }

    return memory;
}

AVSharedMemoryLocal::AVSharedMemoryLocal(int32_t size, uint32_t flags, const std::string &name)
    : base_(nullptr), size_(size), flags_(flags), name_(name)
{
    MEDIA_LOGD("enter ctor, instance: 0x%{public}06" PRIXPTR ", name = %{public}s",
               FAKE_POINTER(this), name_.c_str());
}

AVSharedMemoryLocal::~AVSharedMemoryLocal()
{
    MEDIA_LOGD("enter dtor, instance: 0x%{public}06" PRIXPTR ", name = %{public}s",
               FAKE_POINTER(this), name_.c_str());
    if (base_ != nullptr) {
        delete [] base_;
        base_ = nullptr;
    }
}

int32_t AVSharedMemoryLocal::Init()
{
    if (size_ > MAX_ALLOWED_SIZE) {
        MEDIA_LOGE("create avsharedmemory failed, size %{public}d is too large, name = %{public}s",
                   size_, name_.c_str());
        return MSERR_INVALID_VAL;
    }
    base_ = new (std::nothrow) uint8_t[size_];
    if (base_ == nullptr) {
        MEDIA_LOGE("create avsharedmemory failed, new failed, size = %{public}d , name = %{public}s",
                   size_, name_.c_str());
        return MSERR_INVALID_OPERATION;
    }
    return MSERR_OK;
}

uint8_t *AVSharedMemoryLocal::GetBase()
{
    return base_;
}

int32_t AVSharedMemoryLocal::GetSize()
{
    return size_;
}

uint32_t AVSharedMemoryLocal::GetFlags()
{
    return flags_;
}
} // namespace Media
} // namespace OHOS