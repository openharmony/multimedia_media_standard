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

#include "player_server.h"
#include <fstream>
#include "media_log.h"
#include "errors.h"
#include "player_engine_dl.h"
#include "bytrace.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerServer"};
}

namespace OHOS {
namespace Media {
const std::string START_TAG = "PlayerCreate->Start";
const std::string STOP_TAG = "PlayerStop->Destroy";
std::shared_ptr<IPlayerService> PlayerServer::Create()
{
    std::shared_ptr<PlayerServer> server = std::make_shared<PlayerServer>();
    CHECK_AND_RETURN_RET_LOG(server != nullptr, nullptr, "failed to new PlayerServer");

    int32_t ret = server->Init();
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, nullptr, "Player server init Failed!");
    return server;
}

PlayerServer::PlayerServer()
    : startTimeMonitor_(START_TAG),
      stopTimeMonitor_(STOP_TAG),
      cbLoop_("player_cb_loop")
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayerServer::~PlayerServer()
{
    (void)cbLoop_.Stop();
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t PlayerServer::Init()
{
    return cbLoop_.Start();
}

int32_t PlayerServer::SetSource(const std::string &uri)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (status_ != PLAYER_IDLE) {
        MEDIA_LOGE("current state is: %{public}d, not support SetSource\n", status_);
        return ERR_INVALID_OPERATION;
    }

    startTimeMonitor_.StartTime();

    MEDIA_LOGD("current uri is : %{public}s", uri.c_str());
    playerEngine_ = PlayerEngineDl::Instance().CreateEngine(uri);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, ERR_INVALID_OPERATION, "failed to create player engine");

    int32_t ret = playerEngine_->SetSource(uri);
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, ERR_INVALID_OPERATION, "SetSource Failed!");

    std::shared_ptr<IPlayerEngineObs> obs = shared_from_this();
    ret = playerEngine_->SetObs(obs);
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, ERR_INVALID_OPERATION, "SetObs Failed!");

    status_ = PLAYER_INITIALIZED;
    return ret;
}

int32_t PlayerServer::Prepare()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, ERR_INVALID_OPERATION, "playerEngine_ is nullptr");

    if (status_ != PLAYER_INITIALIZED && status_ != PLAYER_STOPPED) {
        MEDIA_LOGE("Can not Prepare, currentState is %{public}d", status_);
        return ERR_INVALID_OPERATION;
    }

    int32_t ret = ERR_OK;
    if (surface_ != nullptr) {
        ret = playerEngine_->SetVideoSurface(surface_);
        CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, ERR_INVALID_OPERATION, "Engine SetVideoSurface Failed!");
    }

    ret = playerEngine_->Prepare();
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, ERR_INVALID_OPERATION, "Engine Prepare Failed!");
    status_ = PLAYER_PREPARED;
    return ERR_OK;
}

int32_t PlayerServer::Play()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, ERR_INVALID_OPERATION, "playerEngine_ is nullptr");

    if (status_ != PLAYER_PREPARED && status_ != PLAYER_PLAYBACK_COMPLETE &&
        status_ != PLAYER_PAUSED && status_ != PLAYER_STARTED) {
        MEDIA_LOGE("Can not Play, currentState is %{public}d", status_);
        return ERR_INVALID_OPERATION;
    }

    if (status_ == PLAYER_STARTED) {
        return ERR_OK;
    }

    int32_t ret = playerEngine_->Play();
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, ERR_INVALID_OPERATION, "Engine Play Failed!");

    startTimeMonitor_.FinishTime();
    status_ = PLAYER_STARTED;
    return ERR_OK;
}

int32_t PlayerServer::PrepareAsync()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, ERR_INVALID_OPERATION, "playerEngine_ is nullptr");

    if (status_ != PLAYER_INITIALIZED && status_ != PLAYER_STOPPED) {
        MEDIA_LOGE("Can not PrepareAsync, currentState is %{public}d", status_);
        return ERR_INVALID_OPERATION;
    }

    if (status_ == PLAYER_PREPARING) {
        return ERR_OK;
    }

    int32_t ret = ERR_OK;
    if (surface_ != nullptr) {
        ret = playerEngine_->SetVideoSurface(surface_);
        CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, ERR_INVALID_OPERATION, "Engine SetVideoSurface Failed!");
    }
    ret = playerEngine_->PrepareAsync();
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, ERR_INVALID_OPERATION, "Engine PrepareAsync Failed!");
    status_ = PLAYER_PREPARING;
    return ERR_OK;
}

int32_t PlayerServer::Pause()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, ERR_INVALID_OPERATION, "playerEngine_ is nullptr");

    if (status_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not Pause, currentState is %{public}d", status_);
        return ERR_INVALID_OPERATION;
    }

    if (status_ == PLAYER_PAUSED || status_ == PLAYER_PLAYBACK_COMPLETE) {
        MEDIA_LOGI("currentState is %{public}d", status_);
        return ERR_OK;
    }

    if (status_ != PLAYER_STARTED) {
        MEDIA_LOGE("Can not Pause, status_ is %{public}d", status_);
        return ERR_INVALID_OPERATION;
    }

    int32_t ret = playerEngine_->Pause();
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, ERR_INVALID_OPERATION, "Engine Pause Failed!");
    status_ = PLAYER_PAUSED;
    return ERR_OK;
}

int32_t PlayerServer::Stop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, ERR_INVALID_OPERATION, "playerEngine_ is nullptr");

    if (status_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not Stop, currentState is %{public}d", status_);
        return ERR_INVALID_OPERATION;
    }

    if (status_ == PLAYER_STOPPED) {
        MEDIA_LOGI("currentState is %{public}d", status_);
        return ERR_OK;
    }

    if ((status_ != PLAYER_PREPARED) && (status_ != PLAYER_STARTED) &&
        (status_ != PLAYER_PLAYBACK_COMPLETE) && (status_ != PLAYER_PAUSED)) {
        MEDIA_LOGE("current state: %{public}d, can not stop", status_);
        return ERR_INVALID_OPERATION;
    }

    stopTimeMonitor_.StartTime();

    int32_t ret = playerEngine_->Stop();
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, ERR_INVALID_OPERATION, "Engine Stop Failed!");
    status_ = PLAYER_STOPPED;
    return ret;
}

int32_t PlayerServer::Reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return OnReset();
}

int32_t PlayerServer::OnReset()
{
    if (status_ == PLAYER_IDLE) {
        MEDIA_LOGI("currentState is %{public}d", status_);
        return ERR_OK;
    }
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, ERR_INVALID_OPERATION, "playerEngine_ is nullptr");
    int32_t ret = playerEngine_->Reset();
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, ERR_INVALID_OPERATION, "Engine Reset Failed!");
    playerEngine_ = nullptr;
    OnStateChanged(PLAYER_IDLE);
    stopTimeMonitor_.FinishTime();
    FinishTrace(BYTRACE_TAG_ZMEDIA, STOP_TAG);
    return ret;
}

int32_t PlayerServer::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    {
        std::lock_guard<std::mutex> lockCb(mutexCb_);
        playerCb_ = nullptr;
    }
    (void)OnReset();
    return ERR_OK;
}

int32_t PlayerServer::SetVolume(float leftVolume, float rightVolume)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, ERR_INVALID_OPERATION, "playerEngine_ is nullptr");

    if (status_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not SetVolume, currentState is %{public}d", status_);
        return ERR_INVALID_OPERATION;
    }

    const float max_volume = 1.0f;
    if ((leftVolume < 0) || (leftVolume > max_volume) || (rightVolume < 0) || (rightVolume > max_volume)) {
        MEDIA_LOGE("SetVolume failed, the volume should be set to a value ranging from 0 to 5");
        return ERR_INVALID_OPERATION;
    }

    int32_t ret = playerEngine_->SetVolume(leftVolume, rightVolume);
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, ERR_INVALID_OPERATION, "Engine SetVolume Failed!");
    return ret;
}

bool PlayerServer::IsValidSeekMode(PlayerSeekMode mode)
{
    switch (mode) {
        case SEEK_PREVIOUS_SYNC:
        case SEEK_NEXT_SYNC:
        case SEEK_CLOSEST_SYNC:
        case SEEK_CLOSEST:
            break;
        default:
            MEDIA_LOGE("Unknown seek mode %{public}d", mode);
            return false;
    }
    return true;
}

int32_t PlayerServer::Seek(uint64_t mSeconds, int32_t mode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, ERR_INVALID_OPERATION, "playerEngine_ is nullptr");

    if (status_ != PLAYER_PREPARED && status_ != PLAYER_PAUSED &&
        status_ != PLAYER_STARTED && status_ != PLAYER_PLAYBACK_COMPLETE) {
            MEDIA_LOGE("Can not Seek, currentState is %{public}d", status_);
            return ERR_INVALID_OPERATION;
        }

    if (IsValidSeekMode(static_cast<PlayerSeekMode>(mode)) != true) {
        MEDIA_LOGE("Seek failed, inValidMode");
        return ERR_INVALID_OPERATION;
    }

    int32_t ret = playerEngine_->Seek(mSeconds, mode);
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, ERR_INVALID_OPERATION, "Engine Seek Failed!");
    return ret;
}

int32_t PlayerServer::GetCurrentTime(uint64_t &currentTime)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, ERR_INVALID_OPERATION, "playerEngine_ is nullptr");

    if (status_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not GetCurrentTime, currentState is %{public}d", status_);
        return ERR_INVALID_OPERATION;
    }

    MEDIA_LOGI("PlayerServer::GetCurrentTime");
    int32_t ret = playerEngine_->GetCurrentTime(currentTime);
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, ERR_INVALID_OPERATION, "Engine GetCurrentTime Failed!");
    return ERR_OK;
}

int32_t PlayerServer::GetDuration(uint64_t &duration)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, ERR_INVALID_OPERATION, "playerEngine_ is nullptr");

    if (status_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not GetDuration, currentState is %{public}d", status_);
        return ERR_INVALID_OPERATION;
    }

    if (status_ == PLAYER_IDLE || status_ == PLAYER_INITIALIZED) {
        MEDIA_LOGE("Can not GetDuration, currentState is %{public}d", status_);
        return ERR_INVALID_OPERATION;
    }

    int ret = playerEngine_->GetDuration(duration);
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, ERR_INVALID_OPERATION, "Engine GetDuration Failed!");
    return ERR_OK;
}

int32_t PlayerServer::SetPlaybackSpeed(int32_t mode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, ERR_NO_INIT, "playerEngine_ is nullptr");

    if ((status_ != PLAYER_STARTED) && (status_ != PLAYER_PREPARED) &&
        (status_ != PLAYER_PAUSED) && (status_ != PLAYER_PLAYBACK_COMPLETE)) {
        MEDIA_LOGE("Can not SetPlaybackSpeed, currentState is %{public}d", status_);
        return ERR_INVALID_OPERATION;
    }

    int ret = playerEngine_->SetPlaybackSpeed(mode);
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, ERR_INVALID_OPERATION, "Engine SetPlaybackSpeed Failed!");
    return ERR_OK;
}

int32_t PlayerServer::GetPlaybackSpeed(int32_t &mode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, ERR_NO_INIT, "playerEngine_ is nullptr");

    if (status_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not GetDuration, currentState is %{public}d", status_);
        return ERR_INVALID_OPERATION;
    }

    int ret = playerEngine_->GetPlaybackSpeed(mode);
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, ERR_INVALID_OPERATION, "Engine GetPlaybackSpeed Failed!");
    return ERR_OK;
}

int32_t PlayerServer::SetVideoSurface(sptr<Surface> surface)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(surface != nullptr, ERR_INVALID_VALUE, "surface is nullptr");

    if (status_ != PLAYER_INITIALIZED) {
        MEDIA_LOGE("current state: %{public}d, can not SetVideoSurface", status_);
        return ERR_INVALID_OPERATION;
    }

    surface_ = surface;
    return ERR_OK;
}

bool PlayerServer::IsPlaying()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (status_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not judge IsPlaying, currentState is %{public}d", status_);
        return ERR_INVALID_OPERATION;
    }

    return status_ == PLAYER_STARTED;
}

bool PlayerServer::IsLooping()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (status_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not judge IsLooping, currentState is %{public}d", status_);
        return ERR_INVALID_OPERATION;
    }

    return looping_;
}

int32_t PlayerServer::SetLooping(bool loop)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, ERR_INVALID_OPERATION, "playerEngine_ is nullptr");

    if (status_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not SetLooping, currentState is %{public}d", status_);
        return ERR_INVALID_OPERATION;
    }
    looping_ = loop;

    int32_t ret = playerEngine_->SetLooping(loop);
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, ERR_INVALID_OPERATION, "Engine SetLooping Failed!");
    return ret;
}

int32_t PlayerServer::SetPlayerCallback(const std::shared_ptr<PlayerCallback> &callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, ERR_INVALID_VALUE, "callback is nullptr");

    if (status_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not SetPlayerCallback, currentState is %{public}d", status_);
        return ERR_INVALID_OPERATION;
    }

    if (status_ == PLAYER_IDLE || status_ == PLAYER_STOPPED) {
        MEDIA_LOGE("Can not SetPlayerCallback, currentState is %{public}d", status_);
        return ERR_INVALID_OPERATION;
    }

    {
        std::lock_guard<std::mutex> lockCb(mutexCb_);
        playerCb_ = callback;
    }
    return ERR_OK;
}

void PlayerServer::OnError(int32_t errorType, int32_t errorCode)
{
    std::lock_guard<std::mutex> lockCb(mutexCb_);
    if (playerCb_ != nullptr) {
        auto cbTask = std::make_shared<TaskHandler>([this, errorType, errorCode] {
            playerCb_->OnError(errorType, errorCode);
            return ERR_OK;
        });
        (void)cbLoop_.EnqueueTask(cbTask);
    }
}

void PlayerServer::OnSeekDone(uint64_t currentPositon)
{
    std::lock_guard<std::mutex> lockCb(mutexCb_);
    if (playerCb_ != nullptr) {
        auto cbTask = std::make_shared<TaskHandler>([this, currentPositon] {
            playerCb_->OnSeekDone(currentPositon);
            return ERR_OK;
        });
        (void)cbLoop_.EnqueueTask(cbTask);
    }
}

void PlayerServer::OnEndOfStream(bool isLooping)
{
    std::lock_guard<std::mutex> lockCb(mutexCb_);
    if (playerCb_ != nullptr) {
        auto cbTask = std::make_shared<TaskHandler>([this, isLooping] {
            playerCb_->OnEndOfStream(isLooping);
            return ERR_OK;
        });
        (void)cbLoop_.EnqueueTask(cbTask);
    }
}

void PlayerServer::OnStateChanged(PlayerStates state)
{
    status_ = state;
    MEDIA_LOGD("OnStateChanged Callback, currentState is %{public}d", status_);
    std::lock_guard<std::mutex> lockCb(mutexCb_);
    if (playerCb_ != nullptr) {
        auto cbTask = std::make_shared<TaskHandler>([this, state] {
            playerCb_->OnStateChanged(state);
            return ERR_OK;
        });
        (void)cbLoop_.EnqueueTask(cbTask);
    }
}

void PlayerServer::OnPositionUpdated(uint64_t position)
{
    std::lock_guard<std::mutex> lockCb(mutexCb_);
    if (playerCb_ != nullptr) {
        auto cbTask = std::make_shared<TaskHandler>([this, position] {
            playerCb_->OnPositionUpdated(position);
            return ERR_OK;
        });
        (void)cbLoop_.EnqueueTask(cbTask);
    }
}

void PlayerServer::OnMessage(int32_t type, int32_t extra)
{
    std::lock_guard<std::mutex> lockCb(mutexCb_);
    if (playerCb_ != nullptr) {
        auto cbTask = std::make_shared<TaskHandler>([this, type, extra] {
            playerCb_->OnMessage(type, extra);
            return ERR_OK;
        });
        (void)cbLoop_.EnqueueTask(cbTask);
    }
}
} // Media
} // OHOS