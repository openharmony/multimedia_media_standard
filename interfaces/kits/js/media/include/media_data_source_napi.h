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

#ifndef MEDIA_DATA_SOURCE_NAPI_H_
#define MEDIA_DATA_SOURCE_NAPI_H_

#include "media_data_source.h"
#include "callback_works.h"

namespace OHOS {
namespace Media {
class MediaDataSourceNapi {
public:
    MediaDataSourceNapi();
    ~MediaDataSourceNapi();
    DISALLOW_COPY_AND_MOVE(MediaDataSourceNapi);
    static napi_value Init(napi_env env, napi_value exports);
    int32_t ReadAt(int64_t pos, uint32_t length, const std::shared_ptr<AVSharedMemory> &mem);
    int32_t ReadAt(uint32_t length, const std::shared_ptr<AVSharedMemory> &mem);
    int32_t GetSize(int64_t &size) const;
    int32_t CallbackCheckAndSetNoChange();
    void Release();

private:
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static void Destructor(napi_env env, void *nativeObject, void *finalize);
    static napi_value CreateMediaDataSource(napi_env env, napi_callback_info info);
    static napi_value On(napi_env env, napi_callback_info info);
    static napi_value SetSize(napi_env env, napi_callback_info info);
    static napi_value GetSize(napi_env env, napi_callback_info info);
    void SaveCallbackReference(napi_env env, const std::string &callbackName, napi_value callback);
    int32_t CheckCallbackWorks();
    static napi_ref constructor_;
    napi_env env_ = nullptr;
    napi_ref wrapper_ = nullptr;
    std::shared_ptr<CallbackWorks> callbackWorks_ = nullptr;
    std::shared_ptr<JsCallback> readAt_ = nullptr;
    int64_t size_ = -1;
    bool noChange_ = false;
};
} // namespace Media
} // namespace OHOS
#endif /* MEDIA_DATA_SOURCE_NAPI_H_ */