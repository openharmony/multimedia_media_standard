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

#include "recorder_engine_dl.h"
#include <dlfcn.h>
#include "media_log.h"
#include "errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderEngineDl"};
}

namespace OHOS {
namespace Media {
const std::string RECORDER_ENGINE_PATH = "/system/lib/libmedia_recorder_engine.z.so";
const std::string RECORDER_ENGINE_CREATE = "CreateRecorderEngine";
RecorderEngineDl &RecorderEngineDl::Instance()
{
    static RecorderEngineDl instance;
    return instance;
}

std::shared_ptr<IRecorderEngine> RecorderEngineDl::CreateEngine()
{
    std::lock_guard<std::mutex> lock(mutex_);
    GetEntry();
    CHECK_AND_RETURN_RET_LOG(createFunc_ != nullptr, nullptr, "createFunc_ is nullptr");
    IRecorderEngine *engine = createFunc_();
    CHECK_AND_RETURN_RET_LOG(engine != nullptr, nullptr, "failed to create engine");
    std::shared_ptr<IRecorderEngine> retEngine(engine);
    return retEngine;
}

RecorderEngineDl::RecorderEngineDl()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

RecorderEngineDl::~RecorderEngineDl()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
    if (engineLib_ != nullptr) {
        (void)dlclose(engineLib_);
        engineLib_ = nullptr;
    }
}

void RecorderEngineDl::GetEntry()
{
    if (engineLib_ == nullptr) {
        engineLib_ = dlopen(RECORDER_ENGINE_PATH.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (engineLib_ == nullptr) {
            MEDIA_LOGE("failed to dlopen %{public}s, errno:%{public}d, errormsg:%{public}s",
                RECORDER_ENGINE_PATH.c_str(), errno, dlerror());
            return;
        }

        createFunc_ = reinterpret_cast<CreateEngineFunc>(dlsym(engineLib_, RECORDER_ENGINE_CREATE.c_str()));
        if (createFunc_ == nullptr) {
            MEDIA_LOGE("failed to dlsym %{public}s, errno:%{public}d, errormsg:%{public}s",
                RECORDER_ENGINE_CREATE.c_str(), errno, dlerror());
            (void)dlclose(engineLib_);
            engineLib_ = nullptr;
        }
    }
}
} // Media
} // OHOS