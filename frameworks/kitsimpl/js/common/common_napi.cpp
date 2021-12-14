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

#include "common_napi.h"
#include <climits>
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CommonNapi"};
}

namespace OHOS {
namespace Media {
std::string CommonNapi::GetStringArgument(napi_env env, napi_value value)
{
    std::string strValue = "";
    size_t bufLength = 0;
    napi_status status = napi_get_value_string_utf8(env, value, nullptr, 0, &bufLength);
    if (status == napi_ok && bufLength > 0 && bufLength < PATH_MAX) {
        char *buffer = (char *)malloc((bufLength + 1) * sizeof(char));
        CHECK_AND_RETURN_RET_LOG(buffer != nullptr, strValue, "no memory");
        status = napi_get_value_string_utf8(env, value, buffer, bufLength + 1, &bufLength);
        if (status == napi_ok) {
            MEDIA_LOGD("argument = %{public}s", buffer);
            strValue = buffer;
        }
        free(buffer);
        buffer = nullptr;
    }
    return strValue;
}

bool CommonNapi::GetPropertyInt32(napi_env env, napi_value configObj, const std::string &type, int32_t &result)
{
    napi_value item = nullptr;
    bool exist = false;
    napi_status status = napi_has_named_property(env, configObj, type.c_str(), &exist);
    if (status != napi_ok || !exist) {
        MEDIA_LOGE("can not find %{public}s property", type.c_str());
        return false;
    }

    if (napi_get_named_property(env, configObj, type.c_str(), &item) != napi_ok) {
        MEDIA_LOGE("get %{public}s property fail", type.c_str());
        return false;
    }

    if (napi_get_value_int32(env, item, &result) != napi_ok) {
        MEDIA_LOGE("get %{public}s property value fail", type.c_str());
        return false;
    }
    return true;
}

napi_status CommonNapi::FillErrorArgs(napi_env env, int32_t errCode, const napi_value &args)
{
    napi_value codeStr = nullptr;
    napi_status status = napi_create_string_utf8(env, "code", NAPI_AUTO_LENGTH, &codeStr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && codeStr != nullptr, napi_invalid_arg, "create code str fail");

    napi_value errCodeVal = nullptr;
    int32_t errCodeInt = errCode;
    status = napi_create_int32(env, errCodeInt - MS_ERR_OFFSET, &errCodeVal);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && errCodeVal != nullptr, napi_invalid_arg,
        "create error code number val fail");

    status = napi_set_property(env, args, codeStr, errCodeVal);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, napi_invalid_arg, "set error code property fail");

    napi_value nameStr = nullptr;
    status = napi_create_string_utf8(env, "name", NAPI_AUTO_LENGTH, &nameStr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && nameStr != nullptr, napi_invalid_arg, "create name str fail");

    napi_value errNameVal = nullptr;
    status = napi_create_string_utf8(env, "BusinessError", NAPI_AUTO_LENGTH, &errNameVal);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && errNameVal != nullptr, napi_invalid_arg,
        "create BusinessError str fail");

    status = napi_set_property(env, args, nameStr, errNameVal);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, napi_invalid_arg, "set error name property fail");
    return napi_ok;
}

napi_status CommonNapi::CreateError(napi_env env, int32_t errCode, const std::string &errMsg, napi_value &errVal)
{
    napi_get_undefined(env, &errVal);

    napi_value msgValStr = nullptr;
    napi_status nstatus = napi_create_string_utf8(env, errMsg.c_str(), NAPI_AUTO_LENGTH, &msgValStr);
    if (nstatus != napi_ok || msgValStr == nullptr) {
        MEDIA_LOGE("create error message str fail");
        return napi_invalid_arg;
    }

    nstatus = napi_create_error(env, nullptr, msgValStr, &errVal);
    if (nstatus != napi_ok || errVal == nullptr) {
        MEDIA_LOGE("create error fail");
        return napi_invalid_arg;
    }

    napi_value codeStr = nullptr;
    nstatus = napi_create_string_utf8(env, "code", NAPI_AUTO_LENGTH, &codeStr);
    if (nstatus != napi_ok || codeStr == nullptr) {
        MEDIA_LOGE("create code str fail");
        return napi_invalid_arg;
    }

    napi_value errCodeVal = nullptr;
    nstatus = napi_create_int32(env, errCode - MS_ERR_OFFSET, &errCodeVal);
    if (nstatus != napi_ok || errCodeVal == nullptr) {
        MEDIA_LOGE("create error code number val fail");
        return napi_invalid_arg;
    }

    nstatus = napi_set_property(env, errVal, codeStr, errCodeVal);
    if (nstatus != napi_ok) {
        MEDIA_LOGE("set error code property fail");
        return napi_invalid_arg;
    }

    napi_value nameStr = nullptr;
    nstatus = napi_create_string_utf8(env, "name", NAPI_AUTO_LENGTH, &nameStr);
    if (nstatus != napi_ok || nameStr == nullptr) {
        MEDIA_LOGE("create name str fail");
        return napi_invalid_arg;
    }

    napi_value errNameVal = nullptr;
    nstatus = napi_create_string_utf8(env, "BusinessError", NAPI_AUTO_LENGTH, &errNameVal);
    if (nstatus != napi_ok || errNameVal == nullptr) {
        MEDIA_LOGE("create BusinessError str fail");
        return napi_invalid_arg;
    }

    nstatus = napi_set_property(env, errVal, nameStr, errNameVal);
    if (nstatus != napi_ok) {
        MEDIA_LOGE("set error name property fail");
        return napi_invalid_arg;
    }

    return napi_ok;
}

void MediaAsyncContext::SignError(int32_t code, std::string message)
{
    errMessage = message;
    errCode = code;
    errFlag = true;
    MEDIA_LOGE("SignError: %{public}s", message.c_str());
}
}
}
