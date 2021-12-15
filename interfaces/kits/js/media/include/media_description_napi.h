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

#ifndef MEDIA_DESCRIPTION_NAPI_H_
#define MEDIA_DESCRIPTION_NAPI_H_

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "format.h"
#include "media_errors.h"

namespace OHOS {
namespace Media {
class MediaDescriptionNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);
    static napi_value CreateMediaDescription(napi_env env, Format &format);

private:
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static void Destructor(napi_env env, void *nativeObject, void *finalize);
    static napi_value Contain(napi_env env, napi_callback_info info);
    static napi_value GetBoolean(napi_env env, napi_callback_info info);
    static napi_value GetNumber(napi_env env, napi_callback_info info);
    static napi_value GetString(napi_env env, napi_callback_info info);
    static napi_value GetArrayBuffer(napi_env env, napi_callback_info info);
    static napi_value Set(napi_env env, napi_callback_info info);
    static napi_value Delete(napi_env env, napi_callback_info info);
    MediaDescriptionNapi();
    ~MediaDescriptionNapi();

    static napi_ref constructor_;
    napi_env env_ = nullptr;
    napi_ref wrapper_ = nullptr;
    Format *format_;
};
}
}
#endif