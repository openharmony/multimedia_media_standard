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

#include "media_data_source_demo_noseek.h"
#include <iostream>
#include "media_errors.h"
#include "directory_ex.h"
#include "media_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaDataSourceDemoNoSeek"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<MediaDataSourceDemo> MediaDataSourceDemoNoSeek::Create(const std::string &uri, int32_t size)
{
    std::string realPath;
    if (!PathToRealPath(uri, realPath)) {
        std::cout << "Path is unaccessable: " << uri << std::endl;
        return nullptr;
    }
    std::shared_ptr<MediaDataSourceDemoNoSeek> dataSrc = std::make_shared<MediaDataSourceDemoNoSeek>(realPath, size);
    if (dataSrc == nullptr) {
        std::cout << "create source failed" << std::endl;
        return nullptr;
    }
    if (dataSrc->Init() != MSERR_OK) {
        std::cout << "init source failed" << std::endl;
        return nullptr;
    }
    return dataSrc;
}

MediaDataSourceDemoNoSeek::MediaDataSourceDemoNoSeek(const std::string &uri, int32_t size)
    : uri_(uri),
      fixedSize_(size)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaDataSourceDemoNoSeek::~MediaDataSourceDemoNoSeek()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    if (fd_ != nullptr) {
        (void)fclose(fd_);
        fd_ = nullptr;
    }
}

int32_t MediaDataSourceDemoNoSeek::Init()
{
    fd_ = fopen(uri_.c_str(), "rb+");
    if (fd_ == nullptr) {
        std::cout<<"open file failed"<<std::endl;
        return MSERR_INVALID_VAL;
    }
    return MSERR_OK;
}

void MediaDataSourceDemoNoSeek::Reset()
{
    std::cout<< "reset data source" << std::endl;
    pos_ = 0;
    (void)fseek(fd_, 0, SEEK_SET);
}

int32_t MediaDataSourceDemoNoSeek::ReadAt(int64_t pos, uint32_t length, const std::shared_ptr<AVSharedMemory> &mem)
{
    (void)pos;
    (void)length;
    (void)mem;
    return 0;
}

int32_t MediaDataSourceDemoNoSeek::ReadAt(uint32_t length, const std::shared_ptr<AVSharedMemory> &mem)
{
    CHECK_AND_RETURN_RET_LOG(mem != nullptr, MSERR_INVALID_VAL, "Mem is nullptr");
    size_t readRet = 0;
    if (fixedSize_ > 0) {
        length = static_cast<uint32_t>(fixedSize_);
    }
    CHECK_AND_RETURN_RET_LOG(mem->GetSize() > 0, SOURCE_ERROR_IO, "AVSHMEM length should large than 0");
    length = std::min(length, static_cast<uint32_t>(mem->GetSize()));
    int32_t realLen = static_cast<int32_t>(length);
    if (pos_ >= size_) {
        MEDIA_LOGI("Is eos");
        return SOURCE_ERROR_EOF;
    }
    if (mem->GetBase() == nullptr) {
        MEDIA_LOGI("Is null mem");
        return SOURCE_ERROR_IO;
    }
    readRet = fread(mem->GetBase(), static_cast<size_t>(length), 1, fd_);
    if (ferror(fd_)) {
        MEDIA_LOGI("Failed to call fread");
        return SOURCE_ERROR_IO;
    }
    if (readRet == 0) {
        realLen = static_cast<int32_t>(size_ - pos_);
    }
    MEDIA_LOGD("length %{public}u realLen %{public}d", length, realLen);
    pos_ += realLen;
    return realLen;
}

int32_t MediaDataSourceDemoNoSeek::GetSize(int64_t &size)
{
    (void)fseek(fd_, 0, SEEK_END);
    size_ = static_cast<int64_t>(ftell(fd_));
    (void)fseek(fd_, 0, SEEK_SET);
    size = -1;
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
