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

#ifndef MEDIA_DATA_SOURCE_TEST_SEEKABLE_H
#define MEDIA_DATA_SOURCE_TEST_SEEKABLE_H

#include <string>
#include "media_data_source.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class MediaDataSourceTestSeekable : public IMediaDataSource {
public:
    static std::shared_ptr<IMediaDataSource> Create(const std::string &uri);
    explicit MediaDataSourceTestSeekable(const std::string &uri);
    virtual ~MediaDataSourceTestSeekable();
    DISALLOW_COPY_AND_MOVE(MediaDataSourceTestSeekable);
    std::shared_ptr<AVSharedMemory> GetMem() override;
    int32_t ReadAt(int64_t pos, uint32_t length) override;
    int32_t ReadAt(uint32_t length) override;
    int32_t GetSize(int64_t &size) override;

private:
    int32_t Init();
    std::string uri_;
    FILE *fd_ = nullptr;
    int64_t size_ = 0;
    int64_t pos_ = 0;
    std::shared_ptr<AVSharedMemory> mem_;
};
} // namespace Media
} // namespace OHOS
#endif
