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
constexpr uint64_t RING_BUFFER_MAX_SIZE = 5242880; // 5 * 1024 * 1024

PlayerEngineGstImpl::PlayerEngineGstImpl()
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
    if (url.find("://") == std::string::npos || url.find("file://") == 0) {
        return true;
    }
    return false;
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
    appsrcWarp_ = GstAppsrcWarp::Create(dataSrc);
    CHECK_AND_RETURN_RET_LOG(appsrcWarp_ != nullptr, MSERR_NO_MEMORY, "new appsrcwarp failed!");
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

    int32_t ret = GstPlayerInit();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_VAL, "GstPlayerInit failed");

    CHECK_AND_RETURN_RET_LOG(playerCtrl_ != nullptr, MSERR_INVALID_VAL, "playerCtrl_ is nullptr");
    playerCtrl_->Prepare();

    if (playerCtrl_->GetState() != PLAYER_PREPARED) {
        MEDIA_LOGE("gstplayer prepare failed");
        GstPlayerDeInit();
        return MSERR_INVALID_VAL;
    }

    // The duration of some resources without header information cannot be obtained.
    MEDIA_LOGD("Prepared ok out");
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::PrepareAsync()
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGD("Prepare in");

    int32_t ret = GstPlayerInit();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_VAL, "GstPlayerInit failed");

    CHECK_AND_RETURN_RET_LOG(playerCtrl_ != nullptr, MSERR_INVALID_VAL, "playerCtrl_ is nullptr");
    playerCtrl_->PrepareAsync();

    // The duration of some resources without header information cannot be obtained.
    MEDIA_LOGD("Prepared ok out");
    return MSERR_OK;
}

void PlayerEngineGstImpl::PlayerLoop()
{
    MEDIA_LOGD("PlayerLoop in");
    playerBuild_ = std::make_unique<GstPlayerBuild>();
    CHECK_AND_RETURN_LOG(playerBuild_ != nullptr, "playerBuild_ is nullptr");

    rendererCtrl_ = playerBuild_->BuildRendererCtrl(producerSurface_);
    CHECK_AND_RETURN_LOG(rendererCtrl_ != nullptr, "rendererCtrl_ is nullptr");

    playerCtrl_ = playerBuild_->BuildPlayerCtrl();
    CHECK_AND_RETURN_LOG(playerCtrl_ != nullptr, "playerCtrl_ is nullptr");

    condVarSync_.notify_all();

    MEDIA_LOGD("Start the player loop");
    playerBuild_->CreateLoop();
    MEDIA_LOGD("Stop the player loop");
}

int32_t PlayerEngineGstImpl::GstPlayerInit()
{
    if (gstPlayerInit_) {
        return MSERR_OK;
    }

    MEDIA_LOGD("GstPlayerInit in");
    playerThread_.reset(new(std::nothrow) std::thread(&PlayerEngineGstImpl::PlayerLoop, this));
    CHECK_AND_RETURN_RET_LOG(playerThread_ != nullptr, MSERR_NO_MEMORY, "new std::thread failed.");

    if (!playerCtrl_) {
        MEDIA_LOGD("Player not yet initialized, wait for 1 second");
        std::unique_lock<std::mutex> lockSync(mutexSync_);
        (void)condVarSync_.wait_for(lockSync, std::chrono::seconds(1));
        if (playerCtrl_ == nullptr) {
            MEDIA_LOGE("gstplayer initialized failed");
            GstPlayerDeInit();
            return MSERR_INVALID_VAL;
        }
    }

    int ret = GstPlayerPrepare();
    if (ret != MSERR_OK) {
        MEDIA_LOGE("GstPlayerPrepare failed");
        GstPlayerDeInit();
        return MSERR_INVALID_VAL;
    }

    playerBuild_->WaitMainLoopStart();

    MEDIA_LOGD("GstPlayerInit out");
    gstPlayerInit_ = true;
    return MSERR_OK;
}

void PlayerEngineGstImpl::GstPlayerDeInit()
{
    if (playerBuild_ != nullptr) {
        playerBuild_->DestroyLoop();
    }

    if (playerThread_ != nullptr && playerThread_->joinable()) {
        playerThread_->join();
    }

    rendererCtrl_ = nullptr;
    playerCtrl_ = nullptr;
    playerBuild_ = nullptr;
    gstPlayerInit_ = false;
    appsrcWarp_ = nullptr;
}

int32_t PlayerEngineGstImpl::GstPlayerPrepare() const
{
    CHECK_AND_RETURN_RET_LOG(playerCtrl_ != nullptr, MSERR_INVALID_VAL, "playerCtrl_ is nullptr");
    int32_t ret;
    if (appsrcWarp_ == nullptr) {
        ret = playerCtrl_->SetUrl(url_);
        playerCtrl_->SetRingBufferMaxSize(RING_BUFFER_MAX_SIZE);
        playerCtrl_->SetBufferingInfo();
        playerCtrl_->SetHttpTimeOut();
    } else {
        ret = playerCtrl_->SetSource(appsrcWarp_);
    }
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_VAL, "SetUrl failed");

    ret = rendererCtrl_->SetCallbacks(obs_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_VAL, "SetCallbacks failed");

    ret = playerCtrl_->SetCallbacks(obs_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_VAL, "SetCallbacks failed");

    if (producerSurface_ == nullptr) {
        playerCtrl_->SetVideoTrack(false);
    }
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::Play()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerCtrl_ != nullptr, MSERR_INVALID_OPERATION, "playerCtrl_ is nullptr");

    MEDIA_LOGD("Play in");
    playerCtrl_->Play();
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::Pause()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerCtrl_ != nullptr, MSERR_INVALID_OPERATION, "playerCtrl_ is nullptr");

    playerCtrl_->Pause();
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::GetCurrentTime(int32_t &currentTime)
{
    std::unique_lock<std::mutex> lock(mutex_);

    currentTime = 0;
    if (playerCtrl_ != nullptr) {
        uint64_t tempTime = playerCtrl_->GetPosition();
        currentTime = static_cast<int32_t>(tempTime);
        MEDIA_LOGD("Time in milli seconds: %{public}d", currentTime);
    }
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::GetVideoTrackInfo(std::vector<Format> &videoTrack)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerCtrl_ != nullptr, MSERR_INVALID_OPERATION, "playerCtrl_ is nullptr");

    int32_t retVal = playerCtrl_->GetVideoTrackInfo(videoTrack);
    return retVal;
}

int32_t PlayerEngineGstImpl::GetAudioTrackInfo(std::vector<Format> &audioTrack)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerCtrl_ != nullptr, MSERR_INVALID_OPERATION, "playerCtrl_ is nullptr");

    int32_t retVal = playerCtrl_->GetAudioTrackInfo(audioTrack);
    return retVal;
}

int32_t PlayerEngineGstImpl::GetVideoWidth()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerCtrl_ != nullptr, MSERR_INVALID_OPERATION, "playerCtrl_ is nullptr");

    return playerCtrl_->GetVideoWidth();
}

int32_t PlayerEngineGstImpl::GetVideoHeight()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerCtrl_ != nullptr, MSERR_INVALID_OPERATION, "playerCtrl_ is nullptr");

    return playerCtrl_->GetVideoHeight();
}

int32_t PlayerEngineGstImpl::GetDuration(int32_t &duration)
{
    std::unique_lock<std::mutex> lock(mutex_);

    duration = 0;
    if (playerCtrl_ != nullptr) {
        uint64_t tempDura = playerCtrl_->GetDuration();
        if (tempDura != GST_CLOCK_TIME_NONE) {
            duration = static_cast<int32_t>(tempDura);
        } else {
            duration = -1;
            MEDIA_LOGD("it's live mode");
        }

        MEDIA_LOGD("Duration in milli seconds: %{public}d", duration);
    }
    return MSERR_OK;
}

double PlayerEngineGstImpl::ChangeModeToSpeed(const PlaybackRateMode &mode) const
{
    if (mode == SPEED_FORWARD_0_75_X) {
        return SPEED_0_75_X;
    }
    if (mode == SPEED_FORWARD_1_00_X) {
        return SPEED_1_00_X;
    }
    if (mode == SPEED_FORWARD_1_25_X) {
        return SPEED_1_25_X;
    }
    if (mode == SPEED_FORWARD_1_75_X) {
        return SPEED_1_75_X;
    }
    if (mode == SPEED_FORWARD_2_00_X) {
        return SPEED_2_00_X;
    }

    MEDIA_LOGW("unknow mode:%{public}d, return default speed(SPEED_1_00_X)", mode);

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

    MEDIA_LOGW("unknow rate:%{public}lf, return default speed(SPEED_FORWARD_1_00_X)", rate);

    return  SPEED_FORWARD_1_00_X;
}

int32_t PlayerEngineGstImpl::SetPlaybackSpeed(PlaybackRateMode mode)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (playerCtrl_ != nullptr) {
        double rate = ChangeModeToSpeed(mode);
        return playerCtrl_->SetRate(rate);
    }
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::GetPlaybackSpeed(PlaybackRateMode &mode)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (playerCtrl_ != nullptr) {
        double rate = playerCtrl_->GetRate();
        mode = ChangeSpeedToMode(rate);
    }
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::SetLooping(bool loop)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (playerCtrl_ != nullptr) {
        MEDIA_LOGD("SetLooping in");
        return playerCtrl_->SetLoop(loop);
    }
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::Stop()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerCtrl_ != nullptr, MSERR_INVALID_OPERATION, "playerCtrl_ is nullptr");

    MEDIA_LOGD("Stop in");
    playerCtrl_->Stop();
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::Reset()
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGD("Reset in");
    if (playerCtrl_ != nullptr) {
        playerCtrl_->Stop();
    }
    GstPlayerDeInit();
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerCtrl_ != nullptr, MSERR_INVALID_OPERATION, "playerCtrl_ is nullptr");
    CHECK_AND_RETURN_RET_LOG(mSeconds >= 0, MSERR_INVALID_OPERATION, "seek time must >= 0");
    MEDIA_LOGD("Seek in %{public}d", mSeconds);

    uint64_t position = static_cast<uint64_t>(mSeconds);
    return playerCtrl_->Seek(position, mode);
}

int32_t PlayerEngineGstImpl::SetVolume(float leftVolume, float rightVolume)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (playerCtrl_ != nullptr) {
        MEDIA_LOGD("SetVolume in");
        playerCtrl_->SetVolume(leftVolume, rightVolume);
    }
    return MSERR_OK;
}
} // Media
} // OHOS
