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

#include "media_data_source_napi.h"
#include "media_log.h"
#include "media_errors.h"
#include "securec.h"
#include "common_napi.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaDataSourceNapi"};
    const std::string CLASS_NAME = "MediaDataSource";
    const std::string READ_AT_CALLBACK_NAME = "readAt";
}

namespace OHOS {
namespace Media {
napi_ref MediaDataSourceNapi::constructor_ = nullptr;

MediaDataSourceNapi::MediaDataSourceNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaDataSourceNapi::~MediaDataSourceNapi()
{
    readAt_ = nullptr;
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
    callbackWorks_ = nullptr;
}

napi_value MediaDataSourceNapi::Init(napi_env env, napi_value exports)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("MediaDataSourceNapi Init start");
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_GETTER_SETTER("size", GetSize, SetSize),
        DECLARE_NAPI_FUNCTION("on", On),
    };
    napi_property_descriptor static_prop[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createMediaDataSource", CreateMediaDataSource),
    };
    napi_value constructor = nullptr;
    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && constructor != nullptr, nullptr, "define class fail");

    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && constructor_ != nullptr, nullptr, "create reference fail");

    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "set named property fail");

    status = napi_define_properties(env, exports, sizeof(static_prop) / sizeof(static_prop[0]), static_prop);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "define properties fail");
    return exports;
}

napi_value MediaDataSourceNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    napi_value jsThis = nullptr;
    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "constructor fail");

    MediaDataSourceNapi *sourceNapi = new(std::nothrow) MediaDataSourceNapi();
    CHECK_AND_RETURN_RET_LOG(sourceNapi != nullptr, result, "no memory");

    sourceNapi->env_ = env;

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(sourceNapi),
        MediaDataSourceNapi::Destructor, nullptr, &(sourceNapi->wrapper_));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "native wrap fail");
    MEDIA_LOGD("Constructor success");
    return jsThis;
}

void MediaDataSourceNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    (void)env;
    (void)finalize;
    if (nativeObject != nullptr) {
        delete reinterpret_cast<MediaDataSourceNapi *>(nativeObject);
    }
    MEDIA_LOGD("Destructor success");
}

napi_value MediaDataSourceNapi::CreateMediaDataSource(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    napi_value constructor = nullptr;
    napi_status status = napi_get_reference_value(env, constructor_, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && constructor != nullptr, result, "get reference value fail");

    status = napi_new_instance(env, constructor, 0, nullptr, &result);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && result != nullptr, result, "new instance fail");

    MEDIA_LOGD("CreateMediaDataSource success");
    return result;
}

napi_value MediaDataSourceNapi::On(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 2;
    napi_value args[2] = { nullptr };
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr && args[0] != nullptr && args[1] != nullptr,
        undefinedResult, "invalid argument");

    MediaDataSourceNapi *data = nullptr;
    status = napi_unwrap(env, jsThis, (void **)&data);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && data != nullptr, undefinedResult, "set callback fail");
    CHECK_AND_RETURN_RET_LOG(data->noChange_ == false, undefinedResult, "no change");
    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string ||
        napi_typeof(env, args[1], &valueType1) != napi_ok || valueType1 != napi_function) {
        MEDIA_LOGE("invalid arguments type");
        return undefinedResult;
    }

    std::string callbackName = CommonNapi::GetStringArgument(env, args[0]);
    MEDIA_LOGD("callbackName: %{public}s", callbackName.c_str());
    data->SaveCallbackReference(env, callbackName, args[1]);
    return undefinedResult;
}

void MediaDataSourceNapi::SaveCallbackReference(napi_env env, const std::string &callbackName,
    napi_value callback)
{
    if (callbackName == READ_AT_CALLBACK_NAME) {
        readAt_ = JsCallback::Create(env, callback, callbackName);
        CHECK_AND_RETURN_LOG(readAt_ != nullptr, "creating reference for readAt_ fail")
    } else {
        MEDIA_LOGE("unknown callback: %{public}s", callbackName.c_str());
        return;
    }
}

int32_t MediaDataSourceNapi::CheckCallbackWorks()
{
    if (callbackWorks_ == nullptr) {
        callbackWorks_ = std::make_shared<CallbackWorks>(env_);
        CHECK_AND_RETURN_RET_LOG(callbackWorks_ != nullptr, MSERR_NO_MEMORY, "init callbackwork failed");
    }
    return MSERR_OK;
}

int32_t MediaDataSourceNapi::CallbackCheckAndSetNoChange()
{
    CHECK_AND_RETURN_RET_LOG(readAt_ != nullptr, MSERR_NO_MEMORY, "readAt is null");
    noChange_ = true;
    return MSERR_OK;
}

napi_value MediaDataSourceNapi::GetSize(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    napi_value jsThis = nullptr;

    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, undefinedResult, "get args error");

    MediaDataSourceNapi *data = nullptr;
    status = napi_unwrap(env, jsThis, (void **)&data);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && data != nullptr, undefinedResult, "napi_unwrap error");
    napi_value result = nullptr;
    status = napi_create_int64(env, data->size_, &result);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && result != nullptr, undefinedResult, "get napi value error");

    MEDIA_LOGD("GetSize success");
    return result;
}

napi_value MediaDataSourceNapi::SetSize(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };

    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr && args[0] != nullptr,
        undefinedResult, "get args error");

    MediaDataSourceNapi *data = nullptr;
    status = napi_unwrap(env, jsThis, (void **)&data);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && data != nullptr, undefinedResult, "napi_unwrap error");
    CHECK_AND_RETURN_RET_LOG(data->noChange_ == false, undefinedResult, "no change");
    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        MEDIA_LOGE("invalid arguments type");
        return undefinedResult;
    }

    status = napi_get_value_int64(env, args[0], &data->size_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "get napi value error");

    MEDIA_LOGD("SetSize success");
    return undefinedResult;
}

int32_t MediaDataSourceNapi::ReadAt(int64_t pos, uint32_t length, const std::shared_ptr<AVSharedMemory> &mem)
{
    CHECK_AND_RETURN_RET_LOG(env_ != nullptr, 0, "env is nullptr");
    CHECK_AND_RETURN_RET_LOG(readAt_ != nullptr, 0, "readAt_ is nullptr");

    // this ReadAt args count is 3
    std::shared_ptr<CallbackWarp> cb = CallbackWarp::Create(env_, 3, readAt_);
    CHECK_AND_RETURN_RET_LOG(cb != nullptr, 0, "create callback fail");
    CHECK_AND_RETURN_RET_LOG(cb->SetArg(pos) == MSERR_OK, 0, "set arg failed");
    CHECK_AND_RETURN_RET_LOG(cb->SetArg(length) == MSERR_OK, 0, "set arg failed");
    CHECK_AND_RETURN_RET_LOG(cb->SetArg(mem) == MSERR_OK, 0, "set arg failed");

    CHECK_AND_RETURN_RET_LOG(CheckCallbackWorks() == MSERR_OK, 0, "works in null");
    CHECK_AND_RETURN_RET_LOG(callbackWorks_->Push(cb) == MSERR_OK, 0, "push work fail");
    napi_value result = nullptr;
    cb->GetResult(result);
    CHECK_AND_RETURN_RET_LOG(result != nullptr, 0, "get result failed");
    int32_t size = 0;
    napi_status status = napi_get_value_int32(env_, result, &size);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, 0, "get value for ref failed");
    return size;
}

int32_t MediaDataSourceNapi::ReadAt(uint32_t length, const std::shared_ptr<AVSharedMemory> &mem)
{
    CHECK_AND_RETURN_RET_LOG(env_ != nullptr, 0, "env is nullptr");
    CHECK_AND_RETURN_RET_LOG(readAt_ != nullptr, 0, "readAt_ is nullptr");

    // this ReadAt args count is 2
    std::shared_ptr<CallbackWarp> cb = CallbackWarp::Create(env_, 2, readAt_);
    CHECK_AND_RETURN_RET_LOG(cb != nullptr, 0, "create callback fail");
    CHECK_AND_RETURN_RET_LOG(cb->SetArg(length) == MSERR_OK, 0, "set arg failed");
    CHECK_AND_RETURN_RET_LOG(cb->SetArg(mem) == MSERR_OK, 0, "set arg failed");

    CHECK_AND_RETURN_RET_LOG(CheckCallbackWorks() == MSERR_OK, 0, "works in null");
    CHECK_AND_RETURN_RET_LOG(callbackWorks_->Push(cb) == MSERR_OK, 0, "push work fail");
    napi_value result = nullptr;
    cb->GetResult(result);
    CHECK_AND_RETURN_RET_LOG(result != nullptr, 0, "get result failed");
    int32_t size = 0;
    napi_status status = napi_get_value_int32(env_, result, &size);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, 0, "get value for ref failed");
    return size;
}

int32_t MediaDataSourceNapi::GetSize(int64_t &size) const
{
    size = size_;
    return MSERR_OK;
}

void MediaDataSourceNapi::Release()
{
    CHECK_AND_RETURN_LOG(callbackWorks_ != nullptr, "callbackwork is null");
    callbackWorks_->CancelAll();
}
}  // namespace Media
}  // namespace OHOS
