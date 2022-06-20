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
#include "media_errors.h"
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

sptr<IRemoteObject> MediaServer::GetSubSystemAbility(IStandardMediaService::MediaSystemAbility subSystemId,
    const sptr<IRemoteObject> &listener)
{
    int32_t ret = MediaServiceStub::SetDeathListener(listener);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed set death listener");

    switch (subSystemId) {
        case MediaSystemAbility::MEDIA_RECORDER: {
            return MediaServerManager::GetInstance().CreateStubObject(MediaServerManager::RECORDER);
        }
        case MediaSystemAbility::MEDIA_PLAYER: {
            return MediaServerManager::GetInstance().CreateStubObject(MediaServerManager::PLAYER);
        }
        case MediaSystemAbility::MEDIA_AVMETADATAHELPER: {
            return MediaServerManager::GetInstance().CreateStubObject(MediaServerManager::AVMETADATAHELPER);
        }
        case MediaSystemAbility::MEDIA_CODECLIST: {
            return MediaServerManager::GetInstance().CreateStubObject(MediaServerManager::AVCODECLIST);
        }
        case MediaSystemAbility::MEDIA_AVCODEC: {
            return MediaServerManager::GetInstance().CreateStubObject(MediaServerManager::AVCODEC);
        }
        case MediaSystemAbility::RECORDER_PROFILES: {
            return MediaServerManager::GetInstance().CreateStubObject(MediaServerManager::RECORDERPROFILES);
        }
        case MediaSystemAbility::MEDIA_AVMUXER: {
            return MediaServerManager::GetInstance().CreateStubObject(MediaServerManager::AVMUXER);
        }
        default: {
            MEDIA_LOGE("default case, media client need check subSystemId");
            return nullptr;
        }
    }
}

int32_t MediaServer::Dump(int32_t fd, const std::vector<std::u16string> &args)
{
    if (fd <= 0) {
        MEDIA_LOGW("Failed to check fd");
        return OHOS::INVALID_OPERATION;
    }
    if (MediaServerManager::GetInstance().Dump(fd, args) != OHOS::NO_ERROR) {
        MEDIA_LOGW("Failed to call MediaServerManager::Dump");
        return OHOS::INVALID_OPERATION;
    }

    return OHOS::NO_ERROR;
}
} // namespace Media
} // namespace OHOS
