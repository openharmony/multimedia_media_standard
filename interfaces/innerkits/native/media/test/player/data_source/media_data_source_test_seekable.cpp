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

#include "media_data_source_test_seekable.h"
#include <iostream>
#include "media_errors.h"
#include "media_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaDataSourceTestSeekable"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<IMediaDataSource> MediaDataSourceTestSeekable::Create(const std::string &uri)
{
    std::shared_ptr<MediaDataSourceTestSeekable> dataSrc = std::make_shared<MediaDataSourceTestSeekable>(uri);
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

MediaDataSourceTestSeekable::MediaDataSourceTestSeekable(const std::string &uri)
    : uri_(uri)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaDataSourceTestSeekable::~MediaDataSourceTestSeekable()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    if (mem_ != nullptr) {
        mem_ = nullptr;
    }
    if (fd_ != nullptr) {
        (void)fclose(fd_);
        fd_ = nullptr;
    }
}

int32_t MediaDataSourceTestSeekable::Init()
{
    fd_ = fopen(uri_.c_str(), "rb+");
    if (fd_ == nullptr) {
        std::cout<<"open file failed"<<std::endl;
        return MSERR_INVALID_VAL;
    }
    return MSERR_OK;
}

std::shared_ptr<AVSharedMemory> MediaDataSourceTestSeekable::GetMem()
{
    CHECK_AND_RETURN_RET_LOG(mem_ != nullptr, nullptr, "dataSrc_ is nullptr");
    return mem_;
}

int32_t MediaDataSourceTestSeekable::ReadAt(int64_t pos, uint32_t length)
{
    MEDIA_LOGD("pos %{public}" PRId64 " pos_ %{public}" PRId64 "", pos, pos_);
    if (pos != pos_) {
        (void)fseek(fd_, pos, SEEK_SET);
        pos_ = pos;
    }
    size_t readRet = 0;
    int32_t realLen = static_cast<int32_t>(length);
    const std::string name = "mediaDataTest";
    if (pos_ >= size_) {
        return SOURCE_ERROR_EOF;
    }
    mem_ = AVSharedMemory::Create(static_cast<int32_t>(length), AVSharedMemory::Flags::FLAGS_READ_ONLY, name);
    readRet = fread(mem_->GetBase(), static_cast<size_t>(length), static_cast<size_t>(1), fd_);
    if (readRet == 0) {
        realLen = static_cast<int32_t>(size_ - pos_);
    }
    MEDIA_LOGD("length %{public}u realLen %{public}d", length, realLen);
    pos_ += realLen;
    return realLen;
}

int32_t MediaDataSourceTestSeekable::ReadAt(uint32_t length)
{
    (void)length;
    return 0;
}

int32_t MediaDataSourceTestSeekable::GetSize(int64_t &size)
{
    (void)fseek(fd_, 0, SEEK_END);
    size = static_cast<int64_t>(ftell(fd_));
    (void)fseek(fd_, 0, SEEK_SET);
    size_ = static_cast<uint32_t>(size);
    return MSERR_OK;
}
} // Media
} // OHOS
