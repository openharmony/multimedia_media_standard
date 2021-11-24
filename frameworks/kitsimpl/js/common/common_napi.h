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

#ifndef COMMON_NAPI_H
#define COMMON_NAPI_H

#include <string>
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace Media {
struct AutoRef {
    AutoRef(napi_env env, napi_ref cb)
        : env_(env), cb_(cb)
    {
    }
    ~AutoRef()
    {
        if (env_ != nullptr && cb_ != nullptr) {
            (void)napi_delete_reference(env_, cb_);
        }
    }
    napi_env env_;
    napi_ref cb_;
};

class CommonNapi {
public:
    CommonNapi() = delete;
    ~CommonNapi() = delete;
    static std::string GetStringArgument(napi_env env, napi_value value);
    static bool GetPropertyInt32(napi_env env, napi_value configObj, const std::string &type, int32_t &result);
    static napi_status FillErrorArgs(napi_env env, int32_t errCode, const napi_value &args);
};
}
}
#endif
