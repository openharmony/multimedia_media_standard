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
#include "jscallback.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "JsCallback"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<JsCallback> JsCallback::Create(napi_env env, napi_value callback, const std::string &callbackName)
{
    napi_ref ref = nullptr;
    const int32_t refCount = 1;
    napi_status status = napi_create_reference(env, callback, refCount, &ref);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "create ref failed");
    std::shared_ptr<JsCallback> jsCallback = std::make_shared<JsCallback>(env, ref, callbackName);
    CHECK_AND_RETURN_RET_LOG(jsCallback != nullptr, nullptr, "failed to new jsCallback");

    return jsCallback;
}

JsCallback::JsCallback(napi_env env, napi_ref ref, const std::string &callbackName)
    : env_(env),
      ref_(ref),
      name_(callbackName)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create name %{public}s", FAKE_POINTER(this), callbackName.c_str());
}

JsCallback::~JsCallback()
{
    if (ref_ != nullptr) {
        napi_delete_reference(env_, ref_);
        ref_ = nullptr;
    }
    env_ = nullptr;
}

napi_value JsCallback::GetCallback() const
{
    CHECK_AND_RETURN_RET_LOG(ref_ != nullptr, nullptr, "callback ref is nullptr");
    napi_value callback = nullptr;
    napi_status status = napi_get_reference_value(env_, ref_, &callback);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "get reference value fail");
    return callback;
}

std::string JsCallback::GetName() const
{
    return name_;
}
} // namespace Media
} // namespace OHOS
