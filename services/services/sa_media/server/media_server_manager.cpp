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
#include "avmetadatahelper_service_stub.h"
#include "avcodeclist_service_stub.h"
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
            return CreateRecorderStubObject();
        }
        case PLAYER: {
            return CreatePlayerStubObject();
        }
        case AVMETADATAHELPER: {
            return CreateAVMetadataHelperStubObject();
        }
        case AVCODECLIST: {
            return CreateAVCodecListStubObject();
        }
        default: {
            MEDIA_LOGE("default case, media server manager failed");
            return nullptr;
        }
    }
}

sptr<IRemoteObject> MediaServerManager::CreatePlayerStubObject()
{
    if (playerStubMap_.size() >= SERVER_MAX_NUMBER) {
        MEDIA_LOGE("The number of player services(%{public}zu) has reached the upper limit."
            "Please release the applied resources.", playerStubMap_.size());
        return nullptr;
    }
    sptr<PlayerServiceStub> playerStub = PlayerServiceStub::Create();
    if (playerStub == nullptr) {
        MEDIA_LOGE("failed to create PlayerServiceStub");
        return nullptr;
    }
    sptr<IRemoteObject> object = playerStub->AsObject();
    if (object != nullptr) {
        pid_t pid = IPCSkeleton::GetCallingPid();
        playerStubMap_[object] = pid;
        MEDIA_LOGD("The number of player services(%{public}zu) pid(%{public}d).", playerStubMap_.size(), pid);
    }
    return object;
}

sptr<IRemoteObject> MediaServerManager::CreateRecorderStubObject()
{
    if (recorderStubMap_.size() >= SERVER_MAX_NUMBER) {
        MEDIA_LOGE("The number of recorder services(%{public}zu) has reached the upper limit."
            "Please release the applied resources.", recorderStubMap_.size());
        return nullptr;
    }
    sptr<RecorderServiceStub> recorderStub = RecorderServiceStub::Create();
    if (recorderStub == nullptr) {
        MEDIA_LOGE("failed to create RecorderServiceStub");
        return nullptr;
    }
    sptr<IRemoteObject> object = recorderStub->AsObject();
    if (object != nullptr) {
        pid_t pid = IPCSkeleton::GetCallingPid();
        recorderStubMap_[object] = pid;
        MEDIA_LOGD("The number of recorder services(%{public}zu) pid(%{public}d).", recorderStubMap_.size(), pid);
    }
    return object;
}

sptr<IRemoteObject> MediaServerManager::CreateAVMetadataHelperStubObject()
{
    constexpr uint32_t metadataHelperNumMax = 32;
    if (avMetadataHelperStubMap_.size() >= metadataHelperNumMax) {
        MEDIA_LOGE("The number of avmetadatahelper services(%{public}zu) has reached the upper limit."
            "Please release the applied resources.", avMetadataHelperStubMap_.size());
        return nullptr;
    }
    sptr<AVMetadataHelperServiceStub> avMetadataHelperStub = AVMetadataHelperServiceStub::Create();
    if (avMetadataHelperStub == nullptr) {
        MEDIA_LOGE("failed to create AVMetadataHelperServiceStub");
        return nullptr;
    }
    sptr<IRemoteObject> object = avMetadataHelperStub->AsObject();
    if (object != nullptr) {
        pid_t pid = IPCSkeleton::GetCallingPid();
        avMetadataHelperStubMap_[object] = pid;
        MEDIA_LOGD("The number of avmetadatahelper services(%{public}zu).", avMetadataHelperStubMap_.size());
    }
    return object;
}

sptr<IRemoteObject> MediaServerManager::CreateAVCodecListStubObject()
{
    if (avCodecListStubMap_.size() >= SERVER_MAX_NUMBER) {
        MEDIA_LOGE("The number of codeclist services(%{public}zu) has reached the upper limit."
            "Please release the applied resources.", avCodecListStubMap_.size());
        return nullptr;
    }
    sptr<AVCodecListServiceStub> avCodecListStub = AVCodecListServiceStub::Create();
    if (avCodecListStub == nullptr) {
        MEDIA_LOGE("failed to create AVCodecListServiceStub");
        return nullptr;
    }
    sptr<IRemoteObject> object = avCodecListStub->AsObject();
    if (object != nullptr) {
        pid_t pid = IPCSkeleton::GetCallingPid();
        avCodecListStubMap_[object] = pid;
        MEDIA_LOGD("The number of codeclist services(%{public}zu).", avCodecListStubMap_.size());
    }
    return object;
}

void MediaServerManager::DestroyStubObject(StubType type, sptr<IRemoteObject> object)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pid_t pid = IPCSkeleton::GetCallingPid();
    switch (type) {
        case RECORDER: {
            for (auto it = recorderStubMap_.begin(); it != recorderStubMap_.end(); it++) {
                if (it->first == object) {
                    MEDIA_LOGD("destory recorder stub services(%{public}zu) pid(%{public}d).",
                        recorderStubMap_.size(), pid);
                    (void)recorderStubMap_.erase(it);
                    return;
                }
            }
            MEDIA_LOGE("find recorder object failed, pid(%{public}d).", pid);
            break;
        }
        case PLAYER: {
            for (auto it = playerStubMap_.begin(); it != playerStubMap_.end(); it++) {
                if (it->first == object) {
                    MEDIA_LOGD("destory player stub services(%{public}zu) pid(%{public}d).",
                        playerStubMap_.size(), pid);
                    (void)playerStubMap_.erase(it);
                    return;
                }
            }
            MEDIA_LOGE("find player object failed, pid(%{public}d).", pid);
            break;
        }
        case AVMETADATAHELPER: {
            for (auto it = avMetadataHelperStubMap_.begin(); it != avMetadataHelperStubMap_.end(); it++) {
                if (it->first == object) {
                    MEDIA_LOGD("destory avmetadatahelper stub services(%{public}zu) pid(%{public}d).",
                        avMetadataHelperStubMap_.size(), pid);
                    (void)avMetadataHelperStubMap_.erase(it);
                    return;
                }
            }
            MEDIA_LOGE("find avmetadatahelper object failed, pid(%{public}d).", pid);
            break;
        }
        default: {
            MEDIA_LOGE("default case, media server manager failed, pid(%{public}d).", pid);
            break;
        }
    }
}

void MediaServerManager::DestroyStubObjectForPid(pid_t pid)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("recorder stub services(%{public}zu) pid(%{public}d).", recorderStubMap_.size(), pid);
    for (auto itRecorder = recorderStubMap_.begin(); itRecorder != recorderStubMap_.end();) {
        if (itRecorder->second == pid) {
            itRecorder = recorderStubMap_.erase(itRecorder);
        } else {
            itRecorder++;
        }
    }
    MEDIA_LOGD("recorder stub services(%{public}zu).", recorderStubMap_.size());

    MEDIA_LOGD("player stub services(%{public}zu) pid(%{public}d).", playerStubMap_.size(), pid);
    for (auto itPlayer = playerStubMap_.begin(); itPlayer != playerStubMap_.end();) {
        if (itPlayer->second == pid) {
            itPlayer = playerStubMap_.erase(itPlayer);
        } else {
            itPlayer++;
        }
    }
    MEDIA_LOGD("player stub services(%{public}zu).", playerStubMap_.size());

    MEDIA_LOGD("avmetadatahelper stub services(%{public}zu) pid(%{public}d).", avMetadataHelperStubMap_.size(), pid);
    for (auto itAvMetadata = avMetadataHelperStubMap_.begin(); itAvMetadata != avMetadataHelperStubMap_.end();) {
        if (itAvMetadata->second == pid) {
            itAvMetadata = avMetadataHelperStubMap_.erase(itAvMetadata);
        } else {
            itAvMetadata++;
        }
    }
    MEDIA_LOGD("avmetadatahelper stub services(%{public}zu).", avMetadataHelperStubMap_.size());
}
} // Media
} // OHOS
