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
#include <uv.h>
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderCallbackNapi"};
}

namespace OHOS {
namespace Media {
struct RecordJsErrCallbackParam {
    napi_env env = nullptr;
    napi_ref callback = nullptr;
    std::string callbackName = "unknown";
    std::string errorTypeMsg = "unknown";
    std::string errorCodeMsg = "unknown";
};

static void ErrorCallbackToJS(uv_loop_s *loop, uv_work_t *jsWork)
{
    // async callback, jsWork and jsWork->data should be heap object.
    uv_queue_work(loop, jsWork, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        RecordJsErrCallbackParam *param = reinterpret_cast<RecordJsErrCallbackParam *>(work->data);
        std::string request = param->callbackName;
        MEDIA_LOGD("RecorderCallbackNapi jsCallback: %{public}s, uv_queue_work start", request.c_str());
        do {
            napi_value jsCallback = nullptr;
            status = napi_get_reference_value(param->env, param->callback, &jsCallback);
            CHECK_AND_BREAK_LOG(status == napi_ok && jsCallback != nullptr, "%{public}s get reference value fail",
                request.c_str());

            napi_value errorTypeVal = nullptr;
            status = napi_create_string_utf8(param->env, param->errorTypeMsg.c_str(), NAPI_AUTO_LENGTH, &errorTypeVal);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s fail to get error type value", request.c_str());

            napi_value errorCodeVal = nullptr;
            status = napi_create_string_utf8(param->env, param->errorCodeMsg.c_str(), NAPI_AUTO_LENGTH, &errorCodeVal);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s fail to get error code value", request.c_str());

            napi_value args[1] = { nullptr };
            status = napi_create_error(param->env, errorTypeVal, errorCodeVal, &args[0]);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s fail to create error callback", request.c_str());

            // Call back function
            const size_t argCount = 1;
            napi_value result = nullptr;
            status = napi_call_function(param->env, nullptr, jsCallback, argCount, args, &result);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s fail to napi call function", request.c_str());
            MEDIA_LOGD("send record js error callback success");
        } while (false);
        delete param;
        delete work;
    });
}

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
    std::string errTypeMsg = RecorderErrorTypeToString(static_cast<RecorderErrorType>(errorType));
    std::string errCodeMsg = MSErrorToString(static_cast<MediaServiceErrCode>(errorCode));
    MEDIA_LOGD("enter OnError() is called, type: %{public}s, error code: %{public}s",
        errTypeMsg.c_str(), errCodeMsg.c_str());
    CHECK_AND_RETURN_LOG(recorderNapi_.errorCallback_ != nullptr, "no error callback reference received from JS");

    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    CHECK_AND_RETURN_LOG(loop != nullptr, "napi_get_uv_event_loop loop is nullptr");

    RecordJsErrCallbackParam *cbParam = new(std::nothrow) RecordJsErrCallbackParam {
        .env = env_,
        .callback = recorderNapi_.errorCallback_,
        .callbackName = ERROR_CALLBACK_NAME,
        .errorTypeMsg = errTypeMsg,
        .errorCodeMsg = errCodeMsg,
    };
    CHECK_AND_RETURN_LOG(cbParam != nullptr, "no mem, new RecordJsErrCallbackParam failed why?");

    uv_work_t *jsWork = new(std::nothrow) uv_work_t {};
    if (jsWork == nullptr) {
        delete cbParam;
        MEDIA_LOGE("no mem, new uv_work_t failed");
        return;
    }
    jsWork->data = reinterpret_cast<void *>(&cbParam);
    ErrorCallbackToJS(loop, jsWork);
    MEDIA_LOGD("leave OnError() is called, type: %{public}s, error code: %{public}s",
        errTypeMsg.c_str(), errCodeMsg.c_str());
}

void RecorderCallbackNapi::OnInfo(int32_t type, int32_t extra)
{
    MEDIA_LOGD("OnInfo() is called, type: %{public}d, extra: %{public}d", type, extra);
}
}  // namespace Media
}  // namespace OHOS