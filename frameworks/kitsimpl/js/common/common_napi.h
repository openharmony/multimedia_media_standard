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
class MediaJsResult {
public:
    virtual ~MediaJsResult() = default;
    virtual napi_status GetJsResult(napi_env env, napi_value *result) = 0;
};

class MediaJsResultInt : public MediaJsResult {
public:
    explicit MediaJsResultInt(const int32_t &value)
        : value_(value)
    {
    }
    ~MediaJsResultInt() = default;
    napi_status GetJsResult(napi_env env, napi_value *result) override
    {
        return napi_create_int32(env, value_, result);
    }
private:
    int32_t value_;
};

class MediaJsResultString : public MediaJsResult {
public:
    explicit MediaJsResultString(const std::string &value)
        : value_(value)
    {
    }
    ~MediaJsResultString() = default;
    napi_status GetJsResult(napi_env env, napi_value *result) override
    {
        return napi_create_string_utf8(env, value_.c_str(), NAPI_AUTO_LENGTH, result);
    }

private:
    std::string value_;
};

class MediaJsResultInstance : public MediaJsResult {
public:
    explicit MediaJsResultInstance(const napi_ref &constructor)
        : constructor_(constructor)
    {
    }
    ~MediaJsResultInstance() = default;
    napi_status GetJsResult(napi_env env, napi_value *result) override
    {
        napi_value constructor = nullptr;
        napi_status ret = napi_get_reference_value(env, constructor_, &constructor);
        if (ret != napi_ok || constructor == nullptr) {
            return ret;
        }
        return napi_new_instance(env, constructor, 0, nullptr, result);
    }

private:
    napi_ref constructor_;
};

struct MediaAsyncContext {
    explicit MediaAsyncContext(napi_env env) : env(env) {}
    virtual ~MediaAsyncContext() = default;
    static void CompleteCallback(napi_env env, napi_status status, void *data);
    void SignError(int32_t code, std::string message);
    napi_env env;
    napi_async_work work = nullptr;
    napi_deferred deferred = nullptr;
    napi_ref callbackRef = nullptr;
    std::unique_ptr<MediaJsResult> JsResult;
    bool errFlag = false;
    int32_t errCode = 0;
    std::string errMessage = "";
};

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
    static napi_status CreateError(napi_env env, int32_t errCode, const std::string &errMsg, napi_value &errVal);
    static napi_ref CreateReference(napi_env env, napi_value arg);
    static napi_deferred CreatePromise(napi_env env, napi_ref ref, napi_value *result);
};
}
}
#endif
