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
RecorderCallbackNapi::RecorderCallbackNapi(napi_env env)
    : env_(env)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

RecorderCallbackNapi::~RecorderCallbackNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void RecorderCallbackNapi::SaveCallbackReference(const std::string &callbackName, napi_value args)
{
    std::lock_guard<std::mutex> lock(mutex_);

    napi_ref callback = nullptr;
    const int32_t refCount = 1;

    napi_status status = napi_create_reference(env_, args, refCount, &callback);
    CHECK_AND_RETURN_LOG(status == napi_ok && callback != nullptr, "creating reference for callback fail");

    std::shared_ptr<AutoRef> cb = std::make_shared<AutoRef>(env_, callback);
    if (callbackName == ERROR_CALLBACK_NAME) {
        errorCallback_ = cb;
    } else if (callbackName == PREPARE_CALLBACK_NAME) {
        prepareCallback_ = cb;
    } else if (callbackName == START_CALLBACK_NAME) {
        startCallback_ = cb;
    } else if (callbackName == PAUSE_CALLBACK_NAME) {
        pauseCallback_ = cb;
    } else if (callbackName == RESUME_CALLBACK_NAME) {
        resumeCallback_ = cb;
    } else if (callbackName == STOP_CALLBACK_NAME) {
        stopCallback_ = cb;
    } else if (callbackName == RESET_CALLBACK_NAME) {
        resetCallback_ = cb;
    } else if (callbackName == RELEASE_CALLBACK_NAME) {
        releaseCallback_ = cb;
    } else {
        MEDIA_LOGE("unknown callback: %{public}s", callbackName.c_str());
        return;
    }
}

void RecorderCallbackNapi::SendErrorCallback(napi_env env, MediaServiceExtErrCode errCode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(env != nullptr, "env is nullptr");
    CHECK_AND_RETURN_LOG(errorCallback_ != nullptr, "no error callback reference");

    napi_value jsCallback = nullptr;
    napi_status status = napi_get_reference_value(env, errorCallback_->cb_, &jsCallback);
    CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr, "get reference value fail");

    napi_value msgValStr = nullptr;
    status = napi_create_string_utf8(env, MSExtErrorToString(errCode).c_str(), NAPI_AUTO_LENGTH, &msgValStr);
    CHECK_AND_RETURN_LOG(status == napi_ok && msgValStr != nullptr, "create error message str fail");

    napi_value args[1] = { nullptr };
    status = napi_create_error(env, nullptr, msgValStr, &args[0]);
    CHECK_AND_RETURN_LOG(status == napi_ok && args[0] != nullptr, "create error callback fail");

    status = CommonNapi::FillErrorArgs(env, static_cast<int32_t>(errCode), args[0]);
    CHECK_AND_RETURN_LOG(status == napi_ok, "create error callback fail");

    const size_t argCount = 1;
    napi_value result = nullptr;
    status = napi_call_function(env, nullptr, jsCallback, argCount, args, &result);
    CHECK_AND_RETURN_LOG(status == napi_ok, "call error callback fail");
}

void RecorderCallbackNapi::SendCallback(napi_env env, const std::string &callbackName)
{
    std::shared_ptr<AutoRef> callbackRef = nullptr;
    if (callbackName == PREPARE_CALLBACK_NAME) {
        callbackRef = prepareCallback_;
    } else if (callbackName == START_CALLBACK_NAME) {
        callbackRef = startCallback_;
    } else if (callbackName == PAUSE_CALLBACK_NAME) {
        callbackRef = pauseCallback_;
    } else if (callbackName == RESUME_CALLBACK_NAME) {
        callbackRef = resumeCallback_;
    } else if (callbackName == STOP_CALLBACK_NAME) {
        callbackRef = stopCallback_;
    } else if (callbackName == RESET_CALLBACK_NAME) {
        callbackRef = resetCallback_;
    } else if (callbackName == RELEASE_CALLBACK_NAME) {
        callbackRef = releaseCallback_;
    } else {
        MEDIA_LOGE("unknown callback: %{public}s", callbackName.c_str());
        return;
    }

    CHECK_AND_RETURN_LOG(callbackRef != nullptr, "no callback reference");
    napi_value jsCallback = nullptr;
    napi_status status = napi_get_reference_value(env, callbackRef->cb_, &jsCallback);
    CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr, "get reference value fail");

    napi_value result = nullptr;
    status = napi_call_function(env, nullptr, jsCallback, 0, nullptr, &result);
    CHECK_AND_RETURN_LOG(status == napi_ok, "send callback fail");

    MEDIA_LOGD("send callback success");
}

void RecorderCallbackNapi::OnError(RecorderErrorType errorType, int32_t errCode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("OnError is called, name: %{public}d, error message: %{public}d", errorType, errCode);
    CHECK_AND_RETURN_LOG(errorCallback_ != nullptr, "errorCallback_ is nullptr");

    RecordJsCallback *cb = new(std::nothrow) RecordJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "cb is nullptr");
    cb->callback = errorCallback_;
    cb->callbackName = ERROR_CALLBACK_NAME;
    cb->errorMsg = MSErrorToString(static_cast<MediaServiceErrCode>(errorType));
    cb->errorCode = MSErrorToExtError(static_cast<MediaServiceErrCode>(errCode));
    return ErrorCallbackToJS(cb);
}

void RecorderCallbackNapi::OnInfo(int32_t type, int32_t extra)
{
    MEDIA_LOGD("OnInfo() is called, type: %{public}d, extra: %{public}d", type, extra);
}

void RecorderCallbackNapi::ErrorCallbackToJS(RecordJsCallback *jsCb)
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);

    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        MEDIA_LOGE("fail to new uv_work_t");
        delete jsCb;
        return;
    }
    work->data = reinterpret_cast<void *>(jsCb);

    // async callback, jsWork and jsWork->data should be heap object.
    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        RecordJsCallback *event = reinterpret_cast<RecordJsCallback *>(work->data);
        std::string request = event->callbackName;
        napi_env env = event->callback->env_;
        napi_ref callback = event->callback->cb_;
        MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work start", request.c_str());
        do {
            CHECK_AND_BREAK_LOG(status != UV_ECANCELED, "%{public}s canceled", request.c_str());
            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(env, callback, &jsCallback);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr, "%{public}s get reference value fail",
                request.c_str());

            napi_value msgValStr = nullptr;
            nstatus = napi_create_string_utf8(env, event->errorMsg.c_str(), NAPI_AUTO_LENGTH, &msgValStr);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && msgValStr != nullptr, "%{public}s fail to get error code value",
                request.c_str());

            napi_value args[1] = { nullptr };
            nstatus = napi_create_error(env, nullptr, msgValStr, &args[0]);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && args[0] != nullptr, "%{public}s fail to create error callback",
                request.c_str());

            nstatus = CommonNapi::FillErrorArgs(env, static_cast<int32_t>(event->errorCode), args[0]);
            CHECK_AND_RETURN_LOG(nstatus == napi_ok, "create error callback fail");

            // Call back function
            const size_t argCount = 1;
            napi_value result = nullptr;
            nstatus = napi_call_function(env, nullptr, jsCallback, argCount, args, &result);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "%{public}s fail to napi call function", request.c_str());
        } while (0);
        delete event;
        delete work;
    });
    if (ret != 0) {
        MEDIA_LOGE("fail to uv_queue_work task");
        delete jsCb;
        delete work;
    }
}
}  // namespace Media
}  // namespace OHOS
