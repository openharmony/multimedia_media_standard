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

void RecorderCallbackNapi::SaveCallbackReference(const std::string &name, std::weak_ptr<AutoRef> ref)
{
    std::lock_guard<std::mutex> lock(mutex_);
    refMap_[name] = ref;
}

void RecorderCallbackNapi::SendErrorCallback(MediaServiceExtErrCode errCode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (refMap_.find(ERROR_CALLBACK_NAME) == refMap_.end()) {
        MEDIA_LOGW("can not find error callback!");
        return;
    }

    RecordJsCallback *cb = new(std::nothrow) RecordJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "cb is nullptr");
    cb->autoRef = refMap_.at(ERROR_CALLBACK_NAME);
    cb->callbackName = ERROR_CALLBACK_NAME;
    cb->errorMsg = MSExtErrorToString(errCode);
    cb->errorCode = errCode;
    return OnJsErrorCallBack(cb);
}

void RecorderCallbackNapi::SendStateCallback(const std::string &callbackName)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (refMap_.find(callbackName) == refMap_.end()) {
        MEDIA_LOGW("can not find %{public}s callback!", callbackName.c_str());
        return;
    }

    RecordJsCallback *cb = new(std::nothrow) RecordJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "cb is nullptr");
    cb->autoRef = refMap_.at(callbackName);
    cb->callbackName = callbackName;
    return OnJsStateCallBack(cb);
}

void RecorderCallbackNapi::OnError(RecorderErrorType errorType, int32_t errCode)
{
    MEDIA_LOGD("OnError is called, name: %{public}d, error message: %{public}d", errorType, errCode);
    MediaServiceExtErrCode err = MSErrorToExtError(static_cast<MediaServiceErrCode>(errCode));
    return SendErrorCallback(err);
}

void RecorderCallbackNapi::OnInfo(int32_t type, int32_t extra)
{
    MEDIA_LOGD("OnInfo() is called, type: %{public}d, extra: %{public}d", type, extra);
}

void RecorderCallbackNapi::OnJsStateCallBack(RecordJsCallback *jsCb) const
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (loop == nullptr) {
        MEDIA_LOGE("fail to get uv event loop");
        delete jsCb;
        return;
    }

    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        MEDIA_LOGE("fail to new uv_work_t");
        delete jsCb;
        return;
    }

    work->data = reinterpret_cast<void *>(jsCb);
    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        CHECK_AND_RETURN_LOG(work != nullptr, "work is nullptr");
        RecordJsCallback *event = reinterpret_cast<RecordJsCallback *>(work->data);
        std::string request = event->callbackName;
        MEDIA_LOGD("OnJsStateCallBack %{public}s, uv_queue_work start", request.c_str());
        do {
            CHECK_AND_BREAK_LOG(status != UV_ECANCELED, "%{public}s canceled", request.c_str());
            std::shared_ptr<AutoRef> ref = event->autoRef.lock();
            CHECK_AND_BREAK_LOG(ref != nullptr, "%{public}s AutoRef is nullptr", request.c_str());

            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr, "%{public}s get reference value fail",
                request.c_str());

            napi_value result = nullptr;
            nstatus = napi_call_function(ref->env_, nullptr, jsCallback, 0, nullptr, &result);
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

void RecorderCallbackNapi::OnJsErrorCallBack(RecordJsCallback *jsCb) const
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);

    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        MEDIA_LOGE("No memory");
        delete jsCb;
        return;
    }
    work->data = reinterpret_cast<void *>(jsCb);

    // async callback, jsWork and jsWork->data should be heap object.
    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        CHECK_AND_RETURN_LOG(work != nullptr, "work is nullptr");
        RecordJsCallback *event = reinterpret_cast<RecordJsCallback *>(work->data);
        std::string request = event->callbackName;
        MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work start", request.c_str());
        do {
            CHECK_AND_BREAK_LOG(status != UV_ECANCELED, "%{public}s canceled", request.c_str());
            std::shared_ptr<AutoRef> ref = event->autoRef.lock();
            CHECK_AND_BREAK_LOG(ref != nullptr, "%{public}s AutoRef is nullptr", request.c_str());

            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr, "%{public}s get reference value fail",
                request.c_str());

            napi_value msgValStr = nullptr;
            nstatus = napi_create_string_utf8(ref->env_, event->errorMsg.c_str(), NAPI_AUTO_LENGTH, &msgValStr);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && msgValStr != nullptr, "%{public}s fail to get error code value",
                request.c_str());

            napi_value args[1] = { nullptr };
            nstatus = napi_create_error(ref->env_, nullptr, msgValStr, &args[0]);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && args[0] != nullptr, "%{public}s fail to create error callback",
                request.c_str());

            nstatus = CommonNapi::FillErrorArgs(ref->env_, static_cast<int32_t>(event->errorCode), args[0]);
            CHECK_AND_RETURN_LOG(nstatus == napi_ok, "create error callback fail");

            // Call back function
            napi_value result = nullptr;
            nstatus = napi_call_function(ref->env_, nullptr, jsCallback, 1, args, &result);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "%{public}s fail to napi call function", request.c_str());
        } while (0);
        delete event;
        delete work;
    });
    if (ret != 0) {
        MEDIA_LOGE("Failed to execute libuv work queue");
        delete jsCb;
        delete work;
    }
}
} // namespace Media
} // namespace OHOS
