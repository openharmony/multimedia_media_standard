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
    constexpr float INVALID_VOLUME = -1.0;
    constexpr double DEFAULT_RATE = 1.0;
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "GstPlayerCtrl"};
    constexpr int MILLI = 1000;
    constexpr int MICRO = MILLI * 1000;
    // multiqueue property
    constexpr int PLAYBIN_QUEUE_MAX_SIZE = 100 * 1024 * 1024; // 100 * 1024 * 1024 Bytes
    constexpr int BUFFER_TIME_DEFAULT = 15000;  // 15s
    constexpr int HTTP_TIME_OUT_DEFAULT = 15000;  // 15s
    constexpr int BUFFER_LOW_PERCENT_DEFAULT = 1;
    constexpr int BUFFER_HIGH_PERCENT_DEFAULT = 4;
    constexpr int BUFFER_FULL_PERCENT_DEFAULT = 100;

    using namespace OHOS::Media;
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
        { GST_RESOURCE_ERROR_TIME_OUT, MSERR_NETWORK_TIMEOUT },
    };
}

namespace OHOS {
namespace Media {
GstPlayerCtrl::GstPlayerCtrl(GstPlayer *gstPlayer)
    : gstPlayer_(gstPlayer),
      taskQue_("GstCtrlTask"),
      volume_(INVALID_VOLUME),
      rate_(DEFAULT_RATE)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
    (void)taskQue_.Start();
    trackParse_ = GstPlayerTrackParse::Create();
    if (trackParse_ == nullptr) {
        MEDIA_LOGE("create track parse fail");
    }
}

GstPlayerCtrl::~GstPlayerCtrl()
{
    condVarPlaySync_.notify_all();
    condVarPauseSync_.notify_all();
    condVarStopSync_.notify_all();
    condVarSeekSync_.notify_all();
    condVarPreparingSync_.notify_all();
    (void)taskQue_.Stop();
    for (auto &signalId : signalIds_) {
        g_signal_handler_disconnect(gstPlayer_, signalId);
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void GstPlayerCtrl::SetRingBufferMaxSize(uint64_t size)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(gstPlayer_ != nullptr, "gstPlayer_ is nullptr");
    g_object_set(gstPlayer_, "ring-buffer-max-size", static_cast<guint64>(size), nullptr);
}

void GstPlayerCtrl::SetBufferingInfo()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(gstPlayer_ != nullptr, "gstPlayer_ is nullptr");
    MEDIA_LOGD("SetBufferingInfo");

    bool flags = true;
    g_object_set(gstPlayer_, "buffering-flags", flags, nullptr);

    uint64_t bufferDuration = static_cast<uint64_t>(BUFFER_TIME_DEFAULT) * static_cast<uint64_t>(MICRO);
    g_object_set(gstPlayer_, "buffer-size", PLAYBIN_QUEUE_MAX_SIZE,
        "buffer-duration", bufferDuration, "low-percent", BUFFER_LOW_PERCENT_DEFAULT,
        "high-percent", BUFFER_HIGH_PERCENT_DEFAULT, nullptr);
}

void GstPlayerCtrl::SetHttpTimeOut()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(gstPlayer_ != nullptr, "gstPlayer_ is nullptr");
    MEDIA_LOGD("SetHttpTimeOut");

    g_object_set(gstPlayer_, "timeout", static_cast<uint32_t>(HTTP_TIME_OUT_DEFAULT / MILLI), nullptr);
}

int32_t GstPlayerCtrl::SetUrl(const std::string &url)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(gstPlayer_ != nullptr, MSERR_INVALID_VAL, "gstPlayer_ is nullptr");
    gst_player_set_uri(gstPlayer_, url.c_str());
    currentState_ = PLAYER_PREPARING;
    return MSERR_OK;
}

int32_t GstPlayerCtrl::SetSource(const std::shared_ptr<GstAppsrcWarp> &appsrcWarp)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(gstPlayer_ != nullptr, MSERR_INVALID_OPERATION, "gstPlayer_ is nullptr");
    appsrcWarp_ = appsrcWarp;
    gst_player_set_uri(gstPlayer_, "appsrc://");
    currentState_ = PLAYER_PREPARING;
    return MSERR_OK;
}

int32_t GstPlayerCtrl::SetCallbacks(const std::weak_ptr<IPlayerEngineObs> &obs)
{
    CHECK_AND_RETURN_RET_LOG(obs.lock() != nullptr,
        MSERR_INVALID_OPERATION, "obs is nullptr, please set callback");
    if (appsrcWarp_ != nullptr) {
        CHECK_AND_RETURN_RET_LOG(appsrcWarp_->SetErrorCallback(obs) == MSERR_OK,
            MSERR_INVALID_OPERATION, "set obs failed");
    }

    signalIds_.push_back(g_signal_connect(gstPlayer_, "state-changed", G_CALLBACK(OnStateChangedCb), this));
    signalIds_.push_back(g_signal_connect(gstPlayer_, "end-of-stream", G_CALLBACK(OnEndOfStreamCb), this));
    signalIds_.push_back(g_signal_connect(gstPlayer_, "error-msg", G_CALLBACK(OnErrorCb), this));
    signalIds_.push_back(g_signal_connect(gstPlayer_, "seek-done", G_CALLBACK(OnSeekDoneCb), this));
    signalIds_.push_back(g_signal_connect(gstPlayer_, "position-updated", G_CALLBACK(OnPositionUpdatedCb), this));
    signalIds_.push_back(g_signal_connect(gstPlayer_, "source-setup", G_CALLBACK(OnSourceSetupCb), this));
    signalIds_.push_back(g_signal_connect(gstPlayer_, "buffering", G_CALLBACK(OnCachedPercentCb), this));
    signalIds_.push_back(g_signal_connect(gstPlayer_, "buffering-time", G_CALLBACK(OnBufferingTimeCb), this));
    signalIds_.push_back(g_signal_connect(gstPlayer_, "mq-num-use-buffering", G_CALLBACK(OnMqNumUseBufferingCb), this));
    signalIds_.push_back(g_signal_connect(gstPlayer_, "resolution-changed", G_CALLBACK(OnResolutionChanegdCb), this));
    signalIds_.push_back(g_signal_connect(gstPlayer_, "element-setup", G_CALLBACK(OnElementSetupCb), this));

    obs_ = obs;
    currentState_ = PLAYER_PREPARING;
    return MSERR_OK;
}

void GstPlayerCtrl::OnElementSetupCb(const GstPlayer *player, GstElement *src, GstPlayerCtrl *playerGst)
{
    CHECK_AND_RETURN_LOG(player != nullptr, "player is null");
    CHECK_AND_RETURN_LOG(playerGst != nullptr, "playerGst is null");
    CHECK_AND_RETURN_LOG(src != nullptr, "src is null");

    const gchar *metadata = gst_element_get_metadata(src, GST_ELEMENT_METADATA_KLASS);
    if (metadata == nullptr) {
        MEDIA_LOGE("gst_element_get_metadata return nullptr");
        return;
    }

    MEDIA_LOGD("get element_name %{public}s, get metadata %{public}s", GST_ELEMENT_NAME(src), metadata);
    std::string metaStr(metadata);

    if (metaStr.find("Codec/Demuxer") != std::string::npos || metaStr.find("Codec/Parser") != std::string::npos) {
        if (playerGst->trackParse_->GetDemuxerElementFind() == false) {
            playerGst->signalIds_.push_back(g_signal_connect(src,
                "pad-added", G_CALLBACK(GstPlayerTrackParse::OnPadAddedCb), playerGst->trackParse_.get()));
            playerGst->trackParse_->SetDemuxerElementFind(true);
        }
    }

    if (metaStr.find("Codec/Decoder/Video/Hardware") != std::string::npos) {
        playerGst->isHardWare_ = true;
        return;
    }

    if (metaStr.find("Sink/Video") != std::string::npos && playerGst->isHardWare_) {
        GstCaps *caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "NV12", nullptr);
        g_object_set(G_OBJECT(src), "caps", caps, nullptr);
        return;
    }
}

void GstPlayerCtrl::SetVideoTrack(bool enable)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(gstPlayer_ != nullptr, "gstPlayer_ is nullptr");

    gst_player_set_video_track_enabled(gstPlayer_, static_cast<gboolean>(enable));
    MEDIA_LOGI("SetVideoTrack Enabled %{public}d", enable);
}

void GstPlayerCtrl::Prepare()
{
    isExit_ = false;
    PauseSync();
}

void GstPlayerCtrl::PrepareAsync()
{
    isExit_ = false;
    Pause();
}

void GstPlayerCtrl::Pause()
{
    std::unique_lock<std::mutex> lock(mutex_);
    auto task = std::make_shared<TaskHandler<void>>([this] { PauseSync(); });
    (void)taskQue_.EnqueueTask(task);
}

void GstPlayerCtrl::PauseSync()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (appsrcWarp_ != nullptr) {
        (void)appsrcWarp_->Prepare();
    }

    if (isExit_ ||
        currentState_ == PLAYER_PAUSED ||
        currentState_ == PLAYER_PREPARED ||
        currentState_ == PLAYER_PLAYBACK_COMPLETE) {
        return;
    }

    if (currentState_ == PLAYER_PREPARING) {
        preparing_ = true;
    }

    MEDIA_LOGD("Pause start!");
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
    auto task = std::make_shared<TaskHandler<void>>([this] { PlaySync(); });
    (void)taskQue_.EnqueueTask(task);
}

void GstPlayerCtrl::PlaySync()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (currentState_ == PLAYER_STARTED || isExit_) {
        return;
    }
    if (IsLiveMode() && currentState_ == PLAYER_PLAYBACK_COMPLETE) {
        std::shared_ptr<IPlayerEngineObs> tempObs = obs_.lock();
        if (tempObs != nullptr) {
            tempObs->OnError(PLAYER_ERROR_UNKNOWN, MSERR_INVALID_STATE);
        }
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

int32_t GstPlayerCtrl::Seek(uint64_t position, const PlayerSeekMode mode)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (IsLiveMode()) {
        return MSERR_INVALID_OPERATION;
    }

    position = (position > sourceDuration_) ? sourceDuration_ : position;
    auto task = std::make_shared<TaskHandler<void>>([this, position, mode] { SeekSync(position, mode); });
    if (taskQue_.EnqueueTask(task) != 0) {
        MEDIA_LOGE("Seek fail");
        return MSERR_INVALID_OPERATION;
    }

    if (seekTask_ != nullptr) {
        MEDIA_LOGI("cancel pre seek Task");
        seekTask_->Cancel();
    }
    seekTask_ = task;

    return MSERR_OK;
}

void GstPlayerCtrl::SeekSync(uint64_t position, const PlayerSeekMode mode)
{
    std::unique_lock<std::mutex> lock(mutex_);
    seekTask_ = nullptr;
    if (currentState_ == PLAYER_STOPPED || isExit_) {
        return;
    }

    if (currentState_ == PLAYER_PLAYBACK_COMPLETE) {
        currentState_ = PLAYER_PAUSED;
    }

    CHECK_AND_RETURN_LOG(gstPlayer_ != nullptr, "gstPlayer_ is nullptr");
    // need keep the seek and seek modes consistent.
    g_object_set(gstPlayer_, "seek-mode", static_cast<gint>(ChangeSeekModeToGstFlag(mode)), nullptr);
    GstClockTime time = static_cast<GstClockTime>(position * MICRO);
    (void)GetPositionInner();
    seeking_ = true;
    gst_player_seek(gstPlayer_, time);

    {
        condVarSeekSync_.wait(lock);
        MEDIA_LOGD("Seek finised!");
    }
}

void GstPlayerCtrl::Stop()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (currentState_ <= PLAYER_PREPARING && !preparing_) {
        isExit_ = true;
        return;
    }

    if (currentState_ == PLAYER_PREPARING && preparing_) {
        MEDIA_LOGD("begin wait stop for current status is preparing!");
        static constexpr int32_t timeout = 1;
        condVarPreparingSync_.wait_for(lock, std::chrono::seconds(timeout));
        MEDIA_LOGD("end wait stop for current status is preparing!");
    }

    if (appsrcWarp_ != nullptr) {
        appsrcWarp_->Stop();
    }
    if (currentState_ == PLAYER_STOPPED) {
        return;
    }

    isExit_ = true;
    userStop_ = true;
    CHECK_AND_RETURN_LOG(gstPlayer_ != nullptr, "gstPlayer_ is nullptr");
    MEDIA_LOGD("Stop start!");
    g_object_set(gstPlayer_, "exit-block", 1, nullptr);
    gst_player_stop(gstPlayer_);

    {
        condVarStopSync_.wait(lock);
        MEDIA_LOGD("Stop finised!");
    }

    condVarPlaySync_.notify_all();
    condVarPauseSync_.notify_all();
    condVarSeekSync_.notify_all();
    bufferingStart_ = false;
    nextSeekFlag_ = false;
    enableLooping_ = false;
    speeding_ = false;
    seeking_ = false;
    rate_ = DEFAULT_RATE;
    lastTime_ = 0;
    if (audioSink_ != nullptr) {
        g_signal_handler_disconnect(audioSink_, signalIdVolume_);
        signalIdVolume_ = 0;
        gst_object_unref(audioSink_);
        audioSink_ = nullptr;
    }
    if (rateTask_ != nullptr) {
        rateTask_->Cancel();
        rateTask_ = nullptr;
    }
    if (seekTask_ != nullptr) {
        seekTask_->Cancel();
        seekTask_ = nullptr;
    }
}

int32_t GstPlayerCtrl::SetLoop(bool loop)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (IsLiveMode()) {
        return MSERR_INVALID_OPERATION;
    }
    enableLooping_ = loop;
    return MSERR_OK;
}

void GstPlayerCtrl::SetVolume(const float &leftVolume, const float &rightVolume)
{
    (void)rightVolume;
    std::unique_lock<std::mutex> lock(mutex_);
    volume_ = leftVolume;
    if (audioSink_ != nullptr) {
        MEDIA_LOGI("SetVolume(%{public}f) to audio sink", volume_);
        g_object_set(audioSink_, "volume", volume_, nullptr);
    }
}

int32_t GstPlayerCtrl::SetParameter(const Format &param)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (!SetAudioRendererInfo(param)) {
        MEDIA_LOGE("unsupported params");
        return MSERR_UNSUPPORT_AUD_PARAMS;
    }

    return MSERR_OK;
}

uint64_t GstPlayerCtrl::GetPosition()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return GetPositionInner();
}

uint64_t GstPlayerCtrl::GetPositionInner()
{
    CHECK_AND_RETURN_RET_LOG(gstPlayer_ != nullptr, 0, "gstPlayer_ is nullptr");

    if (currentState_ == PLAYER_STOPPED) {
        return 0;
    }

    if (currentState_ == PLAYER_PLAYBACK_COMPLETE) {
        if (IsLiveMode()) {
            return lastTime_;
        }
        return sourceDuration_;
    }

    if (seeking_ || speeding_) {
        return lastTime_;
    }

    GstClockTime position = gst_player_get_position(gstPlayer_);
    uint64_t curTime = static_cast<uint64_t>(position) / MICRO;
    curTime = std::min(curTime, sourceDuration_);
    lastTime_ = curTime;
    MEDIA_LOGD("GetPosition curTime(%{public}" PRIu64 ") duration(%{public}" PRIu64 ")", curTime, sourceDuration_);
    return curTime;
}

uint64_t GstPlayerCtrl::GetDuration()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(gstPlayer_ != nullptr, 0, "gstPlayer_ is nullptr");
    if (IsLiveMode()) {
        return GST_CLOCK_TIME_NONE;
    }
    if (currentState_ == PLAYER_STOPPED) {
        return sourceDuration_;
    }

    InitDuration();
    return sourceDuration_;
}

int32_t GstPlayerCtrl::GetVideoTrackInfo(std::vector<Format> &videoTrack)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(trackParse_ != nullptr, 0, "trackParse_ is nullptr");

    return trackParse_->GetVideoTrackInfo(videoTrack);
}

int32_t GstPlayerCtrl::GetAudioTrackInfo(std::vector<Format> &audioTrack)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(trackParse_ != nullptr, 0, "trackParse_ is nullptr");

    return trackParse_->GetAudioTrackInfo(audioTrack);
}

int32_t GstPlayerCtrl::GetVideoWidth()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return videoWidth_;
}

int32_t GstPlayerCtrl::GetVideoHeight()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return videoHeight_;
}

int32_t GstPlayerCtrl::SetRate(double rate)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (IsLiveMode()) {
        return MSERR_INVALID_OPERATION;
    }
    auto task = std::make_shared<TaskHandler<void>>([this, rate] { SetRateSync(rate); });
    if (taskQue_.EnqueueTask(task) != 0) {
        MEDIA_LOGE("set rate(%{public}lf) fail", rate);
        return MSERR_INVALID_OPERATION;
    }

    if (rateTask_ != nullptr) {
        MEDIA_LOGI("cancel pre rate task(%{public}lf)", rate_);
        rateTask_->Cancel();
    }
    rateTask_ = task;
    rate_ = rate;
    return MSERR_OK;
}

void GstPlayerCtrl::SetRateSync(double rate)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (isExit_) {
        return;
    }
    rateTask_ = nullptr;

    CHECK_AND_RETURN_LOG(gstPlayer_ != nullptr, "gstPlayer_ is nullptr");
    MEDIA_LOGD("SetRateSync in, rate=(%{public}lf)", rate);
    (void)GetPositionInner();
    speeding_ = true;
    gst_player_set_rate(gstPlayer_, static_cast<gdouble>(rate));

    condVarSeekSync_.wait(lock);
    MEDIA_LOGD("SetRateSync out, rate=(%{public}lf)", rate);
}

double GstPlayerCtrl::GetRate()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(gstPlayer_ != nullptr, 1.0, "gstPlayer_ is nullptr");

    MEDIA_LOGD("get rate=%{public}lf", rate_);
    return rate_;
}

PlayerStates GstPlayerCtrl::GetState() const
{
    return currentState_;
}

void GstPlayerCtrl::GetAudioSink()
{
    GstElement *playbin = gst_player_get_pipeline(gstPlayer_);
    CHECK_AND_RETURN_LOG(playbin != nullptr, "playbin is null");

    if (audioSink_ != nullptr) {
        gst_object_unref(audioSink_);
        audioSink_ = nullptr;
    }
    g_object_get(playbin, "audio-sink", &audioSink_, nullptr);

    CHECK_AND_RETURN_LOG(audioSink_ != nullptr, "get audio sink fail");

    signalIdVolume_ = g_signal_connect(audioSink_, "notify::volume", G_CALLBACK(OnVolumeChangeCb), this);

    gst_object_unref(playbin);
}

void GstPlayerCtrl::OnStateChangedCb(const GstPlayer *player, GstPlayerState state, GstPlayerCtrl *playerGst)
{
    MEDIA_LOGD("OnStateChangedCb gstplayer State changed: %{public}s", gst_player_state_get_name(state));
    CHECK_AND_RETURN_LOG(player != nullptr, "player is null");
    CHECK_AND_RETURN_LOG(playerGst != nullptr, "playerGst is null");
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
            OnBufferingUpdate(std::string(PlayerKeys::PLAYER_BUFFERING_START));
            bufferingStart_ = true;
            percent_ = 0;
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
        OnBufferingUpdate(std::string(PlayerKeys::PLAYER_BUFFERING_END));
        bufferingStart_ = false;
    }

    MEDIA_LOGD("currentState_ = %{public}d, newState = %{public}d", currentState_, newState);
    if (newState != PLAYER_IDLE && currentState_ != newState) {
        OnStateChanged(newState);
    }

    OnNotify(newState);
    /* call back sequence: statecb > seekdonecb */
    OnSeekDone();
    OnEndOfStream();
}

void GstPlayerCtrl::OnEndOfStreamCb(const GstPlayer *player, GstPlayerCtrl *playerGst)
{
    CHECK_AND_RETURN_LOG(player != nullptr, "player is null");
    CHECK_AND_RETURN_LOG(playerGst != nullptr, "playerGst is null");
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
    if (IsLiveMode()) {
        return;
    }

    if (enableLooping_) {
        (void)Seek(0, SEEK_PREVIOUS_SYNC);
    } else {
        Pause();
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
    char *errMsg = gst_error_get_message(err->domain, err->code);
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
    g_free(errMsg);
}

void GstPlayerCtrl::ErrorProcess(const GstMessage *msg, PlayerErrorType &errorType, int32_t &errorCode)
{
    CHECK_AND_RETURN_LOG(msg != nullptr, "msg is null");
    gchar *debug = nullptr;
    GError *err = nullptr;
    GstMessage *message = gst_message_copy(msg);
    CHECK_AND_RETURN_LOG(message != nullptr, "msg copy failed");
    gst_message_parse_error(message, &err, &debug);
    if (msg->src == nullptr) {
        g_clear_error(&err);
        g_free(debug);
        gst_message_unref(message);
        MEDIA_LOGE("msg parse failed");
        return;
    }
    gchar *name = gst_object_get_path_string(msg->src);
    MessageErrorProcess(name, err, errorType, errorCode);
    g_clear_error(&err);
    g_free(debug);
    g_free(name);
    gst_message_unref(message);
}

void GstPlayerCtrl::OnErrorCb(const GstPlayer *player, const GstMessage *msg, GstPlayerCtrl *playerGst)
{
    MEDIA_LOGE("Received error signal from pipeline.");
    CHECK_AND_RETURN_LOG(player != nullptr, "player is null");
    CHECK_AND_RETURN_LOG(msg != nullptr, "msg is null");
    CHECK_AND_RETURN_LOG(playerGst != nullptr, "playerGst is null");

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

void GstPlayerCtrl::OnResolutionChanegdCb(const GstPlayer *player,
    int32_t width, int32_t height, GstPlayerCtrl *playerGst)
{
    CHECK_AND_RETURN_LOG(player != nullptr, "player is null");
    CHECK_AND_RETURN_LOG(playerGst != nullptr, "playerGst is null");

    playerGst->OnResolutionChange(width, height);
}

void GstPlayerCtrl::OnResolutionChange(int32_t width, int32_t height)
{
    Format format;
    (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_WIDTH), width);
    (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_HEIGHT), height);
    std::shared_ptr<IPlayerEngineObs> tempObs = obs_.lock();
    if (tempObs != nullptr) {
        MEDIA_LOGD("OnResolutionChange width:%{public}d, height:%{public}d", width, height);
        tempObs->OnInfo(INFO_TYPE_RESOLUTION_CHANGE, 0, format);
    }
    videoWidth_ = width;
    videoHeight_ = height;
}

void GstPlayerCtrl::OnSeekDoneCb(const GstPlayer *player, guint64 position, GstPlayerCtrl *playerGst)
{
    CHECK_AND_RETURN_LOG(player != nullptr, "player is null");
    CHECK_AND_RETURN_LOG(playerGst != nullptr, "playerGst is null");
    playerGst->ProcessSeekDone(player, static_cast<uint64_t>(position) / MICRO);
}

void GstPlayerCtrl::ProcessSeekDone(const GstPlayer *cbPlayer, uint64_t position)
{
    if (cbPlayer != gstPlayer_) {
        MEDIA_LOGE("gstplay cb object error: cbPlayer != gstPlayer_");
        return;
    }

    position = std::min(position, sourceDuration_);
    seekDoneNeedCb_ = true;
    seekDonePosition_ = position;
    MEDIA_LOGI("gstplay seek Done: (%{public}" PRIu64 ")", seekDonePosition_);
}

void GstPlayerCtrl::OnSourceSetupCb(const GstPlayer *player, GstElement *src, GstPlayerCtrl *playerGst)
{
    CHECK_AND_RETURN_LOG(player != nullptr, "player is null");
    CHECK_AND_RETURN_LOG(playerGst != nullptr, "playerGst is null");
    CHECK_AND_RETURN_LOG(src != nullptr, "self is null");
    if (playerGst->appsrcWarp_ == nullptr) {
        MEDIA_LOGD("appsrc is null, is not stream mode");
        return;
    }
    GstElementFactory *elementFac = gst_element_get_factory(src);
    const gchar *eleTypeName = g_type_name(gst_element_factory_get_element_type(elementFac));
    if ((eleTypeName != nullptr) && (strstr(eleTypeName, "GstAppSrc") != nullptr)) {
        (void)playerGst->appsrcWarp_->SetAppsrc(src);
    }
}

void GstPlayerCtrl::OnBufferingTimeCb(const GstPlayer *player, guint64 bufferingTime,
    guint mqNumId, GstPlayerCtrl *playerGst)
{
    CHECK_AND_RETURN_LOG(player != nullptr, "player is null");
    CHECK_AND_RETURN_LOG(playerGst != nullptr, "playerGst is null");
    playerGst->ProcessBufferingTime(player, bufferingTime, mqNumId);
}

void GstPlayerCtrl::ProcessBufferingTime(const GstPlayer *cbPlayer, guint64 bufferingTime, guint mqNumId)
{
    if (cbPlayer != gstPlayer_) {
        MEDIA_LOGE("gstplay cb object error: cbPlayer != gstPlayer_");
        return;
    }

    if (currentState_ == PLAYER_PLAYBACK_COMPLETE) {
        return;
    }

    bufferingTime = bufferingTime / MICRO;
    if (bufferingTime > BUFFER_TIME_DEFAULT) {
        bufferingTime = BUFFER_TIME_DEFAULT;
    }

    mqBufferingTime_[mqNumId] = bufferingTime;

    MEDIA_LOGD("ProcessBufferingTime(%{public}" PRIu64 "), mqNumId = %{public}u mqNumUseBuffering_ = %{public}u",
        bufferingTime, mqNumId, mqNumUseBuffering_);

    if (mqBufferingTime_.size() != mqNumUseBuffering_) {
        return;
    }

    guint64 mqBufferingTime = BUFFER_TIME_DEFAULT;
    for (auto iter = mqBufferingTime_.begin(); iter != mqBufferingTime_.end(); ++iter) {
        if (iter->second < mqBufferingTime) {
            mqBufferingTime = iter->second;
        }
    }

    if (bufferingTime_ != mqBufferingTime) {
        bufferingTime_ = mqBufferingTime;
        std::shared_ptr<IPlayerEngineObs> tempObs = obs_.lock();
        if (tempObs != nullptr) {
            Format format;
            (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_CACHED_DURATION),
                                     static_cast<int32_t>(mqBufferingTime));
            tempObs->OnInfo(INFO_TYPE_BUFFERING_UPDATE, 0, format);
        }
    }
}

void GstPlayerCtrl::OnCachedPercentCb(const GstPlayer *player, guint percent, GstPlayerCtrl *playerGst)
{
    CHECK_AND_RETURN_LOG(player != nullptr, "player is null");
    CHECK_AND_RETURN_LOG(playerGst != nullptr, "playerGst is null");
    playerGst->ProcessCachedPercent(player, percent);
}

void GstPlayerCtrl::ProcessCachedPercent(const GstPlayer *cbPlayer, int32_t percent)
{
    if (cbPlayer != gstPlayer_) {
        MEDIA_LOGE("gstplay cb object error: cbPlayer != gstPlayer_");
        return;
    }

    if (currentState_ == PLAYER_PLAYBACK_COMPLETE) {
        return;
    }

    int32_t lastPercent = percent_;
    if (percent >= BUFFER_HIGH_PERCENT_DEFAULT) {
        percent_ = BUFFER_FULL_PERCENT_DEFAULT;
    } else {
        int per = percent * BUFFER_FULL_PERCENT_DEFAULT / BUFFER_HIGH_PERCENT_DEFAULT;
        if (percent_ < per) {
            percent_ = per;
        }
    }

    if (lastPercent == percent_) {
        return;
    }

    std::shared_ptr<IPlayerEngineObs> tempObs = obs_.lock();
    if (tempObs != nullptr) {
        Format format;
        (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_BUFFERING_PERCENT), percent_);
        MEDIA_LOGD("percent = (%{public}d), percent_ = %{public}d, 0x%{public}06" PRIXPTR "",
            percent, percent_, FAKE_POINTER(this));
        tempObs->OnInfo(INFO_TYPE_BUFFERING_UPDATE, 0, format);
    }
}

void GstPlayerCtrl::OnMqNumUseBufferingCb(const GstPlayer *player, guint mqNumUseBuffering, GstPlayerCtrl *playerGst)
{
    CHECK_AND_RETURN_LOG(player != nullptr, "player is null");
    CHECK_AND_RETURN_LOG(playerGst != nullptr, "playerGst is null");
    playerGst->ProcessMqNumUseBuffering(player, mqNumUseBuffering);
}

void GstPlayerCtrl::ProcessMqNumUseBuffering(const GstPlayer *cbPlayer, uint32_t mqNumUseBuffering)
{
    if (cbPlayer != gstPlayer_) {
        MEDIA_LOGE("gstplay cb object error: cbPlayer != gstPlayer_");
        return;
    }

    MEDIA_LOGD("mqNumUseBuffering = (%{public}u)", mqNumUseBuffering);
    mqNumUseBuffering_ = mqNumUseBuffering;
}

void GstPlayerCtrl::OnPositionUpdatedCb(const GstPlayer *player, guint64 position, GstPlayerCtrl *playerGst)
{
    CHECK_AND_RETURN_LOG(player != nullptr, "player is null");
    CHECK_AND_RETURN_LOG(playerGst != nullptr, "playerGst is null");
    playerGst->ProcessPositionUpdated(player, static_cast<uint64_t>(position) / MICRO);
}

void GstPlayerCtrl::ProcessPositionUpdated(const GstPlayer *cbPlayer, uint64_t position)
{
    if (cbPlayer != gstPlayer_) {
        MEDIA_LOGE("gstplay cb object error: cbPlayer != gstPlayer_");
        return;
    }
    lastTime_ = position;

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

void GstPlayerCtrl::OnVolumeChangeCb(const GObject *combiner, const GParamSpec *pspec, const GstPlayerCtrl *playerGst)
{
    (void)combiner;
    (void)pspec;

    MEDIA_LOGI("OnVolumeChangeCb in");
    CHECK_AND_RETURN_LOG(playerGst != nullptr, "playerGst is null");
    playerGst->OnVolumeChange();
}

void GstPlayerCtrl::OnVolumeChange() const
{
    Format format;
    std::shared_ptr<IPlayerEngineObs> tempObs = obs_.lock();
    if (tempObs != nullptr) {
        MEDIA_LOGI("OnVolumeChange");
        tempObs->OnInfo(INFO_TYPE_VOLUME_CHANGE, 0, format);
    }
}

void GstPlayerCtrl::OnStateChanged(PlayerStates state)
{
    if (state == PLAYER_PREPARED) {
        InitDuration();
        GetAudioSink();
        MEDIA_LOGW("KPI-TRACE: prepared");
    }

    currentState_ = state;
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
            condVarPreparingSync_.notify_all();
            preparing_ = false;
            break;
        case PLAYER_STARTED:
            condVarPlaySync_.notify_all();
            break;
        case PLAYER_PAUSED:
        case PLAYER_PLAYBACK_COMPLETE:
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
            if (speeding_) {
                tempObs->OnInfo(INFO_TYPE_SPEEDDONE, 0, format);
                speeding_ = false;
            } else {
                tempObs->OnInfo(INFO_TYPE_SEEKDONE, static_cast<int32_t>(seekDonePosition_), format);
                seeking_ = false;
            }
        }
        seekDoneNeedCb_ = false;
        condVarSeekSync_.notify_all();
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

void GstPlayerCtrl::OnMessage(int32_t extra) const
{
    MEDIA_LOGI("On Message callback info: %{public}d", extra);
    std::shared_ptr<IPlayerEngineObs> tempObs = obs_.lock();
    Format format;
    if (tempObs != nullptr) {
        tempObs->OnInfo(INFO_TYPE_MESSAGE, extra, format);
    }
}

void GstPlayerCtrl::OnBufferingUpdate(const std::string Message) const
{
    MEDIA_LOGI("On Message callback info: %{public}s", Message.c_str());
    std::shared_ptr<IPlayerEngineObs> tempObs = obs_.lock();
    if (tempObs != nullptr) {
        Format format;
        (void)format.PutIntValue(Message, 0);
        tempObs->OnInfo(INFO_TYPE_BUFFERING_UPDATE, 0, format);
    }
}

PlayerStates GstPlayerCtrl::ProcessStoppedState()
{
    PlayerStates newState = PLAYER_STOPPED;
    if (userStop_ || errorFlag_) {
        userStop_ = false;
    } else if (IsLiveMode()) {
        if (currentState_ == PLAYER_STARTED) {
            newState = PLAYER_PLAYBACK_COMPLETE;
        }
    } else {
        if (currentState_ == PLAYER_STARTED) {
            newState = PLAYER_STARTED;
            locatedInEos_ = enableLooping_ ? false : true;
        }
    }

    if (currentState_ == PLAYER_PREPARING) {
        // return stop when vidoe/audio prepare failed, notify pause finished
        condVarPauseSync_.notify_all();
        condVarPreparingSync_.notify_all();
        preparing_ = false;
    }
    return newState;
}

PlayerStates GstPlayerCtrl::ProcessPausedState()
{
    PlayerStates newState = PLAYER_PAUSED;

    if ((currentState_ == PLAYER_PREPARING) ||
        (currentState_ == PLAYER_STOPPED) ||
        (currentState_ == PLAYER_PREPARED)) {
        newState = PLAYER_PREPARED;
    } else if (currentState_ == PLAYER_PLAYBACK_COMPLETE) {
        newState = PLAYER_PLAYBACK_COMPLETE;
    } else if (currentState_ == PLAYER_STARTED && locatedInEos_) {
        newState = PLAYER_PLAYBACK_COMPLETE;
        locatedInEos_ = false;
    } else {
        newState = PLAYER_PAUSED;
    }
    MEDIA_LOGI("ProcessPausedState currentStatus: %{public}d, newState: %{public}d", currentState_, newState);

    return newState;
}

void GstPlayerCtrl::InitDuration()
{
    CHECK_AND_RETURN_LOG(gstPlayer_ != nullptr, "gstPlayer_ is nullptr");
    GstClockTime time = gst_player_get_duration(gstPlayer_);
    if (time != GST_CLOCK_TIME_NONE) {
        sourceDuration_ = static_cast<uint64_t>(time) / MICRO;
    } else {
        sourceDuration_ = GST_CLOCK_TIME_NONE;
    }

    MEDIA_LOGD("InitDuration duration(%{public}" PRIu64 ")", sourceDuration_);
}

bool GstPlayerCtrl::IsLiveMode() const
{
    if (appsrcWarp_ != nullptr && appsrcWarp_->IsLiveMode()) {
        return true;
    }

    if (sourceDuration_ == GST_CLOCK_TIME_NONE) {
        return true;
    }

    return false;
}

bool GstPlayerCtrl::SetAudioRendererInfo(const Format &param)
{
    CHECK_AND_RETURN_RET_LOG(audioSink_ != nullptr, false, "audioSink_ is nullptr");

    int32_t contentType = 0;
    int32_t streamUsage = 0;

    if (param.GetIntValue(PlayerKeys::CONTENT_TYPE, contentType) &&
        param.GetIntValue(PlayerKeys::STREAM_USAGE, streamUsage)) {
        int32_t rendererInfo(0);
        CHECK_AND_RETURN_RET(streamUsage >= 0 && contentType >= 0, false);
        rendererInfo |= (contentType | (static_cast<uint32_t>(streamUsage) <<
            AudioStandard::RENDERER_STREAM_USAGE_SHIFT));
        g_object_set(audioSink_, "audio-renderer-desc", rendererInfo, nullptr);
        return true;
    } else {
        MEDIA_LOGI("parameter doesn't contain content_type or stream_usage");
        return false;
    }
}
} // namespace Media
} // namespace OHOS
