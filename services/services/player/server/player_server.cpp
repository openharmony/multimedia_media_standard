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
#include <map>
#include "media_log.h"
#include "media_errors.h"
#include "engine_factory_repo.h"
#include "player_server_state.h"
#include "media_dfx.h"
#include "ipc_skeleton.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerServer"};
    enum PlayerTaskID : int32_t {
        PREPARE_TASK_ID = 0,
        PLAY_TASK_ID = 1,
        PAUSE_TASK_ID = 2,
        STOP_TASK_ID = 3,
        SEEK_TASK_ID = 4,
    };
}

namespace OHOS {
namespace Media {
const std::string START_TAG = "PlayerCreate->Start";
const std::string STOP_TAG = "PlayerStop->Destroy";
static const std::unordered_map<int32_t, std::string> STATUS_TO_STATUS_DESCRIPTION_TABLE = {
    {PLAYER_STATE_ERROR, "PLAYER_STATE_ERROR"},
    {PLAYER_IDLE, "PLAYER_IDLE"},
    {PLAYER_INITIALIZED, "PLAYER_INITIALIZED"},
    {PLAYER_PREPARING, "PLAYER_PREPARING"},
    {PLAYER_PREPARED, "PLAYER_PREPARED"},
    {PLAYER_STARTED, "PLAYER_STARTED"},
    {PLAYER_PAUSED, "PLAYER_PAUSED"},
    {PLAYER_STOPPED, "PLAYER_STOPPED"},
    {PLAYER_PLAYBACK_COMPLETE, "PLAYER_PLAYBACK_COMPLETE"},
};

std::shared_ptr<IPlayerService> PlayerServer::Create()
{
    std::shared_ptr<PlayerServer> server = std::make_shared<PlayerServer>();
    CHECK_AND_RETURN_RET_LOG(server != nullptr, nullptr, "failed to new PlayerServer");

    (void)server->Init();
    return server;
}

PlayerServer::PlayerServer()
    : startTimeMonitor_(START_TAG),
      stopTimeMonitor_(STOP_TAG)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayerServer::~PlayerServer()
{
    (void)Release();
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t PlayerServer::Init()
{
    MediaTrace trace("PlayerServer::Init");

    idleState_ = std::make_shared<IdleState>(*this);
    initializedState_ = std::make_shared<InitializedState>(*this);
    preparingState_ = std::make_shared<PreparingState>(*this);
    preparedState_ = std::make_shared<PreparedState>(*this);
    playingState_ = std::make_shared<PlayingState>(*this);
    pausedState_ = std::make_shared<PausedState>(*this);
    stoppedState_ = std::make_shared<StoppedState>(*this);
    playbackCompletedState_ = std::make_shared<PlaybackCompletedState>(*this);
    return MSERR_OK;
}

int32_t PlayerServer::SetSource(const std::string &url)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("PlayerServer::SetSource");
    MEDIA_LOGW("KPI-TRACE: PlayerServer SetSource in(url)");
    config_.url = url;
    int32_t ret = InitPlayEngine(url);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetSource Failed!");
    return ret;
}

int32_t PlayerServer::SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("PlayerServer::SetSource");
    CHECK_AND_RETURN_RET_LOG(dataSrc != nullptr, MSERR_INVALID_VAL, "data source is nullptr");
    MEDIA_LOGW("KPI-TRACE: PlayerServer SetSource in(dataSrc)");
    dataSrc_ = dataSrc;
    std::string url = "media data source";
    config_.url = url;
    int32_t ret = InitPlayEngine(url);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "InitPlayEngine Failed!");
    int64_t size = 0;
    (void)dataSrc_->GetSize(size);
    if (size == -1) {
        config_.looping = false;
        config_.speedMode = SPEED_FORWARD_1_00_X;
    }
    return ret;
}

int32_t PlayerServer::SetSource(int32_t fd, int64_t offset, int64_t size)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("PlayerServer::SetSource");
    MEDIA_LOGW("KPI-TRACE: PlayerServer SetSource in(fd)");
    auto uriHelper = std::make_unique<UriHelper>(fd, offset, size);
    CHECK_AND_RETURN_RET_LOG(uriHelper->AccessCheck(UriHelper::URI_READ), MSERR_INVALID_VAL, "Failed to read the fd");
    int32_t ret = InitPlayEngine(uriHelper->FormattedUri());
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetSource Failed!");
    uriHelper_ = std::move(uriHelper);
    config_.url = "file descriptor source";
    return ret;
}

int32_t PlayerServer::InitPlayEngine(const std::string &url)
{
    if (lastOpStatus_ != PLAYER_IDLE) {
        MEDIA_LOGE("current state is: %{public}s, not support SetSource", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }
    startTimeMonitor_.StartTime();
    int32_t ret = taskMgr_.Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "task mgr init failed");
    MEDIA_LOGD("current url is : %{public}s", url.c_str());
    auto engineFactory = EngineFactoryRepo::Instance().GetEngineFactory(IEngineFactory::Scene::SCENE_PLAYBACK, url);
    CHECK_AND_RETURN_RET_LOG(engineFactory != nullptr, MSERR_CREATE_PLAYER_ENGINE_FAILED,
        "failed to get engine factory");
    int32_t appUid = IPCSkeleton::GetCallingUid();
    int32_t appPid = IPCSkeleton::GetCallingPid();
    MEDIA_LOGD("Get app uid: %{public}d, app pid: %{public}d", appUid, appPid);
    playerEngine_ = engineFactory->CreatePlayerEngine(appUid, appPid);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_CREATE_PLAYER_ENGINE_FAILED,
        "failed to create player engine");

    if (dataSrc_ == nullptr) {
        ret = playerEngine_->SetSource(url);
    } else {
        ret = playerEngine_->SetSource(dataSrc_);
    }
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetSource Failed!");

    std::shared_ptr<IPlayerEngineObs> obs = shared_from_this();
    ret = playerEngine_->SetObs(obs);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetObs Failed!");

    lastOpStatus_ = PLAYER_INITIALIZED;
    ChangeState(initializedState_);

    return MSERR_OK;
}

int32_t PlayerServer::Prepare()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGW("KPI-TRACE: PlayerServer Prepare in");
    MediaTrace trace("PlayerServer::Prepare");
    int32_t ret = OnPrepare();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    {
        std::unique_lock<std::mutex> condLock(condMutex_);
        stateCond_.wait(condLock, [this]() {
            return GetCurrState() == preparedState_;
        });
    }

    if (GetCurrState() != preparedState_) {
        MEDIA_LOGE("Prepare failed");
        return MSERR_UNKNOWN;
    }

    return MSERR_OK;
}

int32_t PlayerServer::PrepareAsync()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGW("KPI-TRACE: PlayerServer PrepareAsync in");
    MediaTrace::TraceBegin("PlayerServer::PrepareAsync", PREPARE_TASK_ID);
    return OnPrepare();
}

int32_t PlayerServer::OnPrepare()
{
    if (lastOpStatus_ != PLAYER_INITIALIZED && lastOpStatus_ != PLAYER_STOPPED && lastOpStatus_ != PLAYER_PREPARED) {
        MEDIA_LOGE("Can not Prepare, currentState is %{public}s", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }

    if (lastOpStatus_ == PLAYER_PREPARED) {
        Format format;
        OnInfo(INFO_TYPE_STATE_CHANGE, lastOpStatus_, format);
        return MSERR_OK;
    }

    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");
    int32_t ret = MSERR_OK;
    if (surface_ != nullptr) {
        ret = playerEngine_->SetVideoSurface(surface_);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Engine SetVideoSurface Failed!");
    }

    lastOpStatus_ = PLAYER_PREPARED;

    auto preparedTask = std::make_shared<TaskHandler<int32_t>>([this]() {
        auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
        return currState->Prepare();
    });

    ret = taskMgr_.LaunchTask(preparedTask, PlayerServerTaskType::STATE_CHANGE);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Prepare launch task failed");

    return MSERR_OK;
}

int32_t PlayerServer::HandlePrepare()
{
    int32_t ret = playerEngine_->PrepareAsync();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Server Prepare Failed!");
    (void)playerEngine_->SetVolume(config_.leftVolume, config_.rightVolume);
    (void)playerEngine_->SetLooping(config_.looping);
    if (config_.speedMode != SPEED_FORWARD_1_00_X) {
        (void)playerEngine_->SetPlaybackSpeed(config_.speedMode);
    }

    return MSERR_OK;
}

int32_t PlayerServer::Play()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace::TraceBegin("PlayerServer::Play", PLAY_TASK_ID);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");
    MEDIA_LOGW("KPI-TRACE: PlayerServer Play in");
    if (lastOpStatus_ != PLAYER_PREPARED && lastOpStatus_ != PLAYER_PLAYBACK_COMPLETE &&
        lastOpStatus_ != PLAYER_PAUSED && lastOpStatus_ != PLAYER_STARTED) {
        MEDIA_LOGE("Can not Play, currentState is %{public}s", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }

    if (lastOpStatus_ == PLAYER_PLAYBACK_COMPLETE && dataSrc_ != nullptr) {
        int64_t size = 0;
        (void)dataSrc_->GetSize(size);
        if (size == -1) {
            MEDIA_LOGE("Can not play in complete status, it is live-stream");
            return MSERR_INVALID_OPERATION;
        }
    }

    if (lastOpStatus_ == PLAYER_STARTED) {
        Format format;
        OnInfo(INFO_TYPE_STATE_CHANGE, lastOpStatus_, format);
        return MSERR_OK;
    }

    auto playingTask = std::make_shared<TaskHandler<void>>([this]() {
        auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
        (void)currState->Play();
    });

    int ret = taskMgr_.LaunchTask(playingTask, PlayerServerTaskType::STATE_CHANGE);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Play failed");

    startTimeMonitor_.FinishTime();
    lastOpStatus_ = PLAYER_STARTED;
    return MSERR_OK;
}

int32_t PlayerServer::HandlePlay()
{
    int32_t ret = playerEngine_->Play();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Engine Play Failed!");

    return MSERR_OK;
}

int32_t PlayerServer::Pause()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace::TraceBegin("PlayerServer::Pause", PAUSE_TASK_ID);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");

    if (lastOpStatus_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not Pause, currentState is PLAYER_STATE_ERROR");
        return MSERR_INVALID_OPERATION;
    }

    if (lastOpStatus_ == PLAYER_PAUSED) {
        Format format;
        OnInfo(INFO_TYPE_STATE_CHANGE, lastOpStatus_, format);
        return MSERR_OK;
    }

    if (lastOpStatus_ != PLAYER_STARTED) {
        MEDIA_LOGE("Can not Pause, lastOpStatus_ is %{public}s", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }

    auto pauseTask = std::make_shared<TaskHandler<void>>([this]() {
        auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
        (void)currState->Pause();
    });

    int ret = taskMgr_.LaunchTask(pauseTask, PlayerServerTaskType::STATE_CHANGE);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Pause failed");

    lastOpStatus_ = PLAYER_PAUSED;
    return MSERR_OK;
}

int32_t PlayerServer::HandlePause()
{
    int32_t ret = playerEngine_->Pause();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Engine Pause Failed!");

    return MSERR_OK;
}

int32_t PlayerServer::Stop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace::TraceBegin("PlayerServer::Stop", STOP_TASK_ID);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");
    MEDIA_LOGW("KPI-TRACE: PlayerServer Stop in");
    if (lastOpStatus_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not Stop, currentState is PLAYER_STATE_ERROR");
        return MSERR_INVALID_OPERATION;
    }

    if (lastOpStatus_ == PLAYER_STOPPED) {
        Format format;
        OnInfo(INFO_TYPE_STATE_CHANGE, lastOpStatus_, format);
        return MSERR_OK;
    }

    if ((lastOpStatus_ != PLAYER_PREPARED) && (lastOpStatus_ != PLAYER_STARTED) &&
        (lastOpStatus_ != PLAYER_PLAYBACK_COMPLETE) && (lastOpStatus_ != PLAYER_PAUSED)) {
        MEDIA_LOGE("current state: %{public}s, can not stop", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }

    taskMgr_.ClearAllTask();

    auto stopTask = std::make_shared<TaskHandler<void>>([this]() {
        auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
        (void)currState->Stop();
    });

    int ret = taskMgr_.LaunchTask(stopTask, PlayerServerTaskType::STATE_CHANGE);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Stop failed");

    lastOpStatus_ = PLAYER_STOPPED;
    return MSERR_OK;
}

int32_t PlayerServer::HandleStop()
{
    stopTimeMonitor_.StartTime();

    int32_t ret = playerEngine_->Stop();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Engine Stop Failed!");

    return MSERR_OK;
}

int32_t PlayerServer::Reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGW("KPI-TRACE: PlayerServer Reset in");
    MediaTrace trace("PlayerServer::Reset");
    return OnReset();
}

int32_t PlayerServer::OnReset()
{
    if (lastOpStatus_ == PLAYER_IDLE) {
        Format format;
        OnInfo(INFO_TYPE_STATE_CHANGE, lastOpStatus_, format);
        return MSERR_OK;
    }

    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");

    taskMgr_.ClearAllTask();
    auto idleTask = std::make_shared<TaskHandler<void>>([this]() { ChangeState(idleState_); });
    (void)taskMgr_.LaunchTask(idleTask, PlayerServerTaskType::STATE_CHANGE);
    (void)idleTask->GetResult();
    (void)taskMgr_.Reset();
    lastOpStatus_ = PLAYER_IDLE;

    return MSERR_OK;
}

int32_t PlayerServer::HandleReset()
{
    int32_t ret = playerEngine_->Reset();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Engine Reset Failed!");
    playerEngine_ = nullptr;
    dataSrc_ = nullptr;
    config_.looping = false;
    uriHelper_ = nullptr;
    lastErrMsg_.clear();
    Format format;
    OnInfo(INFO_TYPE_STATE_CHANGE, PLAYER_IDLE, format);
    stopTimeMonitor_.FinishTime();
    return MSERR_OK;
}

int32_t PlayerServer::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    {
        std::lock_guard<std::mutex> lockCb(mutexCb_);
        playerCb_ = nullptr;
    }
    (void)OnReset();
    return MSERR_OK;
}

int32_t PlayerServer::SetVolume(float leftVolume, float rightVolume)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (lastOpStatus_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not SetVolume, currentState is PLAYER_STATE_ERROR");
        return MSERR_INVALID_OPERATION;
    }

    constexpr float maxVolume = 1.0f;
    if ((leftVolume < 0) || (leftVolume > maxVolume) || (rightVolume < 0) || (rightVolume > maxVolume)) {
        MEDIA_LOGE("SetVolume failed, the volume should be set to a value ranging from 0 to 5");
        return MSERR_INVALID_OPERATION;
    }

    config_.leftVolume = leftVolume;
    config_.rightVolume = rightVolume;
    if (lastOpStatus_ == PLAYER_IDLE || lastOpStatus_ == PLAYER_INITIALIZED || lastOpStatus_ == PLAYER_STOPPED) {
        MEDIA_LOGI("Waiting for the engine state is <prepared> to take effect");
        Format format;
        OnInfo(INFO_TYPE_VOLUME_CHANGE, 0, format);
        return MSERR_OK;
    }

    if (playerEngine_ != nullptr) {
        int32_t ret = playerEngine_->SetVolume(config_.leftVolume, config_.rightVolume);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetVolume Failed!");
    }
    return MSERR_OK;
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

int32_t PlayerServer::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace::TraceBegin("Player::Seek", SEEK_TASK_ID);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");

    if (lastOpStatus_ != PLAYER_PREPARED && lastOpStatus_ != PLAYER_PAUSED && lastOpStatus_ != PLAYER_STARTED) {
        MEDIA_LOGE("Can not Seek, currentState is %{public}s", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }

    if (IsValidSeekMode(mode) != true) {
        MEDIA_LOGE("Seek failed, inValid mode");
        return MSERR_INVALID_VAL;
    }

    MEDIA_LOGD("seek position %{public}d, seek mode is %{public}d", mSeconds, mode);
    mSeconds = std::max(0, mSeconds);

    auto seekTask = std::make_shared<TaskHandler<void>>([this, mSeconds, mode]() {
        auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
        (void)currState->Seek(mSeconds, mode);
    });

    int ret = taskMgr_.LaunchTask(seekTask, PlayerServerTaskType::SEEKING);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Seek failed");

    return MSERR_OK;
}

int32_t PlayerServer::HandleSeek(int32_t mSeconds, PlayerSeekMode mode)
{
    int32_t ret = playerEngine_->Seek(mSeconds, mode);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Engine Seek Failed!");

    return MSERR_OK;
}

int32_t PlayerServer::GetCurrentTime(int32_t &currentTime)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (lastOpStatus_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not GetCurrentTime, currentState is PLAYER_STATE_ERROR");
        return MSERR_INVALID_OPERATION;
    }

    MEDIA_LOGI("PlayerServer::GetCurrentTime");
    currentTime = 0;
    if (playerEngine_ != nullptr) {
        int32_t ret = playerEngine_->GetCurrentTime(currentTime);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Engine GetCurrentTime Failed!");
    }
    return MSERR_OK;
}

int32_t PlayerServer::GetVideoTrackInfo(std::vector<Format> &videoTrack)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");

    if (lastOpStatus_ != PLAYER_PREPARED && lastOpStatus_ != PLAYER_PAUSED &&
        lastOpStatus_ != PLAYER_STARTED && lastOpStatus_ != PLAYER_STOPPED &&
        lastOpStatus_ != PLAYER_PLAYBACK_COMPLETE) {
        MEDIA_LOGE("Can not get track info, currentState is %{public}s", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }

    int32_t ret = playerEngine_->GetVideoTrackInfo(videoTrack);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Engine GetVideoTrackInfo Failed!");
    return MSERR_OK;
}

int32_t PlayerServer::GetAudioTrackInfo(std::vector<Format> &audioTrack)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");

    if (lastOpStatus_ != PLAYER_PREPARED && lastOpStatus_ != PLAYER_PAUSED &&
        lastOpStatus_ != PLAYER_STARTED && lastOpStatus_ != PLAYER_STOPPED &&
        lastOpStatus_ != PLAYER_PLAYBACK_COMPLETE) {
        MEDIA_LOGE("Can not get track info, currentState is %{public}s", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }

    int32_t ret = playerEngine_->GetAudioTrackInfo(audioTrack);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Engine GetAudioTrackInfo Failed!");
    return MSERR_OK;
}

int32_t PlayerServer::GetVideoWidth()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");

    if (lastOpStatus_ != PLAYER_PREPARED && lastOpStatus_ != PLAYER_PAUSED &&
        lastOpStatus_ != PLAYER_STARTED && lastOpStatus_ != PLAYER_STOPPED &&
        lastOpStatus_ != PLAYER_PLAYBACK_COMPLETE) {
        MEDIA_LOGE("Can not get track info, currentState is %{public}s", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }

    return playerEngine_->GetVideoWidth();
}

int32_t PlayerServer::GetVideoHeight()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");

    if (lastOpStatus_ != PLAYER_PREPARED && lastOpStatus_ != PLAYER_PAUSED &&
        lastOpStatus_ != PLAYER_STARTED && lastOpStatus_ != PLAYER_STOPPED &&
        lastOpStatus_ != PLAYER_PLAYBACK_COMPLETE) {
        MEDIA_LOGE("Can not get track info, currentState is %{public}s", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }

    return playerEngine_->GetVideoHeight();
}

int32_t PlayerServer::GetDuration(int32_t &duration)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (lastOpStatus_ == PLAYER_IDLE || lastOpStatus_ == PLAYER_INITIALIZED || lastOpStatus_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not GetDuration, currentState is %{public}s", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }
    duration = 0;
    if (playerEngine_ != nullptr) {
        int ret = playerEngine_->GetDuration(duration);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Engine GetDuration Failed!");
    }
    return MSERR_OK;
}

int32_t PlayerServer::SetPlaybackSpeed(PlaybackRateMode mode)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if ((lastOpStatus_ != PLAYER_STARTED) && (lastOpStatus_ != PLAYER_PREPARED) &&
        (lastOpStatus_ != PLAYER_PAUSED) && (lastOpStatus_ != PLAYER_PLAYBACK_COMPLETE)) {
        MEDIA_LOGE("Can not SetPlaybackSpeed, currentState is %{public}s",
            GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }

    if (dataSrc_ != nullptr) {
        int64_t size = 0;
        (void)dataSrc_->GetSize(size);
        if (size == -1) {
            MEDIA_LOGE("Can not SetPlaybackSpeed, it is live-stream");
            return MSERR_INVALID_OPERATION;
        }
    }

    if (config_.speedMode == mode) {
        MEDIA_LOGD("The speed mode is same, mode = %{public}d", mode);
        Format format;
        OnInfo(INFO_TYPE_SPEEDDONE, 0, format);
        return MSERR_OK;
    }

    auto rateTask = std::make_shared<TaskHandler<void>>([this, mode]() {
        auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
        (void)currState->SetPlaybackSpeed(mode);
    });

    int ret = taskMgr_.LaunchTask(rateTask, PlayerServerTaskType::RATE_CHANGE);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "SetPlaybackSpeed failed");

    return MSERR_OK;
}

int32_t PlayerServer::HandleSetPlaybackSpeed(PlaybackRateMode mode)
{
    if (playerEngine_ != nullptr) {
        int ret = playerEngine_->SetPlaybackSpeed(mode);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Engine SetPlaybackSpeed Failed!");
    }
    config_.speedMode = mode;
    return MSERR_OK;
}

int32_t PlayerServer::GetPlaybackSpeed(PlaybackRateMode &mode)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (lastOpStatus_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not GetDuration, currentState is PLAYER_STATE_ERROR");
        return MSERR_INVALID_OPERATION;
    }

    mode = config_.speedMode;
    return MSERR_OK;
}

int32_t PlayerServer::SelectBitRate(uint32_t bitRate)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (playerEngine_ != nullptr) {
        int ret = playerEngine_->SelectBitRate(bitRate);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Engine SelectBitRate Failed!");
    }
    return MSERR_OK;
}

int32_t PlayerServer::SetVideoSurface(sptr<Surface> surface)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(surface != nullptr, MSERR_INVALID_VAL, "surface is nullptr");

    if (lastOpStatus_ != PLAYER_INITIALIZED) {
        MEDIA_LOGE("current state: %{public}s, can not SetVideoSurface", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }

    surface_ = surface;
    return MSERR_OK;
}

bool PlayerServer::IsPlaying()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (lastOpStatus_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not judge IsPlaying, currentState is PLAYER_STATE_ERROR");
        return false;
    }

    return lastOpStatus_ == PLAYER_STARTED;
}

bool PlayerServer::IsLooping()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (lastOpStatus_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not judge IsLooping, currentState is PLAYER_STATE_ERROR");
        return false;
    }

    return config_.looping;
}

int32_t PlayerServer::SetLooping(bool loop)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (lastOpStatus_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not SetLooping, currentState is PLAYER_STATE_ERROR");
        return MSERR_INVALID_OPERATION;
    }

    if (dataSrc_ != nullptr) {
        int64_t size = 0;
        (void)dataSrc_->GetSize(size);
        if (size == -1) {
            MEDIA_LOGE("Can not SetLooping, it is live-stream");
            return MSERR_INVALID_OPERATION;
        }
    }

    if (lastOpStatus_ == PLAYER_IDLE || lastOpStatus_ == PLAYER_INITIALIZED) {
        MEDIA_LOGI("Waiting for the engine state is <prepared> to take effect");
        config_.looping = loop;
        return MSERR_OK;
    }

    if (playerEngine_ != nullptr) {
        int32_t ret = playerEngine_->SetLooping(loop);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetLooping Failed!");
    }
    config_.looping = loop;
    return MSERR_OK;
}

int32_t PlayerServer::SetParameter(const Format &param)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (lastOpStatus_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not SetParameter, currentState is PLAYER_STATE_ERROR");
        return MSERR_INVALID_OPERATION;
    }

    if (playerEngine_ != nullptr) {
        int32_t ret = playerEngine_->SetParameter(param);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetParameter Failed!");
    } else {
        if (param.ContainKey(PlayerKeys::CONTENT_TYPE) && param.ContainKey(PlayerKeys::STREAM_USAGE)) {
            param.GetIntValue(PlayerKeys::CONTENT_TYPE, contentType_);
            param.GetIntValue(PlayerKeys::STREAM_USAGE, streamUsage_);
            param.GetIntValue(PlayerKeys::RENDERER_FLAG, rendererFlag_);
        }
    }

    return MSERR_OK;
}

int32_t PlayerServer::SetPlayerCallback(const std::shared_ptr<PlayerCallback> &callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_INVALID_VAL, "callback is nullptr");

    if (lastOpStatus_ != PLAYER_IDLE && lastOpStatus_ != PLAYER_INITIALIZED) {
        MEDIA_LOGE("Can not SetPlayerCallback, currentState is %{public}s",
            GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }

    {
        std::lock_guard<std::mutex> lockCb(mutexCb_);
        playerCb_ = callback;
    }
    return MSERR_OK;
}

void PlayerServer::FormatToString(std::string &dumpString, std::vector<Format> &videoTrack)
{
    for (auto iter = videoTrack.begin(); iter != videoTrack.end(); iter++) {
        dumpString += iter->Stringify();
    }
}

int32_t PlayerServer::DumpInfo(int32_t fd)
{
    std::string dumpString;
    if (playerEngine_ == nullptr) {
        dumpString +=
            "The engine is not created, note: engine can't be created until set source.\n";
            write(fd, dumpString.c_str(), dumpString.size());
        return MSERR_OK;
    }
    dumpString += "PlayerServer current state is: " + std::to_string(lastOpStatus_) + "\n";
    if (lastErrMsg_.size() != 0) {
        dumpString += "PlayerServer last error is: " + lastErrMsg_ + "\n";
    }
    dumpString += "PlayerServer url is: " + config_.url + "\n";
    dumpString += "PlayerServer play back speed is: " + std::to_string(config_.speedMode) + "\n";
    std::string loopflag = config_.looping ? "" : "not ";
    dumpString += "PlayerServer current " + loopflag + "in looping mode\n";
    dumpString += "PlayerServer left volume and right volume is: " +
        std::to_string(config_.leftVolume) + ", " + std::to_string(config_.rightVolume) + "\n";

    std::vector<Format> videoTrack;
    CHECK_AND_RETURN_RET(GetVideoTrackInfo(videoTrack) == MSERR_OK, MSERR_INVALID_OPERATION);
    dumpString += "PlayerServer video tracks info: \n";
    FormatToString(dumpString, videoTrack);
    
    std::vector<Format> audioTrack;
    CHECK_AND_RETURN_RET(GetAudioTrackInfo(audioTrack) == MSERR_OK, MSERR_INVALID_OPERATION);
    dumpString += "PlayerServer audio tracks info: \n";
    FormatToString(dumpString, audioTrack);
    
    int32_t currentTime;
    CHECK_AND_RETURN_RET(GetCurrentTime(currentTime) == MSERR_OK, MSERR_INVALID_OPERATION);
    dumpString += "PlayerServer current time is: " + std::to_string(currentTime) + "\n";
    write(fd, dumpString.c_str(), dumpString.size());

    return MSERR_OK;
}

void PlayerServer::OnError(PlayerErrorType errorType, int32_t errorCode)
{
    std::lock_guard<std::mutex> lockCb(mutexCb_);
    lastErrMsg_ = MSErrorToExtErrorString(static_cast<MediaServiceErrCode>(errorCode));
    FaultEventWrite(lastErrMsg_, "Player");
    if (playerCb_ != nullptr) {
        playerCb_->OnError(errorType, errorCode);
    }
}

void PlayerServer::OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    std::lock_guard<std::mutex> lockCb(mutexCb_);

    HandleMessage(type, extra, infoBody);

    if (playerCb_ != nullptr) {
        playerCb_->OnInfo(type, extra, infoBody);
    }
}

const std::string &PlayerServer::GetStatusDescription(int32_t status)
{
    static const std::string ILLEGAL_STATE = "PLAYER_STATUS_ILLEGAL";
    if (status < PLAYER_STATE_ERROR || status > PLAYER_PLAYBACK_COMPLETE) {
        return ILLEGAL_STATE;
    }

    return STATUS_TO_STATUS_DESCRIPTION_TABLE.find(status)->second;
}

std::string PlayerServerState::GetStateName() const
{
    return name_;
}

void PlayerServerStateMachine::HandleMessage(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    if (currState_ != nullptr) {
        currState_->OnMessageReceived(type, extra, infoBody);
    }
}

void PlayerServerStateMachine::ChangeState(const std::shared_ptr<PlayerServerState> &state)
{
    std::unique_lock<std::recursive_mutex> lock(recMutex_);

    if (state == nullptr || (state == currState_)) {
        return;
    }

    if (currState_) {
        MEDIA_LOGD("exit state %{public}s", currState_->name_.c_str());
        currState_->StateExit();
    }

    MEDIA_LOGI("change state to %{public}s", state->name_.c_str());
    currState_ = state;

    state->StateEnter();
}

std::shared_ptr<PlayerServerState> PlayerServerStateMachine::GetCurrState()
{
    std::unique_lock<std::recursive_mutex> lock(recMutex_);
    return currState_;
}
} // namespace Media
} // namespace OHOS
