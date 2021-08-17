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

#include "media_client.h"
#include "media_log.h"
#include "recorder_client.h"
#include "player_client.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "i_standard_recorder_service.h"
#include "i_standard_player_service.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaClient"};
}

namespace OHOS {
namespace Media {
IMediaService &MeidaServiceFactory::GetInstance()
{
    static MediaClient instance;
    return instance;
}

MediaClient::MediaClient()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaClient::~MediaClient()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

bool MediaClient::IsAlived()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (mediaProxy_ == nullptr) {
        mediaProxy_ = GetMediaProxy();
    }

    return (mediaProxy_ != nullptr) ? true : false;
}

std::shared_ptr<IRecorderService> MediaClient::CreateRecorderService()
{
    if (!IsAlived()) {
        MEDIA_LOGE("media service does not exist..");
        return nullptr;
    }

    sptr<IRemoteObject> object = mediaProxy_->GetSubSystemAbility(
        IStandardMediaService::MediaSystemAbility::MEDIA_RECORDER);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "recorder proxy object is nullptr.");

    sptr<IStandardRecorderService> recorderProxy = iface_cast<IStandardRecorderService>(object);
    CHECK_AND_RETURN_RET_LOG(recorderProxy != nullptr, nullptr, "recorder proxy is nullptr.");

    std::shared_ptr<RecorderClient> recorder = RecorderClient::Create(recorderProxy);
    CHECK_AND_RETURN_RET_LOG(recorder != nullptr, nullptr, "failed to create recorder client.");

    std::lock_guard<std::mutex> lock(mutex_);
    recorderClientList_.push_back(recorder);
    return recorder;
}

std::shared_ptr<IPlayerService> MediaClient::CreatePlayerService()
{
    if (!IsAlived()) {
        MEDIA_LOGE("media service does not exist..");
        return nullptr;
    }

    sptr<IRemoteObject> object = mediaProxy_->GetSubSystemAbility(
        IStandardMediaService::MediaSystemAbility::MEDIA_PLAYER);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "player proxy object is nullptr.");

    sptr<IStandardPlayerService> playerProxy = iface_cast<IStandardPlayerService>(object);
    CHECK_AND_RETURN_RET_LOG(playerProxy != nullptr, nullptr, "player proxy is nullptr.");

    std::shared_ptr<PlayerClient> player = PlayerClient::Create(playerProxy);
    CHECK_AND_RETURN_RET_LOG(player != nullptr, nullptr, "failed to create player client.");

    std::lock_guard<std::mutex> lock(mutex_);
    playerClientList_.push_back(player);
    return player;
}

int32_t MediaClient::DestroyRecorderService(std::shared_ptr<IRecorderService> recorder)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorder != nullptr, ERR_INVALID_OPERATION, "input recorder is nullptr.");
    recorderClientList_.remove(recorder);
    return ERR_OK;
}

int32_t MediaClient::DestroyPlayerService(std::shared_ptr<IPlayerService> player)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(player != nullptr, ERR_INVALID_OPERATION, "input player is nullptr.");
    playerClientList_.remove(player);
    return ERR_OK;
}

sptr<IStandardMediaService> MediaClient::GetMediaProxy()
{
    MEDIA_LOGD("enter");
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHECK_AND_RETURN_RET_LOG(samgr != nullptr, nullptr, "system ability manager is nullptr.");

    sptr<IRemoteObject> object = samgr->GetSystemAbility(OHOS::PLAYER_DISTRIBUTED_SERVICE_ID);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "meida object is nullptr.");

    mediaProxy_ = iface_cast<IStandardMediaService>(object);
    CHECK_AND_RETURN_RET_LOG(mediaProxy_ != nullptr, nullptr, "media proxy is nullptr.");

    deathRecipient_ = new(std::nothrow) MediaDeathRecipient();
    CHECK_AND_RETURN_RET_LOG(deathRecipient_ != nullptr, nullptr, "failed to new MediaDeathRecipient.");

    deathRecipient_->SetNotifyCb(std::bind(&MediaClient::MediaServerDied, this));
    bool result = object->AddDeathRecipient(deathRecipient_);
    if (!result) {
        MEDIA_LOGE("failed to add deathRecipient");
        return nullptr;
    }
    return mediaProxy_;
}

void MediaClient::MediaServerDied()
{
    MEDIA_LOGE("media server is died!");
    std::lock_guard<std::mutex> lock(mutex_);
    (void)mediaProxy_->AsObject()->RemoveDeathRecipient(deathRecipient_);
    mediaProxy_ = nullptr;
    deathRecipient_ = nullptr;

    for (auto &it : recorderClientList_) {
        auto recorder = std::static_pointer_cast<RecorderClient>(it);
        if (recorder != nullptr) {
            recorder->MediaServerDied();
        }
    }

    for (auto &it : playerClientList_) {
        auto player = std::static_pointer_cast<PlayerClient>(it);
        if (player != nullptr) {
            player->MediaServerDied();
        }
    }
}
} // Media
} // OHOS
