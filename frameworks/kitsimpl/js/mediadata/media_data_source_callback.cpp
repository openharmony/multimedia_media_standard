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

#include "media_data_source_callback.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaDataSourceImpl"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<MediaDataSourceCallback> MediaDataSourceCallback::Create(napi_env env, napi_value dataSrcNapi)
{
    CHECK_AND_RETURN_RET_LOG(env != nullptr && dataSrcNapi != nullptr, nullptr, "env or src is nullptr");
    MediaDataSourceNapi *data = nullptr;
    napi_status status = napi_unwrap(env, dataSrcNapi, reinterpret_cast<void **>(&data));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && data != nullptr, nullptr, "unwarp failed");
    CHECK_AND_RETURN_RET_LOG(data->CallbackCheckAndSetNoChange() == MSERR_OK, nullptr, "check callback failed");
    uint32_t refCount = 1;
    napi_ref ref = nullptr;
    status = napi_create_reference(env, dataSrcNapi, refCount, &(ref));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && ref != nullptr, nullptr, "create ref failed");
    std::shared_ptr<MediaDataSourceCallback> mediaDataSource =
        std::make_shared<MediaDataSourceCallback>(env, ref, *data);
    if (mediaDataSource == nullptr) {
        status = napi_reference_unref(env, ref, &refCount);
        CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "unref failed");
    }
    return mediaDataSource;
}

MediaDataSourceCallback::MediaDataSourceCallback(napi_env env, napi_ref ref, MediaDataSourceNapi &src)
    : env_(env),
      napiSrcRef_(ref),
      dataSrc_(src)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaDataSourceCallback::~MediaDataSourceCallback()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
    if (napiSrcRef_ != nullptr) {
        uint32_t refCount = 0;
        napi_reference_unref(env_, napiSrcRef_, &refCount);
        napiSrcRef_ = nullptr;
    }
    env_ = nullptr;
}

void MediaDataSourceCallback::Release() const
{
    dataSrc_.Release();
}

napi_value MediaDataSourceCallback::GetDataSrc() const
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env_, &undefinedResult);
    napi_value jsResult = nullptr;
    napi_status status = napi_get_reference_value(env_, napiSrcRef_, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsResult != nullptr, undefinedResult, "get reference value fail");
    return jsResult;
}

std::shared_ptr<AVSharedMemory> MediaDataSourceCallback::GetMem()
{
    return dataSrc_.GetMem();
}
int32_t MediaDataSourceCallback::ReadAt(int64_t pos, uint32_t length)
{
    return dataSrc_.ReadAt(pos, length);
}
int32_t MediaDataSourceCallback::ReadAt(uint32_t length)
{
    return dataSrc_.ReadAt(length);
}
int32_t MediaDataSourceCallback::GetSize(int64_t &size)
{
    return dataSrc_.GetSize(size);
}
} // Media
} // OHOS
