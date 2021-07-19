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

#include "player_register.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerEngineFactory"};
}

namespace OHOS {
namespace Media {
PlayerRegister::RegisterHelp::RegisterHelp(std::shared_ptr<PlayerFactoryMake> factoryPtr)
{
    PlayerRegister::GetInstance().playerEngineVec_.push_back(factoryPtr);
}

IPlayerEngine *PlayerRegister::CreateEngine(const std::string& uri) const
{
    int max = 0;
    int score = 0;
    int pos = 0;
    IPlayerEngine *res = nullptr;

    for (size_t i = 0; i < playerEngineVec_.size(); ++i) {
        if (!playerEngineVec_[i]) {
            MEDIA_LOGE("Register error, get the nullptr");
            return nullptr;
        }
        score = playerEngineVec_[i]->Score(uri);
        if (score > max) {
            max = score;
            pos = i;
        }
    }
    res = playerEngineVec_[pos]->CreatePlayer();
    if (!res) {
        MEDIA_LOGE("CreatePlayer failed!");
        return nullptr;
    }

    return res;
}
} // Media
} // OHOS