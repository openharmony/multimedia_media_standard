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

#ifndef MEDIA_DATA_SOURCE_CALLBACK_H_
#define MEDIA_DATA_SOURCE_CALLBACK_H_

#include "media_data_source.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "media_data_source_napi.h"

namespace OHOS {
namespace Media {
class MediaDataSourceCallback : public IMediaDataSource, public NoCopyable {
public:
    static std::shared_ptr<MediaDataSourceCallback> Create(napi_env env, napi_value data);
    MediaDataSourceCallback(napi_env env, napi_ref ref, MediaDataSourceNapi &src);
    virtual ~MediaDataSourceCallback();

    int32_t ReadAt(int64_t pos, uint32_t length, const std::shared_ptr<AVSharedMemory> &mem) override;
    int32_t ReadAt(uint32_t length, const std::shared_ptr<AVSharedMemory> &mem) override;
    int32_t GetSize(int64_t &size) override;
    void Release() const;
    napi_value GetDataSrc() const;
private:
    napi_env env_ = nullptr;
    napi_ref napiSrcRef_ = nullptr;
    MediaDataSourceNapi &dataSrc_;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_DATA_SOURCE_CALLBACK_H_
