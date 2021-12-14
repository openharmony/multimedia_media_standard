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

#include "media_description.h"
#include <climits>
#include "media_log.h"
#include "media_errors.h"
#include "common_napi.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaDescriptionNapi"};
}

namespace OHOS {
namespace Media {
napi_ref MediaDescriptionNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "MediaDescription";

MediaDescriptionNapi::MediaDescriptionNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaDescriptionNapi::~MediaDescriptionNapi()
{
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

napi_value MediaDescriptionNapi::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("contain", Contain),
        DECLARE_NAPI_FUNCTION("getBoolean", GetBoolean),
        DECLARE_NAPI_FUNCTION("getNumber", GetNumber),
        DECLARE_NAPI_FUNCTION("getString", GetString),
        DECLARE_NAPI_FUNCTION("getArrayBuffer", GetArrayBuffer),
        DECLARE_NAPI_FUNCTION("set", Set),
        DECLARE_NAPI_FUNCTION("delete", Delete),
    };

    napi_value constructor = nullptr;
    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define AudioPlayer class");

    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to create reference of constructor");

    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set constructor");
    MEDIA_LOGD("Init success");
    return exports;
}

napi_value MediaDescriptionNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    napi_value jsThis = nullptr;
    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    MediaDescriptionNapi *mdNapi = new(std::nothrow) MediaDescriptionNapi();
    CHECK_AND_RETURN_RET_LOG(mdNapi != nullptr, nullptr, "failed to new MediaDescriptionNapi");

    mdNapi->env_ = env;

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(mdNapi),
        MediaDescriptionNapi::Destructor, nullptr, &(mdNapi->wrapper_));
    if (status != napi_ok) {
        delete mdNapi;
        MEDIA_LOGE("Failed to wrap native instance");
        return undefinedResult;
    }

    MEDIA_LOGD("Constructor success");
    return jsThis;
}

void MediaDescriptionNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    (void)env;
    (void)finalize;
    if (nativeObject != nullptr) {
        delete reinterpret_cast<MediaDescriptionNapi *>(nativeObject);
    }
    MEDIA_LOGD("Destructor success");
}

napi_value MediaDescriptionNapi::CreateMediaDescription(napi_env env, Format &format)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    napi_value constructor = nullptr;

    napi_status status = napi_get_reference_value(env, constructor_, &constructor);
    if (status != napi_ok) {
        MEDIA_LOGE("Failed to get the representation of constructor object");
        return undefinedResult;
    }

    napi_value result = nullptr;
    status = napi_new_instance(env, constructor, 0, nullptr, &result);
    if (status != napi_ok || result == nullptr) {
        MEDIA_LOGE("Failed to instantiate JavaScript audio player instance");
        return undefinedResult;
    }

    MediaDescriptionNapi *mdNapi = nullptr;
    status = napi_unwrap(env, result, reinterpret_cast<void**>(&mdNapi));
    if (status != napi_ok || mdNapi == nullptr) {
        MEDIA_LOGE("Failed to retrieve instance");
        return undefinedResult;
    }

    mdNapi->format_ = &format;
    return result;
}

napi_value MediaDescriptionNapi::Contain(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    // read format_
    return undefinedResult;
}

napi_value MediaDescriptionNapi::GetBoolean(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    // read format_
    return undefinedResult;
}

napi_value MediaDescriptionNapi::GetNumber(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    // read format_
    return undefinedResult;
}

napi_value MediaDescriptionNapi::GetString(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    // read format_
    return undefinedResult;
}

napi_value MediaDescriptionNapi::GetArrayBuffer(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    // read format_
    return undefinedResult;
}

napi_value MediaDescriptionNapi::Set(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    // read format_
    return undefinedResult;
}

napi_value MediaDescriptionNapi::Delete(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    // read format_
    return undefinedResult;
}
}
}