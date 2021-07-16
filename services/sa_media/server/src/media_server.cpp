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

#include "media_server.h"
#include "iservice_registry.h"
#include "media_log.h"
#include "system_ability_definition.h"
#include "media_server_manager.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaServer"};
}

namespace OHOS {
namespace Media {
REGISTER_SYSTEM_ABILITY_BY_ID(MediaServer, PLAYER_DISTRIBUTED_SERVICE_ID, true)
MediaServer::MediaServer(int32_t systemAbilityId, bool runOnCreate)
    : SystemAbility(systemAbilityId, runOnCreate)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaServer::~MediaServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void MediaServer::OnDump()
{
    MEDIA_LOGD("MediaServer OnDump");
}

void MediaServer::OnStart()
{
    MEDIA_LOGD("MediaServer OnStart");
    bool res = Publish(this);
    if (res) {
        MEDIA_LOGD("MediaServer OnStart res=%{public}d", res);
    }
}

void MediaServer::OnStop()
{
    MEDIA_LOGD("MediaServer OnStop");
}

sptr<IRemoteObject> MediaServer::GetSubSystemAbility(IStandardMediaService::MediaSystemAbility subSystemId)
{
    switch (subSystemId) {
        case MediaSystemAbility::MEDIA_RECORDER: {
            return MediaServerManager::GetInstance().CreateStubObject(MediaServerManager::RECORDER);
        }
        case MediaSystemAbility::MEDIA_PLAYER: {
            return MediaServerManager::GetInstance().CreateStubObject(MediaServerManager::PLAYER);
        } 
        default: {
            MEDIA_LOGE("default case, media client need check subSystemId");
            return nullptr;
        }
    }
}
} // Media
} // OHOS