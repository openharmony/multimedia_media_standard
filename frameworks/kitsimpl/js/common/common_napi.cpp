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
#include "avcodec_list.h"
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

bool CommonNapi::GetPropertyDouble(napi_env env, napi_value configObj, const std::string &type, double &result)
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

    if (napi_get_value_double(env, item, &result) != napi_ok) {
        MEDIA_LOGE("get %{public}s property value fail", type.c_str());
        return false;
    }
    return true;
}

std::string CommonNapi::GetPropertyString(napi_env env, napi_value configObj, const std::string &type)
{
    std::string invalid = "";
    bool exist = false;
    napi_status status = napi_has_named_property(env, configObj, type.c_str(), &exist);
    if (status != napi_ok || !exist) {
        MEDIA_LOGE("can not find %{public}s property", type.c_str());
        return invalid;
    }

    napi_value item = nullptr;
    if (napi_get_named_property(env, configObj, type.c_str(), &item) != napi_ok) {
        MEDIA_LOGE("get %{public}s property fail", type.c_str());
        return invalid;
    }

    return GetStringArgument(env, item);
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

napi_ref CommonNapi::CreateReference(napi_env env, napi_value arg)
{
    napi_ref ref = nullptr;
    napi_valuetype valueType = napi_undefined;
    if (arg != nullptr && napi_typeof(env, arg, &valueType) == napi_ok && valueType == napi_function) {
        const size_t refCount = 1;
        MEDIA_LOGD("napi_create_reference");
        napi_create_reference(env, arg, refCount, &ref);
    }
    return ref;
}

napi_deferred CommonNapi::CreatePromise(napi_env env, napi_ref ref, napi_value &result)
{
    napi_deferred deferred = nullptr;
    if (ref == nullptr) {
        MEDIA_LOGD("napi_create_promise");
        napi_create_promise(env, &deferred, &result);
    }
    return deferred;
}

bool CommonNapi::AddRangeProperty(napi_env env, napi_value obj, const std::string &name, int32_t min, int32_t max)
{
    CHECK_AND_RETURN_RET(obj != nullptr, false);

    napi_value range = nullptr;
    napi_status status = napi_create_object(env, &range);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    CHECK_AND_RETURN_RET(SetPropertyInt32(env, range, "min", min) == true, false);
    CHECK_AND_RETURN_RET(SetPropertyInt32(env, range, "max", max) == true, false);

    napi_value nameStr = nullptr;
    status = napi_create_string_utf8(env, name.c_str(), NAPI_AUTO_LENGTH, &nameStr);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    status = napi_set_property(env, obj, nameStr, range);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    return true;
}

bool CommonNapi::AddArrayProperty(napi_env env, napi_value obj, const std::string &name,
    const std::vector<int32_t> &vec)
{
    CHECK_AND_RETURN_RET(obj != nullptr, false);

    napi_value array = nullptr;
    napi_status status = napi_create_array_with_length(env, vec.size(), &array);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    for (uint32_t i = 0; i < vec.size(); i++) {
        napi_value number = nullptr;
        (void)napi_create_int32(env, vec.at(i), &number);
        (void)napi_set_element(env, array, i, number);
    }

    return true;
}

bool CommonNapi::SetPropertyInt32(napi_env env, napi_value &obj, const std::string &key, int32_t value)
{
    CHECK_AND_RETURN_RET(obj != nullptr, false);

    napi_value keyNapi = nullptr;
    napi_status status = napi_create_string_utf8(env, key.c_str(), NAPI_AUTO_LENGTH, &keyNapi);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    napi_value valueNapi = nullptr;
    status = napi_create_int32(env, value, &valueNapi);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    status = napi_set_property(env, obj, keyNapi, valueNapi);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, false, "faile to set property");

    return true;
}

bool CommonNapi::SetPropertyString(napi_env env, napi_value &obj, const std::string &key, const std::string &value)
{
    CHECK_AND_RETURN_RET(obj != nullptr, false);

    napi_value keyNapi = nullptr;
    napi_status status = napi_create_string_utf8(env, key.c_str(), NAPI_AUTO_LENGTH, &keyNapi);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    napi_value valueNapi = nullptr;
    status = napi_create_string_utf8(env, value.c_str(), NAPI_AUTO_LENGTH, &valueNapi);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    status = napi_set_property(env, obj, keyNapi, valueNapi);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, false, "faile to set property");

    return true;
}

napi_value CommonNapi::CreateFormatBuffer(napi_env env, Format &format)
{
    napi_value buffer = nullptr;
    napi_status status = napi_create_object(env, &buffer);
    CHECK_AND_RETURN_RET(status == napi_ok, nullptr);

    for (auto &iter : FORMAT_DATA) {
        if (iter.second == FORMAT_TYPE_INT32) {
            int32_t value = 0;
            bool ret = format.GetIntValue(iter.first, value);
            if (ret == true) {
                CHECK_AND_RETURN_RET(SetPropertyInt32(env, buffer, iter.first, value) == true, nullptr);
            }
        } else if (iter.second == FORMAT_TYPE_STRING) {
            std::string value;
            bool ret = format.GetStringValue(iter.first, value);
            if (ret == true) {
                CHECK_AND_RETURN_RET(SetPropertyString(env, buffer, iter.first, value) == true, nullptr);
            }
        } else if (iter.second == FORMAT_TYPE_ADDR) {
            /* no processing */
        } else {
            MEDIA_LOGE("format type: %{public}d", iter.second);
        }
    }
    return buffer;
}

napi_status MediaJsResultArray::GetJsResult(napi_env env, napi_value &result)
{
    // create Description
    napi_status status = napi_create_array(env, &result);
    if (status != napi_ok) {
        return napi_cancelled;
    }

    auto vecSize = value_.size();
    for (size_t index = 0; index < vecSize; ++index) {
        napi_value description = nullptr;
        description = CommonNapi::CreateFormatBuffer(env, value_[index]);
        if (description == nullptr || napi_set_element(env, result, index, description) != napi_ok) {
            return napi_cancelled;
        }
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

void MediaAsyncContext::CompleteCallback(napi_env env, napi_status status, void *data)
{
    MEDIA_LOGD("CompleteCallback In");
    auto asyncContext = reinterpret_cast<MediaAsyncContext *>(data);
    CHECK_AND_RETURN_LOG(asyncContext != nullptr, "asyncContext is nullptr!");

    if (status != napi_ok) {
        asyncContext->SignError(MSERR_EXT_UNKNOWN, "napi_create_async_work status != napi_ok");
    }

    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    napi_value args[2] = { nullptr };
    napi_get_undefined(env, &args[0]);
    napi_get_undefined(env, &args[1]);
    if (asyncContext->errFlag) {
        MEDIA_LOGD("async callback failed");
        (void)CommonNapi::CreateError(env, asyncContext->errCode, asyncContext->errMessage, result);
        args[0] = result;
    } else {
        MEDIA_LOGD("async callback success");
        if (asyncContext->JsResult != nullptr) {
            asyncContext->JsResult->GetJsResult(env, result);
        }
        args[1] = result;
    }

    if (asyncContext->deferred) {
        if (asyncContext->errFlag) {
            MEDIA_LOGD("napi_reject_deferred");
            napi_reject_deferred(env, asyncContext->deferred, args[0]);
        } else {
            MEDIA_LOGD("napi_resolve_deferred");
            napi_resolve_deferred(env, asyncContext->deferred, args[1]);
        }
    } else {
        MEDIA_LOGD("napi_call_function callback");
        napi_value callback = nullptr;
        napi_get_reference_value(env, asyncContext->callbackRef, &callback);
        CHECK_AND_RETURN_LOG(callback != nullptr, "callbackRef is nullptr!");
        const size_t argCount = 2;
        napi_value retVal;
        napi_get_undefined(env, &retVal);
        napi_call_function(env, nullptr, callback, argCount, args, &retVal);
        napi_delete_reference(env, asyncContext->callbackRef);
    }
    napi_delete_async_work(env, asyncContext->work);
    delete asyncContext;
    asyncContext = nullptr;
}

napi_status AddCodecInfo(napi_env env, napi_value &result, std::shared_ptr<AVCodecInfo> info)
{
    CHECK_AND_RETURN_RET(info != nullptr, napi_generic_failure);

    napi_value obj = nullptr;
    napi_status status = napi_create_object(env, &obj);
    CHECK_AND_RETURN_RET(status == napi_ok, status);

    (void)CommonNapi::SetPropertyString(env, obj, "name", info->GetName());
    (void)CommonNapi::SetPropertyInt32(env, obj, "type", static_cast<int32_t>(info->GetType()));
    (void)CommonNapi::SetPropertyString(env, obj, "mimeType", info->GetMimeType());
    (void)CommonNapi::SetPropertyInt32(env, obj, "isHardwareAccelerated",
        static_cast<int32_t>(info->IsHardwareAccelerated()));
    (void)CommonNapi::SetPropertyInt32(env, obj, "isSoftwareOnly", static_cast<int32_t>(info->IsSoftwareOnly()));
    (void)CommonNapi::SetPropertyInt32(env, obj, "isVendor", static_cast<int32_t>(info->IsVendor()));

    return napi_ok;
}

napi_status MediaCapsJsResultAudio::GetJsResult(napi_env env, napi_value &result)
{
    auto codecList = AVCodecListFactory::CreateAVCodecList();
    CHECK_AND_RETURN_RET(codecList != nullptr, napi_generic_failure);

    std::vector<std::shared_ptr<AudioCaps>> audioCaps;
    if (isDecoder_) {
        audioCaps = codecList->GetAudioDecoderCaps();
    } else {
        audioCaps = codecList->GetAudioEncoderCaps();
    }

    napi_status status = napi_create_array_with_length(env, audioCaps.size(), &result);
    CHECK_AND_RETURN_RET(status == napi_ok, status);

    int32_t index = 0;

    for (auto it = audioCaps.begin(); it != audioCaps.end(); it++) {
        CHECK_AND_CONTINUE((*it) != nullptr);

        napi_value obj = nullptr;
        status = napi_create_object(env, &obj);
        CHECK_AND_CONTINUE(status == napi_ok);

        Range range = (*it)->GetSupportedBitrate();
        (void)CommonNapi::AddRangeProperty(env, obj, "supportedBitrate", range.minVal, range.maxVal);

        range = (*it)->GetSupportedChannel();
        (void)CommonNapi::AddRangeProperty(env, obj, "supportedChannel", range.minVal, range.maxVal);

        range = (*it)->GetSupportedComplexity();
        (void)CommonNapi::AddRangeProperty(env, obj, "supportedComplexity", range.minVal, range.maxVal);

        std::vector<int32_t> vec = (*it)->GetSupportedFormats();
        (void)CommonNapi::AddArrayProperty(env, obj, "supportedFormats", vec);

        vec = (*it)->GetSupportedSampleRates();
        (void)CommonNapi::AddArrayProperty(env, obj, "supportedSampleRates", vec);

        vec = (*it)->GetSupportedProfiles();
        (void)CommonNapi::AddArrayProperty(env, obj, "supportedProfiles", vec);

        vec = (*it)->GetSupportedLevels();
        (void)CommonNapi::AddArrayProperty(env, obj, "supportedLevels", vec);

        auto codecInfo = (*it)->GetCodecInfo();
        if (codecInfo != nullptr) {
            (void)AddCodecInfo(env, obj, codecInfo);
        }

        (void)napi_set_element(env, result, index, obj);
        index++;
    }

    return napi_ok;
}
}
}
