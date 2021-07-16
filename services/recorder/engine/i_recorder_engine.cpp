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

#include "i_recorder_engine.h"
#include "recorder_engine_hst_impl.h"
#include "media_log.h"
#include "errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "IRecorderEngine"};
}

#ifdef __cplusplus
extern "C" {
#endif
__attribute__((visibility("default"))) OHOS::Media::IRecorderEngine *CreateRecorderEngine()
{
    MEDIA_LOGD("enter");
    OHOS::Media::RecorderEngineHstImpl *engine = new(std::nothrow) OHOS::Media::RecorderEngineHstImpl();
    CHECK_AND_RETURN_RET_LOG(engine != nullptr, nullptr, "failed to new RecorderEngineHstImpl");
    int32_t ret = engine->Init();
    if (ret != OHOS::ERR_OK) {
        MEDIA_LOGE("engine init failed !");
        delete engine;
        return nullptr;
    }
    return engine;
}
#ifdef __cplusplus
}
#endif