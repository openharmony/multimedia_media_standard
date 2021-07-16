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

#include "player_client.h"
#include "media_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerClient"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<PlayerClient> PlayerClient::Create(const sptr<IStandardPlayerService> &ipcProxy)
{
    std::shared_ptr<PlayerClient> player = std::make_shared<PlayerClient>(ipcProxy);
    CHECK_AND_RETURN_RET_LOG(player != nullptr, nullptr, "failed to new PlayerClient..");

    int32_t ret = player->CreateListenerObject();
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, nullptr, "failed to create listener object..");

    return player;
}

PlayerClient::PlayerClient(const sptr<IStandardPlayerService> &ipcProxy)
    : playerProxy_(ipcProxy)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayerClient::~PlayerClient()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (playerProxy_ != nullptr) {
        (void)playerProxy_->DestroyStub();
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t PlayerClient::CreateListenerObject()
{
    std::lock_guard<std::mutex> lock(mutex_);
    listenerStub_ = new(std::nothrow) PlayerListenerStub();
    CHECK_AND_RETURN_RET_LOG(listenerStub_ != nullptr, ERR_NO_MEMORY, "failed to new PlayerListenerStub object");
    CHECK_AND_RETURN_RET_LOG(playerProxy_ != nullptr, ERR_DEAD_OBJECT, "player service does not exist..");

    sptr<IRemoteObject> object = listenerStub_->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, ERR_DEAD_OBJECT, "listener object is nullptr..");

    MEDIA_LOGD("SetListenerObject");
    return playerProxy_->SetListenerObject(object);
}

void PlayerClient::MediaServerDied()
{
    std::lock_guard<std::mutex> lock(mutex_);
    playerProxy_ = nullptr;
    listenerStub_ = nullptr;
    if (callback_ != nullptr) {
        callback_->OnError(PLAYER_ERROR_SERVICE_DIED, 0);
    }
}

int32_t PlayerClient::SetSource(const std::string &uri)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerProxy_ != nullptr, ERR_DEAD_OBJECT, "player service does not exist..");
    return playerProxy_->SetSource(uri);
}

int32_t PlayerClient::Play()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerProxy_ != nullptr, ERR_DEAD_OBJECT, "player service does not exist..");
    return playerProxy_->Play();
}

int32_t PlayerClient::Prepare()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerProxy_ != nullptr, ERR_DEAD_OBJECT, "player service does not exist..");
    return playerProxy_->Prepare();
}

int32_t PlayerClient::PrepareAsync()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerProxy_ != nullptr, ERR_DEAD_OBJECT, "player service does not exist..");
    return playerProxy_->PrepareAsync();
}

int32_t PlayerClient::Pause()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerProxy_ != nullptr, ERR_DEAD_OBJECT, "player service does not exist..");
    return playerProxy_->Pause();
}

int32_t PlayerClient::Stop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerProxy_ != nullptr, ERR_DEAD_OBJECT, "player service does not exist..");
    return playerProxy_->Stop();
}

int32_t PlayerClient::Reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerProxy_ != nullptr, ERR_DEAD_OBJECT, "player service does not exist..");
    return playerProxy_->Reset();
}

int32_t PlayerClient::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerProxy_ != nullptr, ERR_DEAD_OBJECT, "player service does not exist..");
    return playerProxy_->Release();
}

int32_t PlayerClient::SetVolume(float leftVolume, float rightVolume)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerProxy_ != nullptr, ERR_DEAD_OBJECT, "player service does not exist..");
    return playerProxy_->SetVolume(leftVolume, rightVolume);
}

int32_t PlayerClient::Seek(uint64_t mSeconds, int32_t mode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerProxy_ != nullptr, ERR_DEAD_OBJECT, "player service does not exist..");
    return playerProxy_->Seek(mSeconds, mode);
}

int32_t PlayerClient::GetCurrentTime(uint64_t &currentTime)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerProxy_ != nullptr, ERR_DEAD_OBJECT, "player service does not exist..");
    return playerProxy_->GetCurrentTime(currentTime);
}

int32_t PlayerClient::SetPlaybackSpeed(int32_t mode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerProxy_ != nullptr, ERR_DEAD_OBJECT, "player service does not exist..");
    return playerProxy_->SetPlaybackSpeed(mode);
}

int32_t PlayerClient::GetPlaybackSpeed(int32_t &mode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerProxy_ != nullptr, ERR_DEAD_OBJECT, "player service does not exist..");
    return playerProxy_->GetPlaybackSpeed(mode);
}

int32_t PlayerClient::GetDuration(uint64_t &duration)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerProxy_ != nullptr, ERR_DEAD_OBJECT, "player service does not exist..");
    return playerProxy_->GetDuration(duration);
}

int32_t PlayerClient::SetVideoSurface(sptr<Surface> surface)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerProxy_ != nullptr, ERR_DEAD_OBJECT, "player service does not exist..");
    CHECK_AND_RETURN_RET_LOG(surface != nullptr, ERR_INVALID_VALUE, "surface is nullptr..");
    return playerProxy_->SetVideoSurface(surface);
}

bool PlayerClient::IsPlaying()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerProxy_ != nullptr, ERR_DEAD_OBJECT, "player service does not exist..");
    return playerProxy_->IsPlaying();
}

bool PlayerClient::IsLooping()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerProxy_ != nullptr, ERR_DEAD_OBJECT, "player service does not exist..");
    return playerProxy_->IsLooping();
}

int32_t PlayerClient::SetLooping(bool loop)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerProxy_ != nullptr, ERR_DEAD_OBJECT, "player service does not exist..");
    return playerProxy_->SetLooping(loop);
}

int32_t PlayerClient::SetPlayerCallback(const std::shared_ptr<PlayerCallback> &callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, ERR_INVALID_VALUE, "input param callback is nullptr..");
    CHECK_AND_RETURN_RET_LOG(listenerStub_ != nullptr, ERR_INVALID_OPERATION, "listenerStub_ is nullptr..");

    callback_ = callback;
    listenerStub_->SetPlayerCallback(callback);

    CHECK_AND_RETURN_RET_LOG(playerProxy_ != nullptr, ERR_DEAD_OBJECT, "player service does not exist..");
    return playerProxy_->SetPlayerCallback();
}
} // Media
} // OHOS