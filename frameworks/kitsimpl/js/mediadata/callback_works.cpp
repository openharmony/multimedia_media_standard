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
#include "callback_works.h"
#include "media_log.h"
#include "media_errors.h"
#include "avsharedmemory.h"
#include "native_engine/native_engine.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CallbackWorks"};
}

namespace OHOS {
namespace Media {
struct CallbackWorkData {
    std::shared_ptr<CallbackWarp> cWarp = nullptr;
    std::shared_ptr<CallbackWorks> cWorks = nullptr;
};

CallbackWorks::CallbackWorks(napi_env env)
    : env_(env)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

CallbackWorks::~CallbackWorks()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t CallbackWorks::Push(const std::shared_ptr<CallbackWarp> &callback)
{
    uv_work_t *work = InitWork(callback);
    CHECK_AND_RETURN_RET_LOG(work != nullptr, MSERR_NO_MEMORY, "new work failed");
    {
        std::unique_lock<std::mutex> lock(mutex_);
        (void)works_.emplace(work);
    }
    if (Run(work) != MSERR_OK) {
        MEDIA_LOGE("run work failed");
        (void)Remove(work);
        return MSERR_NO_MEMORY;
    }
    return MSERR_OK;
}

int32_t CallbackWorks::Remove(uv_work_t *work)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        auto iter = works_.find(work);
        if (iter == works_.end()) {
            MEDIA_LOGE("unknow error work is not in the works");
            return MSERR_NO_MEMORY;
        }
        (void)works_.erase(iter);
    }
    DeinitWork(work);
    return MSERR_OK;
}

void CallbackWorks::CancelAll()
{
    std::unique_lock<std::mutex> lock(mutex_);
    for (auto iter = works_.begin(); iter != works_.end(); ++iter) {
        if (*iter == nullptr) {
            continue;
        }
        int status = uv_cancel(reinterpret_cast<uv_req_t*>(*iter));
        if (status != 0) {
            MEDIA_LOGE("work cancel failed");
        }
        CallbackWorkData *data = reinterpret_cast<CallbackWorkData *>((*iter)->data);
        if (data == nullptr) {
            continue;
        }
        std::shared_ptr<CallbackWarp> callback = data->cWarp;
        callback->CancelResult();
    }
}

uv_work_t *CallbackWorks::InitWork(const std::shared_ptr<CallbackWarp> &callback)
{
    uv_work_t *work = new(std::nothrow) uv_work_t;
    CHECK_AND_RETURN_RET_LOG(work != nullptr, nullptr, "new work is failed");
    MEDIA_LOGD("0x%{public}06" PRIXPTR " work create", FAKE_POINTER(work));
    CallbackWorkData *data = new(std::nothrow) CallbackWorkData;
    if (data == nullptr) {
        MEDIA_LOGE("new CallbackWorkData failed");
        delete work;
        return nullptr;
    }
    data->cWarp = callback;
    data->cWorks = shared_from_this();
    work->data = reinterpret_cast<void *>(data);
    return work;
}

void CallbackWorks::DeinitWork(uv_work_t *work) const
{
    CHECK_AND_RETURN_LOG(work != nullptr, "deinit work is null");
    MEDIA_LOGD("0x%{public}06" PRIXPTR " work destroy", FAKE_POINTER(work));
    CallbackWorkData *data = reinterpret_cast<CallbackWorkData *>(work->data);
    delete data;
    delete work;
}

int32_t CallbackWorks::Run(uv_work_t *work)
{
    uv_loop_s *loop = nullptr;
    CHECK_AND_RETURN_RET_LOG(env_ != nullptr, MSERR_NO_MEMORY, "env is null");
    napi_get_uv_event_loop(env_, &loop);
    CHECK_AND_RETURN_RET_LOG(loop != nullptr, MSERR_NO_MEMORY, "work is null");
    CHECK_AND_RETURN_RET_LOG(work != nullptr, MSERR_NO_MEMORY, "work is null");

    uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        CallbackWorkData *callbackWorkData = reinterpret_cast<CallbackWorkData *>(work->data);
        CHECK_AND_RETURN_LOG(callbackWorkData != nullptr, "unknow error, work data is null");
        std::shared_ptr<CallbackWarp> callbackWarp = callbackWorkData->cWarp;
        std::shared_ptr<CallbackWorks> callbackWorks = callbackWorkData->cWorks;
        CHECK_AND_RETURN_LOG(callbackWarp != nullptr && callbackWorks != nullptr, "unknow error, null mem");
        do {
            // Js Thread
            CHECK_AND_BREAK_LOG(status != UV_ECANCELED, "work canceled");
            std::string request = callbackWarp->GetName();
            MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work start", request.c_str());
            napi_value result = nullptr;
            napi_value jsCallback = callbackWarp->GetCallback();
            CHECK_AND_BREAK_LOG(jsCallback != nullptr, "%{public}s get callback fail", request.c_str());
            napi_status nstatus = napi_call_function(callbackWarp->GetEnv(), nullptr, jsCallback,
                callbackWarp->GetArgsCount(), callbackWarp->GetArgs(), &result);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "%{public}s fail to napi call function", request.c_str());
            callbackWarp->SetResult(result);
        } while (0);
        CHECK_AND_RETURN_LOG(callbackWorks->Remove(work) == MSERR_OK, "unknow error, work not in works");
    });
    reinterpret_cast<NativeEngine*>(env_)->Loop(LOOP_DEFAULT);
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
