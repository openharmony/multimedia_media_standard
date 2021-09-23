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

#ifndef CALLBACK_WORKS_H
#define CALLBACK_WORKS_H

#include <mutex>
#include <uv.h>
#include <unordered_set>
#include "callback_warp.h"

namespace OHOS {
namespace Media {
class CallbackWorks : public std::enable_shared_from_this<CallbackWorks> {
public:
    explicit CallbackWorks(napi_env env);
    ~CallbackWorks();
    DISALLOW_COPY_AND_MOVE(CallbackWorks);
    int32_t Push(const std::shared_ptr<CallbackWarp> &callback);
    int32_t Remove(uv_work_t *work);
    void CancelAll();

private:
    uv_work_t *InitWork(const std::shared_ptr<CallbackWarp> &callback);
    void DeinitWork(uv_work_t *work) const;
    int32_t Run(uv_work_t *work);
    std::unordered_set<uv_work_t *> works_;
    std::mutex mutex_;
    napi_env env_ = nullptr;
};
} // namespace Media
} // namespace OHOS
#endif
