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

#include "avsharedmemorybase.h"
#include <sys/mman.h>
#include <unistd.h>
#include "ashmem.h"
#include "media_errors.h"
#include "media_log.h"
#include "scope_guard.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVSharedMemoryBase"};
}

namespace OHOS {
namespace Media {
struct AVSharedMemoryBaseImpl : public AVSharedMemoryBase {
public:
    AVSharedMemoryBaseImpl(int32_t fd, int32_t size, uint32_t flags, const std::string &name)
        : AVSharedMemoryBase(fd, size, flags, name) {}
};

std::shared_ptr<AVSharedMemory> AVSharedMemoryBase::CreateFromLocal(
    int32_t size, uint32_t flags, const std::string &name)
{
    std::shared_ptr<AVSharedMemoryBase> memory = std::make_shared<AVSharedMemoryBase>(size, flags, name);
    int32_t ret = memory->Init();
    if (ret != MSERR_OK) {
        MEDIA_LOGE("Create avsharedmemory failed, ret = %{public}d", ret);
        return nullptr;
    }

    return memory;
}

std::shared_ptr<AVSharedMemory> AVSharedMemoryBase::CreateFromRemote(
    int32_t fd, int32_t size, uint32_t flags, const std::string &name)
{
    std::shared_ptr<AVSharedMemoryBase> memory = std::make_shared<AVSharedMemoryBaseImpl>(fd, size, flags, name);
    int32_t ret = memory->Init();
    if (ret != MSERR_OK) {
        MEDIA_LOGE("Create avsharedmemory failed, ret = %{public}d", ret);
        return nullptr;
    }

    return memory;
}

AVSharedMemoryBase::AVSharedMemoryBase(int32_t size, uint32_t flags, const std::string &name)
    : base_(nullptr), size_(size), flags_(flags), name_(name), fd_(-1)
{
    MEDIA_LOGD("enter ctor, instance: 0x%{public}06" PRIXPTR ", name = %{public}s",
               FAKE_POINTER(this), name_.c_str());
}

AVSharedMemoryBase::AVSharedMemoryBase(int32_t fd, int32_t size, uint32_t flags, const std::string &name)
    : base_(nullptr), size_(size), flags_(flags), name_(name), fd_(dup(fd))
{
    MEDIA_LOGD("enter ctor, instance: 0x%{public}06" PRIXPTR ", name = %{public}s",
               FAKE_POINTER(this), name_.c_str());
}

AVSharedMemoryBase::~AVSharedMemoryBase()
{
    MEDIA_LOGD("enter dtor, instance: 0x%{public}06" PRIXPTR ", name = %{public}s",
               FAKE_POINTER(this), name_.c_str());
    Close();
}

int32_t AVSharedMemoryBase::Init()
{
    ON_SCOPE_EXIT(0) {
        MEDIA_LOGE("create avsharedmemory failed, name = %{public}s, size = %{public}d, "
                   "flags = 0x%{public}x, fd = %{public}d",
                   name_.c_str(), size_, flags_, fd_);
        Close();
    };

    CHECK_AND_RETURN_RET(size_ > 0, MSERR_INVALID_VAL);

    bool isRemote = false;
    if (fd_ > 0) {
        int size = AshmemGetSize(fd_);
        CHECK_AND_RETURN_RET(size == size_, MSERR_INVALID_VAL);
        isRemote = true;
    } else {
        fd_ = AshmemCreate(name_.c_str(), static_cast<size_t>(size_));
        CHECK_AND_RETURN_RET(fd_ > 0, MSERR_INVALID_VAL);
    }

    int32_t ret = MapMemory(isRemote);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, MSERR_INVALID_VAL);

    CANCEL_SCOPE_EXIT_GUARD(0);
    return MSERR_OK;
}

int32_t AVSharedMemoryBase::MapMemory(bool isRemote)
{
    unsigned int prot = PROT_READ | PROT_WRITE;
    if (isRemote && (flags_ & FLAGS_READ_ONLY)) {
        prot &= ~PROT_WRITE;
    }

    int result = AshmemSetProt(fd_, static_cast<int>(prot));
    CHECK_AND_RETURN_RET(result >= 0, MSERR_INVALID_OPERATION);

    void *addr = ::mmap(nullptr, static_cast<size_t>(size_), static_cast<int>(prot), MAP_SHARED, fd_, 0);
    CHECK_AND_RETURN_RET(addr != MAP_FAILED, MSERR_INVALID_OPERATION);

    base_ = reinterpret_cast<uint8_t*>(addr);
    return MSERR_OK;
}

void AVSharedMemoryBase::Close() noexcept
{
    if (base_ != nullptr) {
        (void)::munmap(base_, static_cast<size_t>(size_));
        base_ = nullptr;
        size_ = 0;
        flags_ = 0;
    }

    if (fd_ > 0) {
        (void)::close(fd_);
        fd_ = -1;
    }
}
} // namespace Media
} // namespace OHOS
