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

#include "player_engine_gst_impl.h"

#include <unistd.h>
#include "media_log.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "audio_system_manager.h"
#include "player_sinkprovider.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerEngineGstImpl"};
}

namespace OHOS {
namespace Media {
constexpr float EPSINON = 0.0001;
constexpr float SPEED_0_75_X = 0.75;
constexpr float SPEED_1_00_X = 1.00;
constexpr float SPEED_1_25_X = 1.25;
constexpr float SPEED_1_75_X = 1.75;
constexpr float SPEED_2_00_X = 2.00;
constexpr size_t MAX_URI_SIZE = 4096;
constexpr int32_t MSEC_PER_USEC = 1000;
constexpr int32_t MSEC_PER_NSEC = 1000000;
constexpr int32_t BUFFER_TIME_DEFAULT = 15000; // 15s
constexpr int32_t BUFFER_HIGH_PERCENT_DEFAULT = 4;
constexpr int32_t BUFFER_FULL_PERCENT_DEFAULT = 100;
constexpr uint32_t INTERRUPT_EVENT_SHIFT = 8;
constexpr uint32_t MAX_SOFT_BUFFERS = 10;
constexpr uint32_t DEFAULT_CACHE_BUFFERS = 1;

PlayerEngineGstImpl::PlayerEngineGstImpl(int32_t uid, int32_t pid)
    : appuid_(uid), apppid_(pid)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayerEngineGstImpl::~PlayerEngineGstImpl()
{
    (void)Reset();
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

bool PlayerEngineGstImpl::IsFileUrl(const std::string &url) const
{
    return url.find("://") == std::string::npos || url.find("file://") == 0;
}

int32_t PlayerEngineGstImpl::GetRealPath(const std::string &url, std::string &realUrlPath) const
{
    std::string fileHead = "file://";
    std::string tempUrlPath;

    if (url.find(fileHead) == 0 && url.size() > fileHead.size()) {
        tempUrlPath = url.substr(fileHead.size());
    } else {
        tempUrlPath = url;
    }

    bool ret = PathToRealPath(tempUrlPath, realUrlPath);
    CHECK_AND_RETURN_RET_LOG(ret, MSERR_OPEN_FILE_FAILED,
        "invalid url. The Url (%{public}s) path may be invalid.", url.c_str());

    if (access(realUrlPath.c_str(), R_OK) != 0) {
        return MSERR_FILE_ACCESS_FAILED;
    }

    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::SetSource(const std::string &url)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(!url.empty(), MSERR_INVALID_VAL, "input url is empty!");
    CHECK_AND_RETURN_RET_LOG(url.length() <= MAX_URI_SIZE, MSERR_INVALID_VAL, "input url length is invalid!");

    std::string realUriPath;
    int32_t ret = MSERR_OK;

    if (IsFileUrl(url)) {
        ret = GetRealPath(url, realUriPath);
        if (ret != MSERR_OK) {
            return ret;
        }
        url_ = "file://" + realUriPath;
    } else {
        url_ = url;
    }

    MEDIA_LOGD("set player source: %{public}s", url_.c_str());
    return ret;
}

int32_t PlayerEngineGstImpl::SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(dataSrc != nullptr, MSERR_INVALID_VAL, "input dataSrc is empty!");
    appsrcWrap_ = GstAppsrcWrap::Create(dataSrc);
    CHECK_AND_RETURN_RET_LOG(appsrcWrap_ != nullptr, MSERR_NO_MEMORY, "new appsrcwrap failed!");
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::SetObs(const std::weak_ptr<IPlayerEngineObs> &obs)
{
    std::unique_lock<std::mutex> lock(mutex_);
    obs_ = obs;
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::SetVideoSurface(sptr<Surface> surface)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(surface != nullptr, MSERR_INVALID_VAL, "surface is nullptr");

    producerSurface_ = surface;
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::Prepare()
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGD("Prepare in");

    int32_t ret = PlayBinCtrlerInit();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_VAL, "PlayBinCtrlerInit failed");

    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, MSERR_INVALID_VAL, "playBinCtrler_ is nullptr");
    ret = playBinCtrler_->Prepare();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Prepare failed");

    // The duration of some resources without header information cannot be obtained.
    MEDIA_LOGD("Prepared ok out");
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::PrepareAsync()
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGD("Prepare in");

    int32_t ret = PlayBinCtrlerInit();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_VAL, "PlayBinCtrlerInit failed");

    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, MSERR_INVALID_VAL, "playBinCtrler_ is nullptr");
    ret = playBinCtrler_->PrepareAsync();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "PrepareAsync failed");

    // The duration of some resources without header information cannot be obtained.
    MEDIA_LOGD("Prepared ok out");
    return MSERR_OK;
}

void PlayerEngineGstImpl::HandleErrorMessage(const PlayBinMessage &msg)
{
    MEDIA_LOGE("error happended, cancel inprocessing job");

    PlayerErrorType errorType = PLAYER_ERROR_UNKNOWN;
    int32_t errorCode = msg.code;
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        notifyObs->OnError(errorType, errorCode);
    }
}

void PlayerEngineGstImpl::HandleInfoMessage(const PlayBinMessage &msg)
{
    MEDIA_LOGI("info msg type:%{public}d, value:%{public}d", msg.type, msg.code);

    int32_t status = msg.code;
    Format format;
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        notifyObs->OnInfo(static_cast<PlayerOnInfoType>(msg.type), status, format);
    }
}

void PlayerEngineGstImpl::HandleSeekDoneMessage(const PlayBinMessage &msg)
{
    MEDIA_LOGI("seek done, seek position = %{public}dms", msg.code / MSEC_PER_USEC);

    int32_t status = msg.code / MSEC_PER_USEC;
    Format format;
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        notifyObs->OnInfo(INFO_TYPE_SEEKDONE, status, format);
    }
}

void PlayerEngineGstImpl::HandleBufferingStart()
{
    percent_ = 0;
    Format format;
    (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_BUFFERING_START), 0);
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        notifyObs->OnInfo(INFO_TYPE_BUFFERING_UPDATE, 0, format);
    }
}

void PlayerEngineGstImpl::HandleBufferingEnd()
{
    Format format;
    (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_BUFFERING_END), 0);
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        notifyObs->OnInfo(INFO_TYPE_BUFFERING_UPDATE, 0, format);
    }
}

void PlayerEngineGstImpl::HandleBufferingTime(const PlayBinMessage &msg)
{
    std::pair<uint32_t, int64_t> bufferingTimePair = std::any_cast<std::pair<uint32_t, int64_t>>(msg.extra);
    uint32_t mqNumId = bufferingTimePair.first;
    int64_t bufferingTime = bufferingTimePair.second / MSEC_PER_NSEC;

    if (bufferingTime > BUFFER_TIME_DEFAULT) {
        bufferingTime = BUFFER_TIME_DEFAULT;
    }

    mqBufferingTime_[mqNumId] = bufferingTime;

    MEDIA_LOGD("ProcessBufferingTime(%{public}" PRIu64 " ms), mqNumId = %{public}u, "
        "mqNumUsedBuffering_ = %{public}u ms", bufferingTime, mqNumId, mqNumUsedBuffering_);

    if (mqBufferingTime_.size() != mqNumUsedBuffering_) {
        return;
    }

    uint64_t mqBufferingTime = BUFFER_TIME_DEFAULT;
    for (auto iter = mqBufferingTime_.begin(); iter != mqBufferingTime_.end(); ++iter) {
        if (iter->second < mqBufferingTime) {
            mqBufferingTime = iter->second;
        }
    }

    if (bufferingTime_ != mqBufferingTime) {
        bufferingTime_ = mqBufferingTime;
        std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
        if (notifyObs != nullptr) {
            Format format;
            (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_CACHED_DURATION),
                                     static_cast<int32_t>(mqBufferingTime));
            notifyObs->OnInfo(INFO_TYPE_BUFFERING_UPDATE, 0, format);
        }
    }
}

void PlayerEngineGstImpl::HandleBufferingPercent(const PlayBinMessage &msg)
{
    int32_t percent = msg.code;
    int32_t lastPercent = percent_;
    if (percent >= BUFFER_HIGH_PERCENT_DEFAULT) {
        percent_ = BUFFER_FULL_PERCENT_DEFAULT;
    } else {
        int32_t per = percent * BUFFER_FULL_PERCENT_DEFAULT / BUFFER_HIGH_PERCENT_DEFAULT;
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

void PlayerEngineGstImpl::HandleBufferingUsedMqNum(const PlayBinMessage &msg)
{
    mqNumUsedBuffering_ = std::any_cast<uint32_t>(msg.extra);
}

void PlayerEngineGstImpl::HandleVideoRenderingStart()
{
    Format format;
    MEDIA_LOGD("video rendering start");
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        notifyObs->OnInfo(INFO_TYPE_MESSAGE, PlayerMessageType::PLAYER_INFO_VIDEO_RENDERING_START, format);
    }
}

void PlayerEngineGstImpl::HandleVideoSizeChanged(const PlayBinMessage &msg)
{
    std::pair<int32_t, int32_t> resolution = std::any_cast<std::pair<int32_t, int32_t>>(msg.extra);
    Format format;
    (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_WIDTH), resolution.first);
    (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_HEIGHT), resolution.second);
    MEDIA_LOGD("video size changed, width = %{public}d, height = %{public}d", resolution.first, resolution.second);
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        notifyObs->OnInfo(INFO_TYPE_RESOLUTION_CHANGE, 0, format);
    }
    videoWidth_ = resolution.first;
    videoHeight_ = resolution.second;
}

void PlayerEngineGstImpl::HandleBitRateCollect(const PlayBinMessage &msg)
{
    std::pair<uint32_t *, uint32_t> bitRatePair = std::any_cast<std::pair<uint32_t *, uint32_t>>(msg.extra);
    Format format;
    (void)format.PutBuffer(std::string(PlayerKeys::PLAYER_BITRATE),
        static_cast<uint8_t *>(static_cast<void *>(bitRatePair.first)), bitRatePair.second * sizeof(uint32_t));
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        notifyObs->OnInfo(INFO_TYPE_BITRATE_COLLECT, 0, format);
    }
}

void PlayerEngineGstImpl::HandleSubTypeMessage(const PlayBinMessage &msg)
{
    switch (msg.subType) {
        case PLAYBIN_SUB_MSG_BUFFERING_START: {
            HandleBufferingStart();
            break;
        }
        case PLAYBIN_SUB_MSG_BUFFERING_END: {
            HandleBufferingEnd();
            break;
        }
        case PLAYBIN_SUB_MSG_BUFFERING_TIME: {
            HandleBufferingTime(msg);
            break;
        }
        case PLAYBIN_SUB_MSG_BUFFERING_PERCENT: {
            HandleBufferingPercent(msg);
            break;
        }
        case PLAYBIN_SUB_MSG_BUFFERING_USED_MQ_NUM: {
            HandleBufferingUsedMqNum(msg);
            break;
        }
        case PLAYBIN_SUB_MSG_VIDEO_RENDERING_START: {
            HandleVideoRenderingStart();
            break;
        }
        case PLAYBIN_SUB_MSG_VIDEO_SIZE_CHANGED: {
            HandleVideoSizeChanged(msg);
            break;
        }
        case PLAYBIN_SUB_MSG_BITRATE_COLLECT: {
            HandleBitRateCollect(msg);
            break;
        }
        case PLAYBIN_MSG_INTERRUPT_EVENT: {
            HandleInterruptMessage(msg);
            break;
        }
        default: {
            break;
        }
    }
}

void PlayerEngineGstImpl::HandleVolumeChangedMessage(const PlayBinMessage &msg)
{
    (void)msg;
    MEDIA_LOGI("volume changed");

    Format format;
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        notifyObs->OnInfo(INFO_TYPE_VOLUME_CHANGE, 0, format);
    }
}

void PlayerEngineGstImpl::HandleInterruptMessage(const PlayBinMessage &msg)
{
    MEDIA_LOGI("interrupt event in");
    uint32_t value = std::any_cast<uint32_t>(msg.extra);
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        Format format;
        int32_t hintType = value & 0x000000FF;
        int32_t forceType = (value >> INTERRUPT_EVENT_SHIFT) & 0x000000FF;
        int32_t eventType = value >> (INTERRUPT_EVENT_SHIFT * 2);
        (void)format.PutIntValue("eventType", eventType);
        (void)format.PutIntValue("forceType", forceType);
        (void)format.PutIntValue("hintType", hintType);
        notifyObs->OnInfo(INFO_TYPE_INTERRUPT_EVENT, 0, format);
    }
}

using MsgNotifyFunc = std::function<void(const PlayBinMessage&)>;

void PlayerEngineGstImpl::OnNotifyMessage(const PlayBinMessage &msg)
{
    const std::unordered_map<int32_t, MsgNotifyFunc> MSG_NOTIFY_FUNC_TABLE = {
        { PLAYBIN_MSG_ERROR, std::bind(&PlayerEngineGstImpl::HandleErrorMessage, this, std::placeholders::_1) },
        { PLAYBIN_MSG_SEEKDONE, std::bind(&PlayerEngineGstImpl::HandleSeekDoneMessage, this, std::placeholders::_1) },
        { PLAYBIN_MSG_SPEEDDONE, std::bind(&PlayerEngineGstImpl::HandleInfoMessage, this, std::placeholders::_1) },
        { PLAYBIN_MSG_BITRATEDONE, std::bind(&PlayerEngineGstImpl::HandleInfoMessage, this, std::placeholders::_1)},
        { PLAYBIN_MSG_EOS, std::bind(&PlayerEngineGstImpl::HandleInfoMessage, this, std::placeholders::_1) },
        { PLAYBIN_MSG_STATE_CHANGE, std::bind(&PlayerEngineGstImpl::HandleInfoMessage, this, std::placeholders::_1) },
        { PLAYBIN_MSG_SUBTYPE, std::bind(&PlayerEngineGstImpl::HandleSubTypeMessage, this, std::placeholders::_1) },
        { PLAYBIN_MSG_VOLUME_CHANGE, std::bind(&PlayerEngineGstImpl::HandleVolumeChangedMessage, this,
            std::placeholders::_1) },
    };

    if (MSG_NOTIFY_FUNC_TABLE.count(msg.type) != 0) {
        MSG_NOTIFY_FUNC_TABLE.at(msg.type)(msg);
    }
}

int32_t PlayerEngineGstImpl::PlayBinCtrlerInit()
{
    if (playBinCtrler_) {
        return MSERR_OK;
    }

    MEDIA_LOGD("PlayBinCtrlerInit in");
    int ret = PlayBinCtrlerPrepare();
    if (ret != MSERR_OK) {
        MEDIA_LOGE("PlayBinCtrlerPrepare failed");
        PlayBinCtrlerDeInit();
        return MSERR_INVALID_VAL;
    }

    MEDIA_LOGD("PlayBinCtrlerInit out");
    return MSERR_OK;
}

void PlayerEngineGstImpl::PlayBinCtrlerDeInit()
{
    url_.clear();
    appsrcWrap_ = nullptr;
    codecChangedDetector_ = nullptr;

    if (playBinCtrler_ != nullptr) {
        playBinCtrler_->SetElemSetupListener(nullptr);
        playBinCtrler_->SetElemUnSetupListener(nullptr);
        playBinCtrler_ = nullptr;
    }

    {
        std::unique_lock<std::mutex> lk(trackParseMutex_);
        trackParse_ = nullptr;
        sinkProvider_ = nullptr;
        for (auto &[elem, signalId] : signalIds_) {
            g_signal_handler_disconnect(elem, signalId);
        }
        signalIds_.clear();
    }
}

int32_t PlayerEngineGstImpl::PlayBinCtrlerPrepare()
{
    codecChangedDetector_ = std::make_shared<CodecChangedDetector>();
    uint8_t renderMode = IPlayBinCtrler::PlayBinRenderMode::DEFAULT_RENDER;
    auto notifier = std::bind(&PlayerEngineGstImpl::OnNotifyMessage, this, std::placeholders::_1);

    {
        std::unique_lock<std::mutex> lk(trackParseMutex_);
        sinkProvider_ = std::make_shared<PlayerSinkProvider>(producerSurface_);
        sinkProvider_->SetAppInfo(appuid_, apppid_);
    }

    IPlayBinCtrler::PlayBinCreateParam createParam = {
        static_cast<IPlayBinCtrler::PlayBinRenderMode>(renderMode), notifier, sinkProvider_
    };
    playBinCtrler_ = IPlayBinCtrler::Create(IPlayBinCtrler::PlayBinKind::PLAYBIN2, createParam);
    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, MSERR_INVALID_VAL, "playBinCtrler_ is nullptr");

    int32_t ret;
    if (appsrcWrap_ == nullptr) {
        ret = playBinCtrler_->SetSource(url_);
    } else {
        ret = playBinCtrler_->SetSource(appsrcWrap_);
    }
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_VAL, "SetSource failed");

    ret = SetVideoScaleType(videoScaleType_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_VAL, "SetVideoScaleType failed");

    ret = SetAudioRendererInfo(contentType_, streamUsage_, rendererFlag_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_VAL, "SetAudioRendererInfo failed");

    auto listener = std::bind(&PlayerEngineGstImpl::OnNotifyElemSetup, this, std::placeholders::_1);
    playBinCtrler_->SetElemSetupListener(listener);
    auto setupListener = std::bind(&PlayerEngineGstImpl::OnNotifyElemSetup, this, std::placeholders::_1);
    playBinCtrler_->SetElemSetupListener(setupListener);

    auto unSetupListener = std::bind(&PlayerEngineGstImpl::OnNotifyElemUnSetup, this, std::placeholders::_1);
    playBinCtrler_->SetElemSetupListener(unSetupListener);

    {
        std::unique_lock<std::mutex> lk(trackParseMutex_);
        trackParse_ = PlayerTrackParse::Create();
        if (trackParse_ == nullptr) {
            MEDIA_LOGE("creat track parse failed");
        }
    }

    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::Play()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, MSERR_INVALID_OPERATION, "playBinCtrler_ is nullptr");

    MEDIA_LOGD("Play in");
    playBinCtrler_->Play();
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::Pause()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, MSERR_INVALID_OPERATION, "playBinCtrler_ is nullptr");

    (void)playBinCtrler_->Pause();
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::GetCurrentTime(int32_t &currentTime)
{
    std::unique_lock<std::mutex> lock(mutex_);

    currentTime = 0;
    if (playBinCtrler_ != nullptr) {
        int64_t tempTime = playBinCtrler_->GetPosition();
        currentTime = static_cast<int32_t>(tempTime / MSEC_PER_USEC);
        MEDIA_LOGD("Time in milliseconds: %{public}d", currentTime);
    }
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::GetVideoTrackInfo(std::vector<Format> &videoTrack)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(trackParse_ != nullptr, MSERR_INVALID_OPERATION, "trackParse_ is nullptr");

    return trackParse_->GetVideoTrackInfo(videoTrack);
}

int32_t PlayerEngineGstImpl::GetAudioTrackInfo(std::vector<Format> &audioTrack)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(trackParse_ != nullptr, MSERR_INVALID_OPERATION, "trackParse_ is nullptr");

    return trackParse_->GetAudioTrackInfo(audioTrack);
}

int32_t PlayerEngineGstImpl::GetVideoWidth()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return videoWidth_;
}

int32_t PlayerEngineGstImpl::GetVideoHeight()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return videoHeight_;
}

int32_t PlayerEngineGstImpl::GetDuration(int32_t &duration)
{
    std::unique_lock<std::mutex> lock(mutex_);

    duration = 0;
    if (playBinCtrler_ != nullptr) {
        int64_t tempDura = playBinCtrler_->GetDuration();
        duration = static_cast<int32_t>(tempDura / MSEC_PER_USEC);
        MEDIA_LOGD("Duration in milliseconds: %{public}d", duration);
    }
    return MSERR_OK;
}

double PlayerEngineGstImpl::ChangeModeToSpeed(const PlaybackRateMode &mode) const
{
    switch (mode) {
        case SPEED_FORWARD_0_75_X:
            return SPEED_0_75_X;
        case SPEED_FORWARD_1_00_X:
            return SPEED_1_00_X;
        case SPEED_FORWARD_1_25_X:
            return SPEED_1_25_X;
        case SPEED_FORWARD_1_75_X:
            return SPEED_1_75_X;
        case SPEED_FORWARD_2_00_X:
            return SPEED_2_00_X;
        default:
            MEDIA_LOGW("unknown mode:%{public}d, return default speed(SPEED_1_00_X)", mode);
    }

    return SPEED_1_00_X;
}

PlaybackRateMode PlayerEngineGstImpl::ChangeSpeedToMode(double rate) const
{
    if (abs(rate - SPEED_0_75_X) < EPSINON) {
        return SPEED_FORWARD_0_75_X;
    }
    if (abs(rate - SPEED_1_00_X) < EPSINON) {
        return SPEED_FORWARD_1_00_X;
    }
    if (abs(rate - SPEED_1_25_X) < EPSINON) {
        return SPEED_FORWARD_1_25_X;
    }
    if (abs(rate - SPEED_1_75_X) < EPSINON) {
        return SPEED_FORWARD_1_75_X;
    }
    if (abs(rate - SPEED_2_00_X) < EPSINON) {
        return SPEED_FORWARD_2_00_X;
    }

    MEDIA_LOGW("unknown rate:%{public}lf, return default speed(SPEED_FORWARD_1_00_X)", rate);

    return  SPEED_FORWARD_1_00_X;
}

int32_t PlayerEngineGstImpl::SetPlaybackSpeed(PlaybackRateMode mode)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (playBinCtrler_ != nullptr) {
        double rate = ChangeModeToSpeed(mode);
        return playBinCtrler_->SetRate(rate);
    }
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::GetPlaybackSpeed(PlaybackRateMode &mode)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (playBinCtrler_ != nullptr) {
        double rate = playBinCtrler_->GetRate();
        mode = ChangeSpeedToMode(rate);
    }
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::SetLooping(bool loop)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (playBinCtrler_ != nullptr) {
        MEDIA_LOGD("SetLooping in");
        return playBinCtrler_->SetLoop(loop);
    }
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::SetParameter(const Format &param)
{
    if (param.ContainKey(PlayerKeys::VIDEO_SCALE_TYPE)) {
        int32_t videoScaleType = 0;
        param.GetIntValue(PlayerKeys::VIDEO_SCALE_TYPE, videoScaleType);
        return SetVideoScaleType(VideoScaleType(videoScaleType));
    }
    if (param.ContainKey(PlayerKeys::CONTENT_TYPE) && param.ContainKey(PlayerKeys::STREAM_USAGE)) {
        param.GetIntValue(PlayerKeys::CONTENT_TYPE, contentType_);
        param.GetIntValue(PlayerKeys::STREAM_USAGE, streamUsage_);
        param.GetIntValue(PlayerKeys::RENDERER_FLAG, rendererFlag_);
        return SetAudioRendererInfo(contentType_, streamUsage_, rendererFlag_);

    }
    if (param.ContainKey(PlayerKeys::AUDIO_INTERRUPT_MODE)) {
        int32_t interruptMode = 0;
        param.GetIntValue(PlayerKeys::AUDIO_INTERRUPT_MODE, interruptMode);
        return SetAudioInterruptMode(interruptMode);
    }
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::Stop()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, MSERR_INVALID_OPERATION, "playBinCtrler_ is nullptr");

    MEDIA_LOGD("Stop in");
    playBinCtrler_->Stop();
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::Reset()
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGD("Reset in");
    PlayBinCtrlerDeInit();
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, MSERR_INVALID_OPERATION, "playBinCtrler_ is nullptr");
    MEDIA_LOGI("Seek in %{public}dms", mSeconds);

    int64_t position = static_cast<int64_t>(mSeconds * MSEC_PER_USEC);
    return playBinCtrler_->Seek(position, mode);
}

int32_t PlayerEngineGstImpl::SetVolume(float leftVolume, float rightVolume)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (playBinCtrler_ != nullptr) {
        MEDIA_LOGD("SetVolume in");
        playBinCtrler_->SetVolume(leftVolume, rightVolume);
    }
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::SelectBitRate(uint32_t bitRate)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (playBinCtrler_ != nullptr) {
        MEDIA_LOGD("SelectBitRate in");
        return playBinCtrler_->SelectBitRate(bitRate);
    }
    return MSERR_INVALID_OPERATION;
}

int32_t PlayerEngineGstImpl::SetVideoScaleType(VideoScaleType videoScaleType)
{
    std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
    if (sinkProvider_ != nullptr) {
        MEDIA_LOGD("SetVideoScaleType in");
        sinkProvider_->SetVideoScaleType(static_cast<uint32_t>(videoScaleType));
    } else {
        videoScaleType_ = videoScaleType;
    }
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::SetAudioRendererInfo(const int32_t contentType,
    const int32_t streamUsage, const int32_t rendererFlag)
{
    std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
    contentType_ = contentType;
    streamUsage_ = streamUsage;
    rendererFlag_ = rendererFlag;
    if (playBinCtrler_ != nullptr) {
        MEDIA_LOGD("SetAudioRendererInfo in");
        int32_t rendererInfo(0);
        rendererInfo |= (contentType | (static_cast<uint32_t>(streamUsage) <<
        AudioStandard::RENDERER_STREAM_USAGE_SHIFT));
        playBinCtrler_->SetAudioRendererInfo(rendererInfo, rendererFlag);
    }
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::SetAudioInterruptMode(const int32_t interruptMode)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (playBinCtrler_ != nullptr) {
        MEDIA_LOGD("SetAudioInterruptMode in");
        playBinCtrler_->SetAudioInterruptMode(interruptMode);
    }
    return MSERR_OK;
}

void PlayerEngineGstImpl::OnNotifyElemSetup(GstElement &elem)
{
    std::unique_lock<std::mutex> lock(trackParseMutex_);
    CHECK_AND_RETURN_LOG(trackParse_ != nullptr, "trackParse_ is null");

    const gchar *metadata = gst_element_get_metadata(&elem, GST_ELEMENT_METADATA_KLASS);
    CHECK_AND_RETURN_LOG(metadata != nullptr, "gst_element_get_metadata return nullptr");

    MEDIA_LOGD("get element_name %{public}s, get metadata %{public}s", GST_ELEMENT_NAME(&elem), metadata);
    std::string metaStr(metadata);

    if (metaStr.find("Codec/Demuxer") != std::string::npos || metaStr.find("Codec/Parser") != std::string::npos) {
        if (trackParse_->GetDemuxerElementFind() == false) {
            gulong signalId = g_signal_connect(&elem, "pad-added",
                G_CALLBACK(PlayerTrackParse::OnPadAddedCb), trackParse_.get());
            CHECK_AND_RETURN_LOG(signalId != 0, "listen to pad-added failed");
            (void)signalIds_.emplace(&elem, signalId);

            trackParse_->SetDemuxerElementFind(true);
        }
    }

    CHECK_AND_RETURN_LOG(sinkProvider_ != nullptr, "sinkProvider_ is nullptr");
    GstElement *videoSink = sinkProvider_->GetVideoSink();
    CHECK_AND_RETURN_LOG(videoSink != nullptr, "videoSink is nullptr");
    codecChangedDetector_->DetectCodecSetup(metaStr, &elem, videoSink);
}

void PlayerEngineGstImpl::OnNotifyElemUnSetup(GstElement &elem)
{
    CHECK_AND_RETURN_LOG(sinkProvider_ != nullptr, "sinkProvider_ is nullptr");
    GstElement *videoSink = sinkProvider_->GetVideoSink();
    CHECK_AND_RETURN_LOG(videoSink != nullptr, "videoSink is nullptr");

    codecChangedDetector_->DetectCodecUnSetup(&elem, videoSink);
}

void CodecChangedDetector::SetupCodecCb(const std::string &metaStr, GstElement *src, GstElement *videoSink)
{
    if (metaStr.find("Codec/Decoder/Video/Hardware") != std::string::npos) {
        isHardwareDec_ = true;
        if (!codecTypeList_.empty()) {
            // For hls scene when change codec, the second codec should not go performance mode process.
            codecTypeList_.push_back(true);
            return;
        }
        // For performance mode.
        codecTypeList_.push_back(true);

        g_object_set(G_OBJECT(src), "performance-mode", TRUE, nullptr);
        g_object_set(G_OBJECT(videoSink), "performance-mode", TRUE, nullptr);

        GstCaps *caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "NV12", nullptr);
        g_object_set(G_OBJECT(videoSink), "caps", caps, nullptr);
        g_object_set(G_OBJECT(src), "sink-caps", caps, nullptr);
        gst_caps_unref(caps);

        GstBufferPool *pool;
        g_object_get(videoSink, "surface-pool", &pool, nullptr);
        g_object_set(G_OBJECT(src), "surface-pool", pool, nullptr);
    } else if (metaStr.find("Codec/Decoder/Video") != std::string::npos) {
        codecTypeList_.push_back(false);
    }
}

void CodecChangedDetector::DetectCodecSetup(const std::string &metaStr, GstElement *src, GstElement *videoSink)
{
    SetupCodecCb(metaStr, src, videoSink);

    if (metaStr.find("Sink/Video") != std::string::npos && !isHardwareDec_) {
        g_object_set(G_OBJECT(src), "max-pool-capacity", MAX_SOFT_BUFFERS, nullptr);
        g_object_set(G_OBJECT(src), "cache-buffers-num", DEFAULT_CACHE_BUFFERS, nullptr);
    }
}

void CodecChangedDetector::DetectCodecUnSetup(GstElement *src, GstElement *videoSink)
{
    const gchar *metadata = gst_element_get_metadata(src, GST_ELEMENT_METADATA_KLASS);
    CHECK_AND_RETURN_LOG(metadata != nullptr, "gst_element_get_metadata return nullptr");

    MEDIA_LOGD("get element_name %{public}s, get metadata %{public}s", GST_ELEMENT_NAME(src), metadata);
    std::string metaStr(metadata);
    if (metaStr.find("Codec/Decoder/Video") == std::string::npos) {
        return;
    }

    if (codecTypeList_.empty()) {
        MEDIA_LOGE("codec type list is empty");
        return;
    }

    bool codecType = codecTypeList_.front();
    codecTypeList_.pop_front();
    if ((codecTypeList_.empty()) || (codecType == codecTypeList_.front())) {
        MEDIA_LOGD("codec type is empty or the next is same");
        return;
    }

    GstCaps *caps = nullptr;
    if (codecTypeList_.front()) {
        caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "NV12", nullptr);
    } else {
        caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGBA", nullptr);
    }
    g_object_set(G_OBJECT(videoSink), "caps", caps, nullptr);
    gst_caps_unref(caps);
}
} // namespace Media
} // namespace OHOS
