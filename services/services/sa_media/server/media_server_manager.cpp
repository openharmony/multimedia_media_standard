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

#include "media_server_manager.h"
#include "recorder_service_stub.h"
#include "player_service_stub.h"
#include "media_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaServerManager"};
}

namespace OHOS {
namespace Media {
constexpr uint32_t SERVER_MAX_NUMBER = 16;
MediaServerManager &MediaServerManager::GetInstance()
{
    static MediaServerManager instance;
    return instance;
}

MediaServerManager::MediaServerManager()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaServerManager::~MediaServerManager()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

sptr<IRemoteObject> MediaServerManager::CreateStubObject(StubType type)
{
    std::lock_guard<std::mutex> lock(mutex_);
    switch (type) {
        case RECORDER: {
            if (recorderStubList_.size() >= SERVER_MAX_NUMBER) {
                MEDIA_LOGE("The number of recorder services(%{public}zu) has reached the upper limit."
                           "Please release the applied resources.", recorderStubList_.size());
                return nullptr;
            }
            sptr<RecorderServiceStub> recorderStub = RecorderServiceStub::Create();
            if (recorderStub == nullptr) {
                MEDIA_LOGE("failed to create RecorderServiceStub");
                return nullptr;
            }
            sptr<IRemoteObject> object = recorderStub->AsObject();
            if (object != nullptr) {
                recorderStubList_.push_back(object);
                MEDIA_LOGD("The number of recorder services(%{public}zu).", recorderStubList_.size());
            }
            return object;
        }
        case PLAYER: {
            if (playerStubList_.size() >= SERVER_MAX_NUMBER) {
                MEDIA_LOGE("The number of player services(%{public}zu) has reached the upper limit."
                           "Please release the applied resources.", playerStubList_.size());
                return nullptr;
            }
            sptr<PlayerServiceStub> playerStub = PlayerServiceStub::Create();
            if (playerStub == nullptr) {
                MEDIA_LOGE("failed to create PlayerServiceStub");
                return nullptr;
            }
            sptr<IRemoteObject> object = playerStub->AsObject();
            if (object != nullptr) {
                playerStubList_.push_back(object);
                MEDIA_LOGD("The number of player services(%{public}zu).", playerStubList_.size());
            }
            return object;
        }
        default: {
            MEDIA_LOGE("default case, media server manager failed");
            return nullptr;
        }
    }
}

void MediaServerManager::DestroyStubObject(StubType type, sptr<IRemoteObject> object)
{
    std::lock_guard<std::mutex> lock(mutex_);
    switch (type) {
        case RECORDER: {
            recorderStubList_.remove(object);
            break;
        }
        case PLAYER: {
            playerStubList_.remove(object);
            break;
        }
        default: {
            MEDIA_LOGE("default case, media server manager failed");
            break;
        }
    }
}
} // Media
} // OHOS
