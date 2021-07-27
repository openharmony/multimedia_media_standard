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

#include "gst_player_ctrl.h"
#include "media_log.h"
#include "audio_system_manager.h"
#include "media_errors.h"
#include "audio_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "GstPlayerCtrl"};
}

namespace OHOS {
namespace Media {
constexpr int MILLI = 1000;
constexpr int MICRO = MILLI * 1000;
GstPlayerCtrl::GstPlayerCtrl(GstPlayer *gstPlayer)
    : gstPlayer_(gstPlayer)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

GstPlayerCtrl::~GstPlayerCtrl()
{
    g_signal_handlers_disconnect_by_data(gstPlayer_, this);
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t GstPlayerCtrl::SetUri(std::string uri)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(gstPlayer_ != nullptr, ERR_NO_INIT, "gstPlayer_ is nullptr");
    gst_player_set_uri(gstPlayer_, uri.c_str());
    currentState_ = PLAYER_PREPARING;
    return ERR_OK;
}

int32_t GstPlayerCtrl::SetCallbacks(const std::weak_ptr<IPlayerEngineObs> &obs, NotifyCbFunc cb)
{
    CHECK_AND_RETURN_RET_LOG(obs.lock() != nullptr, ERR_INVALID_OPERATION, "obs is nullptr, please set playercallback");
    CHECK_AND_RETURN_RET_LOG(cb != nullptr, ERR_INVALID_OPERATION, "cb is nullptr");

    g_signal_connect(gstPlayer_, "state-changed", G_CALLBACK(OnStateChangedCb), this);
    g_signal_connect(gstPlayer_, "end-of-stream", G_CALLBACK(OnEndOfStreamCb), this);
    g_signal_connect(gstPlayer_, "error", G_CALLBACK(OnErrorCb), this);
    g_signal_connect(gstPlayer_, "seek-done", G_CALLBACK(OnSeekDoneCb), this);
    g_signal_connect(gstPlayer_, "position-updated", G_CALLBACK(OnPositionUpdatedCb), this);

    obs_ = obs;
    syncCb_ = cb;
    currentState_ = PLAYER_PREPARING;
    return ERR_OK;
}

void GstPlayerCtrl::SetVideoTrack(bool enable)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(gstPlayer_ != nullptr, "gstPlayer_ is nullptr");

    gst_player_set_video_track_enabled(gstPlayer_, static_cast<gboolean>(enable));
    MEDIA_LOGI("SetVideoTrack Enabled %{public}d", enable);
}

void GstPlayerCtrl::Pause()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(gstPlayer_ != nullptr, "gstPlayer_ is nullptr");

    gst_player_pause(gstPlayer_);
}

void GstPlayerCtrl::Play()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(gstPlayer_ != nullptr, "gstPlayer_ is nullptr");
    gst_player_play(gstPlayer_);
}

void GstPlayerCtrl::Seek(uint64_t position)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(gstPlayer_ != nullptr, "gstPlayer_ is nullptr");

    if (seekInProgress_) {
        nextSeekFlag_ = true;
        nextSeekPos_ = position;
        MEDIA_LOGI("multiple times seeks!");
    }

    seekInProgress_ = true;
    position = (position > sourceDuration_) ? sourceDuration_ : position;
    GstClockTime time = static_cast<GstClockTime>(position * MICRO);
    gst_player_seek(gstPlayer_, time);
}

void GstPlayerCtrl::Stop()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(gstPlayer_ != nullptr, "gstPlayer_ is nullptr");
    enableLooping_ = false;
    bufferingStart_ = false;
    nextSeekFlag_ = false;
    seekInProgress_ = false;
    nextSeekPos_ = 0;

    if (gstPlayerStopped_) {
        MEDIA_LOGI("gstplayer already stopped!");
        currentState_ = PLAYER_STOPPED;
        OnStateChanged(currentState_);
        return;
    }

    if (currentState_ == PLAYER_STOPPED) {
        return;
    }
    userStop_ = true;
    gst_player_stop(gstPlayer_);
}

void GstPlayerCtrl::SetLoop(bool loop)
{
    std::unique_lock<std::mutex> lock(mutex_);
    enableLooping_ = loop;
}

void GstPlayerCtrl::SetVolume(float leftVolume, float rightVolume)
{
    std::unique_lock<std::mutex> lock(mutex_);
    AudioStandard::AudioSystemManager *audioManager = AudioStandard::AudioSystemManager::GetInstance();
    CHECK_AND_RETURN_LOG(audioManager != nullptr, "audioManager is nullptr");
    int32_t sysVolume = static_cast<int32_t>(leftVolume * 15);
    int32_t ret = audioManager->SetVolume(AudioStandard::AudioSystemManager::AudioVolumeType::STREAM_MUSIC, sysVolume);
    CHECK_AND_RETURN_LOG(ret == AudioStandard::SUCCESS, "set volume fail");
}

uint64_t GstPlayerCtrl::GetPosition()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(gstPlayer_ != nullptr, 0, "gstPlayer_ is nullptr");

    if (stopTimeFlag_ || currentState_ == PLAYER_STOPPED) {
        return 0;
    }

    GstClockTime position = gst_player_get_position(gstPlayer_);
    uint64_t curTime = static_cast<uint64_t>(position) / MICRO;
    curTime = std::min(curTime, sourceDuration_);
    MEDIA_LOGD("GetPosition curTime(%{public}llu) duration(%{public}llu)", curTime, sourceDuration_);
    return curTime;
}

uint64_t GstPlayerCtrl::GetDuration()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(gstPlayer_ != nullptr, 0, "gstPlayer_ is nullptr");

    if (stopTimeFlag_) {
        return sourceDuration_;
    }
    InitDuration();
    return sourceDuration_;
}

void GstPlayerCtrl::SetRate(double rate)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(gstPlayer_ != nullptr, "gstPlayer_ is nullptr");

    MEDIA_LOGD("gst_player_set_rate rate(%{public}lf) in", rate);
    gst_player_set_rate(gstPlayer_, static_cast<gdouble>(rate));
}

double GstPlayerCtrl::GetRate()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(gstPlayer_ != nullptr, 1.0, "gstPlayer_ is nullptr");

    gdouble rate = gst_player_get_rate(gstPlayer_);
    MEDIA_LOGD("gst_player_get_rate rate(%{public}lf) in", rate);
    return static_cast<double>(rate);
}

PlayerStates GstPlayerCtrl::GetState()
{
    return currentState_;
}

void GstPlayerCtrl::OnStateChangedCb(const GstPlayer *player, GstPlayerState state, const GstPlayerCtrl *self)
{
    MEDIA_LOGD("OnStateChangedCb gstplayer State changed: %{public}s", gst_player_state_get_name(state));
    CHECK_AND_RETURN_LOG(player != nullptr, "player is null");
    CHECK_AND_RETURN_LOG(self != nullptr, "self is null");
    auto playerGst = const_cast<GstPlayerCtrl *>(self);
    CHECK_AND_RETURN_LOG(playerGst != nullptr, "playerGst is null");

    PlayerStates newState = PLAYER_IDLE;
    switch (state) {
        case GST_PLAYER_STATE_STOPPED: {
            newState = playerGst->ProcessStoppedState();
            break;
        }
        case GST_PLAYER_STATE_BUFFERING: {
            playerGst->OnMessage(PlayerMessageType::PLAYER_INFO_BUFFERING_START, 0);
            playerGst->bufferingStart_ = true;
            return;
        }
        case GST_PLAYER_STATE_PAUSED: {
            newState = playerGst->ProcessPausedState();
            break;
        }
        case GST_PLAYER_STATE_PLAYING: {
            newState = PLAYER_STARTED;
            playerGst->gstPlayerStopped_ = false;
            playerGst->stopTimeFlag_ = false;
            break;
        }
        default: {
            return;
        }
    }

    if (playerGst->bufferingStart_) {
        playerGst->OnMessage(PlayerMessageType::PLAYER_INFO_BUFFERING_END, 0);
        playerGst->bufferingStart_ = false;
    }

    MEDIA_LOGD("playerGst->currentState_ = %{public}d, newState = %{public}d", playerGst->currentState_, newState);
    if (newState != PLAYER_IDLE && playerGst->currentState_ != newState) {
        playerGst->currentState_ = newState;
        if (newState == PLAYER_STARTED) {
            playerGst->OnMessage(PlayerMessageType::PLAYER_INFO_VIDEO_RENDERING_START, 0);
        }
        playerGst->OnStateChanged(newState);
    }

    /* call back sequence: statecb > seekdonecb */
    playerGst->OnSeekDone();
}

void GstPlayerCtrl::OnEndOfStreamCb(const GstPlayer *player, const GstPlayerCtrl *self)
{
    MEDIA_LOGD("End of stream");
    CHECK_AND_RETURN_LOG(player != nullptr, "player is null");
    CHECK_AND_RETURN_LOG(self != nullptr, "self is null");
    auto playerGst = const_cast<GstPlayerCtrl *>(self);
    CHECK_AND_RETURN_LOG(playerGst != nullptr, "playerGst is null");

    playerGst->currentState_ = PLAYER_PLAYBACK_COMPLETE;
    playerGst->OnStateChanged(playerGst->currentState_);
    if (!playerGst->obs_.expired()) {
        std::shared_ptr<IPlayerEngineObs> tempObs = playerGst->obs_.lock();
        tempObs->OnEndOfStream(playerGst->enableLooping_);
    }

    playerGst->CheckReplay();
}

void GstPlayerCtrl::CheckReplay() const
{
    if (enableLooping_) {
        MEDIA_LOGD("Repeat playback");
        gst_player_seek(gstPlayer_, 0);
    }
}

void GstPlayerCtrl::OnErrorCb(const GstPlayer *player, const GError *err, const GstPlayerCtrl *self)
{
    MEDIA_LOGE("Received error signal from pipeline.");
    CHECK_AND_RETURN_LOG(player != nullptr, "player is null");
    CHECK_AND_RETURN_LOG(err != nullptr, "err is null");
    CHECK_AND_RETURN_LOG(self != nullptr, "self is null");
    auto playerGst = const_cast<GstPlayerCtrl *>(self);
    CHECK_AND_RETURN_LOG(playerGst != nullptr, "playerGst is null");

    // If looping is enabled, then disable it else will keep looping forever
    playerGst->enableLooping_ = false;
    int32_t errorType = PLAYER_ERROR;
    int32_t errorCode = PLAYER_INTERNAL_ERROR;

    if (!playerGst->obs_.expired()) {
        std::shared_ptr<IPlayerEngineObs> tempObs = playerGst->obs_.lock();
        tempObs->OnError(errorType, errorCode);
    }

    playerGst->errorFlag_ = true;
}

void GstPlayerCtrl::OnSeekDoneCb(const GstPlayer *player, guint64 position, const GstPlayerCtrl *self)
{
    auto playerGst = const_cast<GstPlayerCtrl *>(self);
    CHECK_AND_RETURN_LOG(playerGst != nullptr, "playerGst is null");
    playerGst->ProcessSeekDone(player, static_cast<uint64_t>(position) / MICRO);
}

void GstPlayerCtrl::ProcessSeekDone(const GstPlayer *cbPlayer, uint64_t position)
{
    if (cbPlayer != gstPlayer_) {
        MEDIA_LOGE("gstplay cb object error: cbPlayer != gstPlayer_");
        return;
    }
    if (nextSeekFlag_) {
        nextSeekFlag_ = false;
        gst_player_seek(gstPlayer_, (nextSeekPos_ * MICRO));
    } else {
        seekInProgress_ = false;
    }

    seekDoneNeedCb_ = true;
    seekDonePosition_ = position;
    MEDIA_LOGI("gstplay seek Done: %{public}llu", seekDonePosition_);
}

void GstPlayerCtrl::OnPositionUpdatedCb(const GstPlayer *player, guint64 position, const GstPlayerCtrl *self)
{
    auto playerGst = const_cast<GstPlayerCtrl *>(self);
    CHECK_AND_RETURN_LOG(playerGst != nullptr, "playerGst is null");
    playerGst->ProcessPositionUpdated(player, static_cast<uint64_t>(position) / MICRO);
}

void GstPlayerCtrl::ProcessPositionUpdated(const GstPlayer *cbPlayer, uint64_t position)
{
    if (cbPlayer != gstPlayer_) {
        MEDIA_LOGE("gstplay cb object error: cbPlayer != gstPlayer_");
        return;
    }

    position = std::min(position, sourceDuration_);
    std::shared_ptr<IPlayerEngineObs> tempObs = obs_.lock();
    if (tempObs != nullptr) {
        tempObs->OnPositionUpdated(position);
    }
}

void GstPlayerCtrl::OnStateChanged(PlayerStates state)
{
    if (state == PLAYER_PREPARED) {
        InitDuration();
        if (syncCb_ != nullptr) {
            syncCb_();
        }
    }

    MEDIA_LOGI("On State callback state: %{public}d", state);
    std::shared_ptr<IPlayerEngineObs> tempObs = obs_.lock();
    if (tempObs != nullptr) {
        tempObs->OnStateChanged(state);
    }
}

void GstPlayerCtrl::OnSeekDone()
{
    if (seekDoneNeedCb_) {
        MEDIA_LOGI("On Seek Done: %{public}llu", seekDonePosition_);
        std::shared_ptr<IPlayerEngineObs> tempObs = obs_.lock();
        if (tempObs != nullptr) {
            tempObs->OnSeekDone(seekDonePosition_);
        }
        seekDoneNeedCb_ = false;
    }
}

void GstPlayerCtrl::OnMessage(int32_t type, int32_t extra)
{
    MEDIA_LOGI("On Message callback infoType: %{public}d", type);
    if (!obs_.expired()) {
        std::shared_ptr<IPlayerEngineObs> tempObs = obs_.lock();
        tempObs->OnMessage(type, extra);
    }
}

PlayerStates GstPlayerCtrl::ProcessStoppedState()
{
    PlayerStates newState = PLAYER_STOPPED;
    gstPlayerStopped_ = true;
    stopTimeFlag_ = true;
    if (userStop_ || errorFlag_) {
        newState = PLAYER_STOPPED;
        userStop_ = false;
    } else {
        newState = PLAYER_PLAYBACK_COMPLETE;
        stopTimeFlag_ = false;
    }
    return newState;
}

PlayerStates GstPlayerCtrl::ProcessPausedState()
{
    PlayerStates newState = PLAYER_PAUSED;
    MEDIA_LOGI("ProcessPausedState currentStatus: %{public}d", currentState_);
    if ((currentState_ == PLAYER_PREPARING) ||
        (currentState_ == PLAYER_STOPPED) ||
        (currentState_ == PLAYER_PREPARED)) {
        newState = PLAYER_PREPARED;
    } else {
        newState = PLAYER_PAUSED;
    }

    gstPlayerStopped_ = false;
    stopTimeFlag_ = false;
    return newState;
}

void GstPlayerCtrl::InitDuration()
{
    CHECK_AND_RETURN_LOG(gstPlayer_ != nullptr, "gstPlayer_ is nullptr");
    GstClockTime time = gst_player_get_duration(gstPlayer_);
    sourceDuration_ = static_cast<uint64_t>(time) / MICRO;
    MEDIA_LOGD("InitDuration duration(%{public}llu)", sourceDuration_);
}
} // Media
} // OHOS
