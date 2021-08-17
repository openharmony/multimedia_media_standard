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

#include "player_impl.h"
#include "i_media_service.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerImpl"};
}

namespace OHOS {
namespace Media {
const std::map<PlayerErrorType, std::string> PLAYER_ERRTYPE_INFOS = {
    {PLAYER_ERROR, "internal player error"},
    {PLAYER_ERROR_UNKNOWN, "unknow player error"},
    {PLAYER_ERROR_SERVICE_DIED, "player service died"},
    {PLAYER_ERROR_EXTEND_START, "player extend start error type"},
};

std::string PlayerErrorTypeToString(PlayerErrorType type)
{
    if (PLAYER_ERRTYPE_INFOS.count(type) != 0) {
        return PLAYER_ERRTYPE_INFOS.at(type);
    }

    if (type > PLAYER_ERROR_EXTEND_START) {
        return "extend error type:" + std::to_string(static_cast<int32_t>(type - PLAYER_ERROR_EXTEND_START));
    }

    return "invalid error type:" + std::to_string(static_cast<int32_t>(type));
}

std::shared_ptr<Player> PlayerFactory::CreatePlayer()
{
    std::shared_ptr<PlayerImpl> impl = std::make_shared<PlayerImpl>();
    CHECK_AND_RETURN_RET_LOG(impl != nullptr, nullptr, "failed to new PlayerImpl");

    int32_t ret = impl->Init();
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, nullptr, "failed to init PlayerImpl");

    return impl;
}

int32_t PlayerImpl::Init()
{
    playerService_ = MeidaServiceFactory::GetInstance().CreatePlayerService();
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, ERR_NO_INIT, "failed to create player service");
    return ERR_OK;
}

PlayerImpl::PlayerImpl()
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayerImpl::~PlayerImpl()
{
    if (playerService_ != nullptr) {
        (void)MeidaServiceFactory::GetInstance().DestroyPlayerService(playerService_);
        playerService_ = nullptr;
    }
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t PlayerImpl::SetSource(const std::string &uri)
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, ERR_INVALID_OPERATION, "player service does not exist..");
    CHECK_AND_RETURN_RET_LOG(!uri.empty(), ERR_INVALID_VALUE, "uri is empty..");
    return playerService_->SetSource(uri);
}

int32_t PlayerImpl::Play()
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, ERR_INVALID_OPERATION, "player service does not exist..");

    return playerService_->Play();
}

int32_t PlayerImpl::Prepare()
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, ERR_INVALID_OPERATION, "player service does not exist..");

    return playerService_->Prepare();
}

int32_t PlayerImpl::PrepareAsync()
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, ERR_INVALID_OPERATION, "player service does not exist..");

    return playerService_->PrepareAsync();
}

int32_t PlayerImpl::Pause()
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, ERR_INVALID_OPERATION, "player service does not exist..");

    return playerService_->Pause();
}

int32_t PlayerImpl::Stop()
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, ERR_INVALID_OPERATION, "player service does not exist..");

    return playerService_->Stop();
}

int32_t PlayerImpl::Reset()
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, ERR_INVALID_OPERATION, "player service does not exist..");

    return playerService_->Reset();
}

int32_t PlayerImpl::Release()
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, ERR_INVALID_OPERATION, "player service does not exist..");
    (void)playerService_->Release();
    if (playerService_ != nullptr) {
        (void)MeidaServiceFactory::GetInstance().DestroyPlayerService(playerService_);
        playerService_ = nullptr;
    }
    return MSERR_OK;
}

int32_t PlayerImpl::SetVolume(float leftVolume, float rightVolume)
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, ERR_INVALID_OPERATION, "player service does not exist..");

    return playerService_->SetVolume(leftVolume, rightVolume);
}

int32_t PlayerImpl::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, ERR_INVALID_OPERATION, "player service does not exist..");

    return playerService_->Seek(mSeconds, mode);
}

int32_t PlayerImpl::GetCurrentTime(int32_t &currentTime)
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, ERR_INVALID_OPERATION, "player service does not exist..");

    return playerService_->GetCurrentTime(currentTime);
}

int32_t PlayerImpl::SetPlaybackSpeed(PlaybackRateMode mode)
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, ERR_INVALID_OPERATION, "player service does not exist..");

    return playerService_->SetPlaybackSpeed(mode);
}

int32_t PlayerImpl::GetPlaybackSpeed(PlaybackRateMode &mode)
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, ERR_INVALID_OPERATION, "player service does not exist..");

    return playerService_->GetPlaybackSpeed(mode);
}

int32_t PlayerImpl::GetDuration(int32_t &duration)
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, ERR_INVALID_OPERATION, "player service does not exist..");

    return playerService_->GetDuration(duration);
}

int32_t PlayerImpl::SetVideoSurface(sptr<Surface> surface)
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, ERR_INVALID_OPERATION, "player service does not exist..");
    CHECK_AND_RETURN_RET_LOG(surface != nullptr, ERR_INVALID_VALUE, "surface is nullptr");

    return playerService_->SetVideoSurface(surface);
}

bool PlayerImpl::IsPlaying()
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, false, "player service does not exist..");

    return playerService_->IsPlaying();
}

bool PlayerImpl::IsLooping()
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, false, "player service does not exist..");

    return playerService_->IsLooping();
}

int32_t PlayerImpl::SetLooping(bool loop)
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, ERR_INVALID_OPERATION, "player service does not exist..");

    return playerService_->SetLooping(loop);
}

int32_t PlayerImpl::SetPlayerCallback(const std::shared_ptr<PlayerCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, ERR_INVALID_OPERATION, "player service does not exist..");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, ERR_INVALID_VALUE, "callback is nullptr");
    return playerService_->SetPlayerCallback(callback);
}
} // nmamespace Media
} // namespace OHOS