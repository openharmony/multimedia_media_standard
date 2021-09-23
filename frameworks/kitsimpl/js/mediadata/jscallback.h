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

#ifndef JS_CALLBACK_H
#define JS_CALLBACK_H

#include <fstream>
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class JsCallback {
public:
    static std::shared_ptr<JsCallback> Create(napi_env env, napi_value callback, const std::string &callbackName);
    JsCallback(napi_env env, napi_ref ref, const std::string &callbackName);
    ~JsCallback();
    DISALLOW_COPY_AND_MOVE(JsCallback);
    napi_value GetCallback() const;
    std::string GetName() const;

private:
    napi_env env_ = nullptr;
    napi_ref ref_ = nullptr;
    std::string name_;
};
} // namespace Media
} // namespace OHOS
#endif
