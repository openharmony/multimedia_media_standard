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

#ifndef MEDIA_DATA_SOURCE_TEST_NOSEEK_H
#define MEDIA_DATA_SOURCE_TEST_NOSEEK_H

#include <string>
#include "media_data_source_test.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class MediaDataSourceTestNoSeek : public MediaDataSourceTest, public NoCopyable {
public:
    static std::shared_ptr<MediaDataSourceTest> Create(const std::string &uri, int32_t size);
    MediaDataSourceTestNoSeek(const std::string &uri, int32_t size);
    ~MediaDataSourceTestNoSeek() override;

    int32_t ReadAt(int64_t pos, uint32_t length, const std::shared_ptr<AVSharedMemory> &mem) override;
    int32_t ReadAt(uint32_t length, const std::shared_ptr<AVSharedMemory> &mem) override;
    int32_t GetSize(int64_t &size) override;
    void Reset() override;

private:
    int32_t Init();
    std::string uri_;
    FILE *fd_ = nullptr;
    int64_t size_ = 0;
    int64_t pos_ = 0;
    int32_t fixedSize_ = 0;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_DATA_SOURCE_TEST_NOSEEK_H
