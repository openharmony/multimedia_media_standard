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
#include <gst/player/player.h>
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
using StreamToServiceErrFunc = void (*)(const gchar *name, int32_t &errorCode);
static const std::unordered_map<int32_t, StreamToServiceErrFunc> STREAM_TO_SERVICE_ERR_FUNC_TABLE = {
    { GST_STREAM_ERROR_DECODE, GstPlayerCtrl::StreamDecErrorParse },
};
static const std::unordered_map<int32_t, MediaServiceErrCode> STREAM_TO_SERVICE_ERR_TABLE = {
    { GST_STREAM_ERROR_FORMAT, MSERR_UNSUPPORT_CONTAINER_TYPE },
    { GST_STREAM_ERROR_TYPE_NOT_FOUND, MSERR_NOT_FIND_CONTAINER },
    /* Currently, audio decoding and video decoding cannot be distinguished which is not supported by msg.
     * By default, we set video decoding is not supported.
     * The identification method must be added when the new demux pulgin in is use.
     */
    { GST_STREAM_ERROR_CODEC_NOT_FOUND, MSERR_UNSUPPORT_VID_DEC_TYPE },
    { GST_STREAM_ERROR_DEMUX, MSERR_DEMUXER_FAILED },
};

static const std::unordered_map<int32_t, MediaServiceErrCode> RESOURCE_TO_SERVICE_ERR_TABLE = {
    { GST_RESOURCE_ERROR_NOT_FOUND, MSERR_OPEN_FILE_FAILED },
    { GST_RESOURCE_ERROR_OPEN_READ, MSERR_OPEN_FILE_FAILED },
    { GST_RESOURCE_ERROR_READ, MSERR_FILE_ACCESS_FAILED },
    { GST_RESOURCE_ERROR_NOT_AUTHORIZED, MSERR_FILE_ACCESS_FAILED },
};

GstPlayerCtrl::GstPlayerCtrl(GstPlayer *gstPlayer)
    : gstPlayer_(gstPlayer),
      taskQue_("GstCtrlTask")
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
    taskQue_.Start();
}

GstPlayerCtrl::~GstPlayerCtrl()
{
    condVarPlaySync_.notify_all();
    condVarPauseSync_.notify_all();
    condVarStopSync_.notify_all();
    condVarSeekSync_.notify_all();
    taskQue_.Stop();
    for (auto &signalId : signalIds_) {
        g_signal_handler_disconnect(gstPlayer_, signalId);
    }
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

int32_t GstPlayerCtrl::SetCallbacks(const std::weak_ptr<IPlayerEngineObs> &obs)
{
    CHECK_AND_RETURN_RET_LOG(obs.lock() != nullptr, ERR_INVALID_OPERATION, "obs is nullptr, please set playercallback");

    signalIds_.push_back(g_signal_connect(gstPlayer_, "state-changed", G_CALLBACK(OnStateChangedCb), this));
    signalIds_.push_back(g_signal_connect(gstPlayer_, "end-of-stream", G_CALLBACK(OnEndOfStreamCb), this));
    signalIds_.push_back(g_signal_connect(gstPlayer_, "error-msg", G_CALLBACK(OnErrorCb), this));
    signalIds_.push_back(g_signal_connect(gstPlayer_, "seek-done", G_CALLBACK(OnSeekDoneCb), this));
    signalIds_.push_back(g_signal_connect(gstPlayer_, "position-updated", G_CALLBACK(OnPositionUpdatedCb), this));

    obs_ = obs;
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

void GstPlayerCtrl::Pause(bool cancelNotExecuted)
{
    if (cancelNotExecuted) {
        PauseSync();
    } else {
        std::unique_lock<std::mutex> lock(mutex_);
        auto task = std::make_shared<TaskHandler>([this] {
            PauseSync();
            return ERR_OK;
        });
        (void)taskQue_.EnqueueTask(task);
    }
}

void GstPlayerCtrl::PauseSync()
{
    std::unique_lock<std::mutex> lock(mutex_);

    if (currentState_ == PLAYER_PAUSED || currentState_ == PLAYER_PREPARED) {
        return;
    }

    CHECK_AND_RETURN_LOG(gstPlayer_ != nullptr, "gstPlayer_ is nullptr");
    gst_player_pause(gstPlayer_);

    {
        condVarPauseSync_.wait(lock);
        MEDIA_LOGD("Pause finised!");
    }
}

void GstPlayerCtrl::Play()
{
    std::unique_lock<std::mutex> lock(mutex_);
    auto task = std::make_shared<TaskHandler>([this] {
        PlaySync();
        return ERR_OK;
    });
    (void)taskQue_.EnqueueTask(task);
}

void GstPlayerCtrl::PlaySync()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (currentState_ == PLAYER_STARTED) {
        return;
    }

    CHECK_AND_RETURN_LOG(gstPlayer_ != nullptr, "gstPlayer_ is nullptr");
    gst_player_play(gstPlayer_);

    {
        condVarPlaySync_.wait(lock);
        MEDIA_LOGD("Play finised!");
    }
}

int32_t GstPlayerCtrl::ChangeSeekModeToGstFlag(const PlayerSeekMode mode) const
{
    int32_t flag = 0;
    switch (mode) {
        case SEEK_PREVIOUS_SYNC:
            flag = GST_SEEK_FLAG_SNAP_BEFORE | GST_SEEK_FLAG_KEY_UNIT;
            break;
        case SEEK_NEXT_SYNC:
            flag = GST_SEEK_FLAG_SNAP_AFTER | GST_SEEK_FLAG_KEY_UNIT;
            break;
        case SEEK_CLOSEST_SYNC:
            flag = GST_SEEK_FLAG_KEY_UNIT;
            break;
        case SEEK_CLOSEST:
            break;
        default:
            MEDIA_LOGW("unknown seek mode");
            break;
    }
    return flag;
}

void GstPlayerCtrl::Seek(uint64_t position, const PlayerSeekMode mode)
{
    std::unique_lock<std::mutex> lock(mutex_);
    position = (position > sourceDuration_) ? sourceDuration_ : position;
    if (seekInProgress_) {
        nextSeekFlag_ = true;
        nextSeekPos_ = position;
        nextSeekMode_ = mode;
    } else {
        seekInProgress_ = true;
        auto task = std::make_shared<TaskHandler>([this, position, mode] {
            SeekSync(position, mode);
            return ERR_OK;
        });
        (void)taskQue_.EnqueueTask(task);
    }
}

void GstPlayerCtrl::MultipleSeek()
{
    seekInProgress_ = false;
    if (nextSeekFlag_) {
        nextSeekFlag_ = false;
        seekInProgress_ = true;
        auto task = std::make_shared<TaskHandler>([this, position = nextSeekPos_, mode = nextSeekMode_] {
            SeekSync(position, mode);
            return ERR_OK;
        });
        (void)taskQue_.EnqueueTask(task);
    }
}

void GstPlayerCtrl::SeekSync(uint64_t position, const PlayerSeekMode mode)
{
    std::unique_lock<std::mutex> lock(mutex_);

    if (currentState_ == PLAYER_STOPPED) {
        return;
    }

    CHECK_AND_RETURN_LOG(gstPlayer_ != nullptr, "gstPlayer_ is nullptr");
    // need keep the seek and seek modes consistent.
    g_object_set(gstPlayer_, "seek-mode", static_cast<gint>(ChangeSeekModeToGstFlag(mode)), nullptr);
    GstClockTime time = static_cast<GstClockTime>(position * MICRO);
    gst_player_seek(gstPlayer_, time);

    {
        condVarSeekSync_.wait(lock);
        MEDIA_LOGD("Seek finised!");
    }
}

void GstPlayerCtrl::Stop()
{
    std::unique_lock<std::mutex> lock(mutex_);
    auto task = std::make_shared<TaskHandler>([this] {
        StopSync();
        return ERR_OK;
    });
    (void)taskQue_.EnqueueTask(task);
}

void GstPlayerCtrl::StopSync()
{
    std::unique_lock<std::mutex> lock(mutex_);

    if (currentState_ == PLAYER_STOPPED) {
        return;
    }

    if (currentState_ == PLAYER_PLAYBACK_COMPLETE) {
        MEDIA_LOGI("gstplayer already stopped!");
        currentState_ = PLAYER_STOPPED;
        OnStateChanged(currentState_);
        return;
    }

    userStop_ = true;
    CHECK_AND_RETURN_LOG(gstPlayer_ != nullptr, "gstPlayer_ is nullptr");
    gst_player_stop(gstPlayer_);

    {
        condVarStopSync_.wait(lock);
        MEDIA_LOGD("Stop finised!");
    }

    bufferingStart_ = false;
    nextSeekFlag_ = false;
    seekInProgress_ = false;
    nextSeekPos_ = 0;
    enableLooping_ = false;
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
    int32_t volume = static_cast<int32_t>(VOLUME_TO_SYSTEM_VOLUME * leftVolume);
    int32_t ret = audioManager->SetVolume(AudioStandard::AudioSystemManager::AudioVolumeType::STREAM_MUSIC, volume);
    CHECK_AND_RETURN_LOG(ret == AudioStandard::SUCCESS, "set volume fail");
    OnVolumeChange();
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
    MEDIA_LOGD("GetPosition curTime(%{public}" PRIu64 ") duration(%{public}" PRIu64 ")", curTime, sourceDuration_);
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
    playerGst->ProcessStateChanged(player, state);
}

void GstPlayerCtrl::ProcessStateChanged(const GstPlayer *cbPlayer, GstPlayerState state)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (cbPlayer != gstPlayer_) {
        MEDIA_LOGE("gstplay cb object error: cbPlayer != gstPlayer_");
        return;
    }

    PlayerStates newState = PLAYER_IDLE;
    switch (state) {
        case GST_PLAYER_STATE_STOPPED: {
            newState = ProcessStoppedState();
            break;
        }
        case GST_PLAYER_STATE_BUFFERING: {
            OnMessage(PlayerMessageType::PLAYER_INFO_BUFFERING_START, 0);
            bufferingStart_ = true;
            return;
        }
        case GST_PLAYER_STATE_PAUSED: {
            newState = ProcessPausedState();
            break;
        }
        case GST_PLAYER_STATE_PLAYING: {
            newState = PLAYER_STARTED;
            break;
        }
        default: {
            return;
        }
    }

    if (bufferingStart_) {
        OnMessage(PlayerMessageType::PLAYER_INFO_BUFFERING_END, 0);
        bufferingStart_ = false;
    }

    MEDIA_LOGD("currentState_ = %{public}d, newState = %{public}d", currentState_, newState);
    if (newState != PLAYER_IDLE && currentState_ != newState) {
        currentState_ = newState;
        if (newState == PLAYER_STARTED) {
            OnMessage(PlayerMessageType::PLAYER_INFO_VIDEO_RENDERING_START, 0);
        }
        OnStateChanged(newState);
    }

    OnNotify(newState);
    /* call back sequence: statecb > seekdonecb */
    OnSeekDone();
    OnEndOfStream();
}

void GstPlayerCtrl::OnEndOfStreamCb(const GstPlayer *player, const GstPlayerCtrl *self)
{
    CHECK_AND_RETURN_LOG(player != nullptr, "player is null");
    CHECK_AND_RETURN_LOG(self != nullptr, "self is null");
    auto playerGst = const_cast<GstPlayerCtrl *>(self);
    return playerGst->ProcessEndOfStream(player);
}

void GstPlayerCtrl::ProcessEndOfStream(const GstPlayer *cbPlayer)
{
    if (cbPlayer != gstPlayer_) {
        MEDIA_LOGE("gstplay cb object error: cbPlayer != gstPlayer_");
        return;
    }
    MEDIA_LOGD("End of stream");
    endOfStreamCb_ = true;

    if (enableLooping_) {
        Seek(0, SEEK_PREVIOUS_SYNC);
    }
}

void GstPlayerCtrl::StreamDecErrorParse(const gchar *name, int32_t &errorCode)
{
    if (strstr(name, "aac") != nullptr) {
        MEDIA_LOGE("tag:MSERR_AUD_DEC_FAILED");
        errorCode = MSERR_AUD_DEC_FAILED;
    } else if (strstr(name, "h264") != nullptr || strstr(name, "h265") != nullptr) {
        MEDIA_LOGE("tag:MSERR_VID_DEC_FAILED");
        errorCode = MSERR_VID_DEC_FAILED;
    } else {
        MEDIA_LOGE("tag:MSERR_UNKNOWN");
        errorCode = MSERR_UNKNOWN;
    }
}

void GstPlayerCtrl::StreamErrorParse(const gchar *name, const GError *err, int32_t &errorCode)
{
    CHECK_AND_RETURN_LOG(name != nullptr, "name is null");
    CHECK_AND_RETURN_LOG(err != nullptr, "err is null");
    MEDIA_LOGE("domain:GST_STREAM_ERROR");
    auto streamIter = STREAM_TO_SERVICE_ERR_TABLE.find(err->code);
    if (streamIter != STREAM_TO_SERVICE_ERR_TABLE.end()) {
        errorCode = streamIter->second;
        return;
    }
    auto streamFuncIter = STREAM_TO_SERVICE_ERR_FUNC_TABLE.find(err->code);
    if (streamFuncIter != STREAM_TO_SERVICE_ERR_FUNC_TABLE.end()) {
        streamFuncIter->second(name, errorCode);
        return;
    }

    errorCode = MSERR_UNKNOWN;
}

void GstPlayerCtrl::ResourceErrorParse(const GError *err, int32_t &errorCode)
{
    CHECK_AND_RETURN_LOG(err != nullptr, "err is null");
    MEDIA_LOGE("domain:GST_RESOURCE_ERROR");
    auto resIter = RESOURCE_TO_SERVICE_ERR_TABLE.find(err->code);
    if (resIter == RESOURCE_TO_SERVICE_ERR_TABLE.end()) {
        errorCode = MSERR_UNKNOWN;
        return;
    }
    errorCode = resIter->second;
}

void GstPlayerCtrl::MessageErrorProcess(const char *name, const GError *err,
    PlayerErrorType &errorType, int32_t &errorCode)
{
    CHECK_AND_RETURN_LOG(err != nullptr, "err is null");
    CHECK_AND_RETURN_LOG(name != nullptr, "name is null");
    char *errMsg = gst_error_get_message (err->domain, err->code);
    MEDIA_LOGE("errMsg:%{public}s", errMsg);
    if (err->domain == GST_STREAM_ERROR) {
        errorType = PLAYER_ERROR;
        StreamErrorParse(name, err, errorCode);
    } else if (err->domain == GST_RESOURCE_ERROR) {
        errorType = PLAYER_ERROR;
        ResourceErrorParse(err, errorCode);
    } else {
        errorType = PLAYER_ERROR_UNKNOWN;
        errorCode = MSERR_UNKNOWN;
    }
    g_free (errMsg);
}

void GstPlayerCtrl::ErrorProcess(const GstMessage *msg, PlayerErrorType &errorType, int32_t &errorCode)
{
    CHECK_AND_RETURN_LOG(msg != nullptr, "msg is null");
    gchar *debug = nullptr;
    GError *err = nullptr;
    GstMessage *message = gst_message_copy(msg);
    CHECK_AND_RETURN_LOG(message != nullptr, "msg copy failed");
    gst_message_parse_error(message, &err, &debug);
    CHECK_AND_RETURN_LOG(msg->src != nullptr, "msg copy failed");
    gchar *name = gst_object_get_path_string (msg->src);
    MessageErrorProcess(name, err, errorType, errorCode);
    g_clear_error(&err);
    g_free(debug);
    g_free(name);
    gst_message_unref(message);
}

void GstPlayerCtrl::OnErrorCb(const GstPlayer *player, const GstMessage *msg, const GstPlayerCtrl *self)
{
    MEDIA_LOGE("Received error signal from pipeline.");
    CHECK_AND_RETURN_LOG(player != nullptr, "player is null");
    CHECK_AND_RETURN_LOG(msg != nullptr, "msg is null");
    CHECK_AND_RETURN_LOG(self != nullptr, "self is null");
    auto playerGst = const_cast<GstPlayerCtrl *>(self);

    // If looping is enabled, then disable it else will keep looping forever
    playerGst->enableLooping_ = false;
    PlayerErrorType errorType = PLAYER_ERROR_UNKNOWN;
    int32_t errorCode = MSERR_UNKNOWN;

    ErrorProcess(msg, errorType, errorCode);

    std::shared_ptr<IPlayerEngineObs> tempObs = playerGst->obs_.lock();
    if (tempObs != nullptr) {
        tempObs->OnError(errorType, errorCode);
    }

    playerGst->errorFlag_ = true;
}

void GstPlayerCtrl::OnSeekDoneCb(const GstPlayer *player, guint64 position, const GstPlayerCtrl *self)
{
    CHECK_AND_RETURN_LOG(player != nullptr, "player is null");
    CHECK_AND_RETURN_LOG(self != nullptr, "self is null");
    auto playerGst = const_cast<GstPlayerCtrl *>(self);
    playerGst->ProcessSeekDone(player, static_cast<uint64_t>(position) / MICRO);
}

void GstPlayerCtrl::ProcessSeekDone(const GstPlayer *cbPlayer, uint64_t position)
{
    if (cbPlayer != gstPlayer_) {
        MEDIA_LOGE("gstplay cb object error: cbPlayer != gstPlayer_");
        return;
    }

    // position = 99643, duration_ = 99591
    position = std::min(position, sourceDuration_);
    seekDoneNeedCb_ = true;
    seekDonePosition_ = position;
    MEDIA_LOGI("gstplay seek Done: (%{public}" PRIu64 ")", seekDonePosition_);
}

void GstPlayerCtrl::OnPositionUpdatedCb(const GstPlayer *player, guint64 position, const GstPlayerCtrl *self)
{
    CHECK_AND_RETURN_LOG(player != nullptr, "player is null");
    CHECK_AND_RETURN_LOG(self != nullptr, "self is null");
    auto playerGst = const_cast<GstPlayerCtrl *>(self);
    playerGst->ProcessPositionUpdated(player, static_cast<uint64_t>(position) / MICRO);
}

void GstPlayerCtrl::ProcessPositionUpdated(const GstPlayer *cbPlayer, uint64_t position)
{
    if (cbPlayer != gstPlayer_) {
        MEDIA_LOGE("gstplay cb object error: cbPlayer != gstPlayer_");
        return;
    }

    // position = 99643, duration_ = 99591
    position = std::min(position, sourceDuration_);
    Format format;
    std::shared_ptr<IPlayerEngineObs> tempObs = obs_.lock();
    if (tempObs != nullptr) {
        MEDIA_LOGD("ProcessPositionUpdated(%{public}" PRIu64 "), 0x%{public}06" PRIXPTR "",
            position, FAKE_POINTER(this));
        tempObs->OnInfo(INFO_TYPE_POSITION_UPDATE, static_cast<int32_t>(position), format);
    }
}

void GstPlayerCtrl::OnVolumeChange()
{
    Format format;
    std::shared_ptr<IPlayerEngineObs> tempObs = obs_.lock();
    if (tempObs != nullptr) {
        MEDIA_LOGE("OnVolumeChange");
        tempObs->OnInfo(INFO_TYPE_VOLUME_CHANGE, 0, format);
    }
}

void GstPlayerCtrl::OnStateChanged(PlayerStates state)
{
    if (state == PLAYER_PREPARED) {
        InitDuration();
    }

    MEDIA_LOGI("On State callback state: %{public}d", state);
    std::shared_ptr<IPlayerEngineObs> tempObs = obs_.lock();
    Format format;
    if (tempObs != nullptr) {
        MEDIA_LOGD("OnStateChanged %{public}d", state);
        tempObs->OnInfo(INFO_TYPE_STATE_CHANGE, static_cast<int32_t>(state), format);
    }
}

void GstPlayerCtrl::OnNotify(PlayerStates state)
{
    switch (state) {
        case PLAYER_PREPARED:
            condVarPauseSync_.notify_all();
            break;
        case PLAYER_STARTED:
            condVarPlaySync_.notify_all();
            break;
        case PLAYER_PAUSED:
            condVarPauseSync_.notify_all();
            break;
        case PLAYER_STOPPED:
            condVarStopSync_.notify_all();
            break;
        default:
            break;
    }
}

void GstPlayerCtrl::OnSeekDone()
{
    if (seekDoneNeedCb_) {
        MEDIA_LOGI("On Seek Done: (%{public}" PRIu64 ")", seekDonePosition_);
        std::shared_ptr<IPlayerEngineObs> tempObs = obs_.lock();
        Format format;
        if (tempObs != nullptr) {
            tempObs->OnInfo(INFO_TYPE_SEEKDONE, static_cast<int32_t>(seekDonePosition_), format);
        }
        seekDoneNeedCb_ = false;
        condVarSeekSync_.notify_all();

        MultipleSeek();
    }
}

void GstPlayerCtrl::OnEndOfStream()
{
    if (endOfStreamCb_) {
        MEDIA_LOGI("On EndOfStream: loop is %{public}d", enableLooping_);
        std::shared_ptr<IPlayerEngineObs> tempObs = obs_.lock();
        Format format;
        if (tempObs != nullptr) {
            tempObs->OnInfo(INFO_TYPE_EOS, static_cast<int32_t>(enableLooping_), format);
        }
        endOfStreamCb_ = false;
    }
}

void GstPlayerCtrl::OnMessage(int32_t type, int32_t extra)
{
    MEDIA_LOGI("On Message callback infoType: %{public}d", type);
    std::shared_ptr<IPlayerEngineObs> tempObs = obs_.lock();
    Format format;
    if (tempObs != nullptr) {
        tempObs->OnInfo(INFO_TYPE_MESSAGE, type, format);
    }
}

PlayerStates GstPlayerCtrl::ProcessStoppedState()
{
    PlayerStates newState = PLAYER_STOPPED;
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

    stopTimeFlag_ = false;
    return newState;
}

void GstPlayerCtrl::InitDuration()
{
    CHECK_AND_RETURN_LOG(gstPlayer_ != nullptr, "gstPlayer_ is nullptr");
    GstClockTime time = gst_player_get_duration(gstPlayer_);
    sourceDuration_ = static_cast<uint64_t>(time) / MICRO;
    MEDIA_LOGD("InitDuration duration(%{public}" PRIu64 ")", sourceDuration_);
}
} // Media
} // OHOS