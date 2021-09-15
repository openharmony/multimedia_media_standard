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
#include "callback_warp.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CallbackWarp"};
    constexpr int32_t ARGS_COUNT_MAX = 10;
}

namespace OHOS {
namespace Media {
struct AvMemNapiWarp {
    explicit AvMemNapiWarp(const std::shared_ptr<AVSharedMemory> &mem) : mem_(mem)
    {
        MEDIA_LOGD("0x%{public}06" PRIXPTR " AvMemNapiWarp Instances create", FAKE_POINTER(this));
    };
    ~AvMemNapiWarp()
    {
        MEDIA_LOGD("0x%{public}06" PRIXPTR " AvMemNapiWarp Instances destroy", FAKE_POINTER(this));
    };
    std::shared_ptr<AVSharedMemory> mem_;
};

std::shared_ptr<CallbackWarp> CallbackWarp::Create(napi_env env, const size_t argsCount,
    std::shared_ptr<JsCallback> jsCb)
{
    CHECK_AND_RETURN_RET_LOG(env != nullptr, nullptr, "env is nullptr");
    CHECK_AND_RETURN_RET_LOG(argsCount >= 0 && argsCount < ARGS_COUNT_MAX, nullptr, "args count error");
    CHECK_AND_RETURN_RET_LOG(jsCb != nullptr, nullptr, "JsCallback is nullptr");
    return std::make_shared<CallbackWarp>(env, argsCount, jsCb);
}

CallbackWarp::CallbackWarp(napi_env env, const size_t argsCount, std::shared_ptr<JsCallback> jsCb)
    : env_(env),
      argsCount_(argsCount),
      jsCb_(jsCb)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

CallbackWarp::~CallbackWarp()
{
    if (argv_ != nullptr) {
        free(argv_);
    }
    if (result_ != nullptr) {
        (void)napi_delete_reference(env_, result_);
        result_ = nullptr;
        resultValue_ = nullptr;
    }
    jsCb_ = nullptr;
}

void CallbackWarp::CancelResult()
{
    std::unique_lock<std::mutex> lock(mutex_);
    condVarResult_.notify_all();
}

int32_t CallbackWarp::SetArg(uint32_t arg)
{
    CHECK_AND_RETURN_RET_LOG(env_ != nullptr, MSERR_INVALID_OPERATION, "env is nullptr");
    CHECK_AND_RETURN_RET_LOG(argsCount_ > avgsPos_, MSERR_INVALID_OPERATION, "args num error");
    CHECK_AND_RETURN_RET_LOG(CheckArgv() == MSERR_OK, MSERR_NO_MEMORY, "malloc failed");
    napi_status status = napi_create_uint32(env_, arg, &argv_[avgsPos_++]);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, MSERR_INVALID_OPERATION, "create napi val failed");
    return MSERR_OK;
}

int32_t CallbackWarp::SetArg(int64_t arg)
{
    CHECK_AND_RETURN_RET_LOG(env_ != nullptr, MSERR_INVALID_OPERATION, "env is nullptr");
    CHECK_AND_RETURN_RET_LOG(argsCount_ > avgsPos_, MSERR_INVALID_OPERATION, "args num error");
    CHECK_AND_RETURN_RET_LOG(CheckArgv() == MSERR_OK, MSERR_NO_MEMORY, "malloc failed");
    napi_status status = napi_create_int64(env_, arg, &argv_[avgsPos_++]);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, MSERR_INVALID_OPERATION, "create napi val failed");
    return MSERR_OK;
}

int32_t CallbackWarp::SetArg(const std::shared_ptr<AVSharedMemory> &mem)
{
    CHECK_AND_RETURN_RET_LOG(env_ != nullptr, MSERR_INVALID_OPERATION, "env is nullptr");
    CHECK_AND_RETURN_RET_LOG(argsCount_ > avgsPos_, MSERR_INVALID_OPERATION, "args num error");
    CHECK_AND_RETURN_RET_LOG(mem != nullptr && mem->GetBase() != nullptr, MSERR_NO_MEMORY, "AVSharedMemory is null");
    CHECK_AND_RETURN_RET_LOG(CheckArgv() == MSERR_OK, MSERR_NO_MEMORY, "malloc failed");
    AvMemNapiWarp *memWarp = new(std::nothrow) AvMemNapiWarp(mem);
    CHECK_AND_RETURN_RET_LOG(memWarp != nullptr, MSERR_NO_MEMORY, "AvMemNapiWarp is null");
    napi_status status = napi_create_external_arraybuffer(env_, mem->GetBase(),
        static_cast<size_t>(mem->GetSize()), [] (napi_env env, void *data, void *hint) {
            (void)env;
            (void)data;
            AvMemNapiWarp *memWarp = reinterpret_cast<AvMemNapiWarp *>(hint);
            delete memWarp;
        }, reinterpret_cast<void *>(memWarp), &argv_[avgsPos_++]);
    if (status != napi_ok) {
        delete memWarp;
        MEDIA_LOGE("create napi val failed");
        return MSERR_INVALID_OPERATION;
    }
    return MSERR_OK;
}

const napi_value *CallbackWarp::GetArgs() const
{
    CHECK_AND_RETURN_RET_LOG(argsCount_ == avgsPos_, nullptr, "args need be full");
    return argv_;
}

size_t CallbackWarp::GetArgsCount() const
{
    CHECK_AND_RETURN_RET_LOG(argsCount_ == avgsPos_, 0, "args need be full");
    return argsCount_;
}

void CallbackWarp::SetResult(napi_value result)
{
    std::unique_lock<std::mutex> lock(mutex_);
    const int32_t refCount = 1;
    napi_status status = napi_create_reference(env_, result, refCount, &result_);
    CHECK_AND_RETURN_LOG(status == napi_ok, "create ref failed");
    resultValue_ = result;
    condVarResult_.notify_all();
}

void CallbackWarp::GetResult(napi_value &result)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (result_ == nullptr) {
        condVarResult_.wait(lock);
    }
    CHECK_AND_RETURN_LOG(result_ != nullptr, "result ref is nullptr");
    CHECK_AND_RETURN_LOG(resultValue_ != nullptr, "resultValue ref is nullptr");
    result = resultValue_;
}

std::string CallbackWarp::GetName() const
{
    CHECK_AND_RETURN_RET_LOG(jsCb_ != nullptr, "unknow", "jscallback is nullptr");
    return jsCb_->GetName();
}

int32_t CallbackWarp::CheckArgv()
{
    if (argv_ == nullptr) {
        argv_ = static_cast<napi_value *>(malloc(sizeof(napi_value) * argsCount_));
        avgsPos_ = 0;
    }
    CHECK_AND_RETURN_RET_LOG(argv_ != nullptr, MSERR_NO_MEMORY, "malloc fail");
    return MSERR_OK;
}

napi_value CallbackWarp::GetCallback() const
{
    return jsCb_->GetCallback();
}

napi_env CallbackWarp::GetEnv() const
{
    return env_;
}
} // namespace Media
} // namespace OHOS
