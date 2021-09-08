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

#ifndef CALLBACK_WARP_H
#define CALLBACK_WARP_H

#include <mutex>
#include "jscallback.h"

namespace OHOS {
namespace Media {
class CallbackWarp {
public:
    static std::shared_ptr<CallbackWarp> Create(napi_env env, const size_t argsCount,
        std::shared_ptr<JsCallback> jsCb);
    CallbackWarp(napi_env env, const size_t argsCount, std::shared_ptr<JsCallback> jsCb);
    ~CallbackWarp();
    DISALLOW_COPY_AND_MOVE(CallbackWarp);
    int32_t SetArg(uint32_t arg);
    int32_t SetArg(int64_t arg);
    const napi_value *GetArgs() const;
    size_t GetArgsCount() const;
    void SetResult(napi_value val);
    void GetResult(napi_value &result);
    std::string GetName() const;
    napi_value GetCallback() const;
    napi_env GetEnv() const;
    int32_t CallFuncAndSendResult();
    void CancelResult();

private:
    int32_t CheckArgv();
    napi_env env_ = nullptr;
    size_t argsCount_ = 0;
    std::shared_ptr<JsCallback> jsCb_ = nullptr;
    napi_value *argv_ = nullptr;
    size_t avgsPos_ = 0;
    napi_value resultValue_ = nullptr;
    napi_ref result_ = nullptr;
    std::mutex mutex_;
    std::condition_variable condVarResult_;
};
} // namespace Media
} // namespace OHOS
#endif
