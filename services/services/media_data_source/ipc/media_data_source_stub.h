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

#ifndef MEDIA_DATA_SOURCE_STUB_H
#define MEDIA_DATA_SOURCE_STUB_H

#include "i_standard_media_data_source.h"
#include "media_death_recipient.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class MediaDataSourceStub : public IRemoteStub<IStandardMediaDataSource>, public NoCopyable {
public:
    explicit MediaDataSourceStub(const std::shared_ptr<IMediaDataSource> &dataSrc);
    virtual ~MediaDataSourceStub();

    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
    int32_t ReadAt(int64_t pos, uint32_t length, const std::shared_ptr<AVSharedMemory> &mem) override;
    int32_t ReadAt(uint32_t length, const std::shared_ptr<AVSharedMemory> &mem) override;
    int32_t GetSize(int64_t &size) override;

private:
    std::shared_ptr<IMediaDataSource> dataSrc_ = nullptr;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_DATA_SOURCE_STUB_H