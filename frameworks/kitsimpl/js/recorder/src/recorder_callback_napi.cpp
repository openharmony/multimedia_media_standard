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

#include "recorder_callback_napi.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderCallbackNapi"};
}

namespace OHOS {
namespace Media {
RecorderCallbackNapi::RecorderCallbackNapi(napi_env env, AudioRecorderNapi &recorder)
    : env_(env),
      recorderNapi_(recorder)
{
}

RecorderCallbackNapi::~RecorderCallbackNapi()
{
}

void RecorderCallbackNapi::OnError(int32_t errorType, int32_t errorCode)
{
    MEDIA_LOGD("OnError() is called, type: %{public}d, error code: %{public}d", errorType, errorCode);
    if (recorderNapi_.errorCallback_ == nullptr) {
        MEDIA_LOGE("no error callback reference received from JS");
        return;
    }

    napi_value jsCallback = nullptr;
    napi_status status = napi_get_reference_value(env_, recorderNapi_.errorCallback_, &jsCallback);
    if ((status != napi_ok) || (jsCallback == nullptr)) {
        MEDIA_LOGE("get reference value fail");
        return;
    }

    std::string errorTypeStr = std::to_string(errorType);
    napi_value errorTypeVal = nullptr;
    if (napi_create_string_utf8(env_, errorTypeStr.c_str(), NAPI_AUTO_LENGTH, &errorTypeVal) != napi_ok) {
        MEDIA_LOGE("get error type value fail");
        return;
    }

    std::string errorCodeStr = std::to_string(errorCode);
    napi_value errorCodeVal = nullptr;
    if (napi_create_string_utf8(env_, errorCodeStr.c_str(), NAPI_AUTO_LENGTH, &errorCodeVal) != napi_ok) {
        MEDIA_LOGE("get error code value fail");
        return;
    }

    napi_value args[1] = {nullptr};
    status = napi_create_error(env_, errorCodeVal, errorTypeVal, &args[0]);
    if (status != napi_ok) {
        MEDIA_LOGE("create error callback fail");
        return;
    }

    size_t argCount = 1;
    napi_value result = nullptr;
    status = napi_call_function(env_, nullptr, jsCallback, argCount, args, &result);
    if (status != napi_ok) {
        MEDIA_LOGE("call error callback fail");
    }

    MEDIA_LOGD("send error callback success");
}

void RecorderCallbackNapi::OnInfo(int32_t type, int32_t extra)
{
    MEDIA_LOGD("OnInfo() is called, type: %{public}d, extra: %{public}d", type, extra);
}
}  // namespace Media
}  // namespace OHOS