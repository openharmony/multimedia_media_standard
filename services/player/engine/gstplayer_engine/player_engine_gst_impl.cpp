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
#include <condition_variable>
#include "errors.h"
#include "media_log.h"
#include "display_type.h"
#include "player_register.h"
#include "histream_manager.h"
#include "errors.h"
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

PlayerEngineGstImpl::PlayerEngineGstImpl()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
    HistreamManager::Instance().SetUp();
    HistreamManager::Instance().UpdateLogLevel();
}

PlayerEngineGstImpl::~PlayerEngineGstImpl()
{
    (void)GstPlayerDeInit();
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t PlayerEngineGstImpl::SetSource(const std::string &uri)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(!uri.empty(), ERR_INVALID_VALUE, "input uri is empty!");

    bool hasFileHead = false;
    std::string::size_type position = -1;
    int32_t ret;
    std::string fileHead = "file://";
    std::string realUriPath, tempUriPath;

    position = uri.find(fileHead);
    hasFileHead = position != std::string::npos ? true : false;

    if (hasFileHead) {
        CHECK_AND_RETURN_RET_LOG(position == 0, ERR_INVALID_VALUE, "illegal uri!");
    }

    if (hasFileHead) {
        tempUriPath = uri.substr(position + fileHead.size());
    } else {
        tempUriPath = uri;
    }

    ret = PathToRealPath(tempUriPath, realUriPath);
    CHECK_AND_RETURN_RET_LOG(ret != ERR_OK, ERR_INVALID_VALUE, "invalid uri. The Uri (%{public}s) path may be invalid.",
        uri.c_str());

    if (access(realUriPath.c_str(), R_OK) != 0) {
        MEDIA_LOGE("dont have premission to open %{public}s.", realUriPath.c_str());
        return ERR_INVALID_VALUE;
    }

    fileUri_ = "file://" + realUriPath;
    MEDIA_LOGI("set player source: %{public}s", fileUri_.c_str());
    return ERR_OK;
}

int32_t PlayerEngineGstImpl::SetObs(const std::weak_ptr<IPlayerEngineObs> &obs)
{
    std::unique_lock<std::mutex> lock(mutex_);
    obs_ = obs;
    return ERR_OK;
}

void PlayerEngineGstImpl::Synchronize()
{
    condVarSync_.notify_all();
}

int32_t PlayerEngineGstImpl::SetVideoSurface(sptr<Surface> surface)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(surface != nullptr, ERR_INVALID_VALUE, "surface is nullptr");

    producerSurface_ = surface;
    return ERR_OK;
}

int32_t PlayerEngineGstImpl::Prepare()
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGI("Prepare in");

    int32_t ret = GstPlayerInit();
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, ERR_NO_INIT, "GstPlayerInit failed");

    CHECK_AND_RETURN_RET_LOG(playerCtrl_ != nullptr, ERR_NO_INIT, "playerCtrl_ is nullptr");
    playerCtrl_->Pause();

    if (playerCtrl_->GetState() != PLAYER_PREPARED) {
        std::unique_lock<std::mutex> lockSync(mutexSync_);
        condVarSync_.wait_for(lockSync, std::chrono::seconds(1));
        if (playerCtrl_->GetState() != PLAYER_PREPARED) {
            MEDIA_LOGE("gstplayer prepare failed");
            GstPlayerDeInit();
            return ERR_NO_INIT;
        }
    }
    MEDIA_LOGD("Prepared ok out");
    return ERR_OK;
}

int32_t PlayerEngineGstImpl::PrepareAsync()
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGI("PrepareAsync in");

    int32_t ret = GstPlayerInit();
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, ERR_NO_INIT, "GstPlayerInit failed");

    CHECK_AND_RETURN_RET_LOG(playerCtrl_ != nullptr, ERR_NO_INIT, "playerCtrl_ is nullptr");
    playerCtrl_->Pause();
    MEDIA_LOGD("PrepareAsync out");
    return ERR_OK;
}

void PlayerEngineGstImpl::PlayerLoop()
{
    MEDIA_LOGD("PlayerLoop in");
    playerBuild_ = std::make_unique<GstPlayerBuild>();
    CHECK_AND_RETURN_LOG(playerBuild_ != nullptr, "playerBuild_ is nullptr");

    playerCtrl_ = playerBuild_->Build(producerSurface_);
    CHECK_AND_RETURN_LOG(playerCtrl_ != nullptr, "playerCtrl_ is nullptr");

    condVarSync_.notify_all();

    MEDIA_LOGI("Start the player loop");
    playerBuild_->CreateLoop();
    MEDIA_LOGI("Stop the player loop");
}

int32_t PlayerEngineGstImpl::GstPlayerInit()
{
    if (gstPlayerInit_) {
        return ERR_OK;
    }

    MEDIA_LOGD("GstPlayerInit in");
    playerThread_.reset(new(std::nothrow) std::thread(&PlayerEngineGstImpl::PlayerLoop, this));
    CHECK_AND_RETURN_RET_LOG(playerThread_ != nullptr, ERR_NO_INIT, "std::thread failed..");

    if (!playerCtrl_) {
        MEDIA_LOGI("Player not yet initialized, wait for 1 second");
        std::unique_lock<std::mutex> lockSync(mutexSync_);
        condVarSync_.wait_for(lockSync, std::chrono::seconds(1));
        if (playerCtrl_ == nullptr) {
            MEDIA_LOGE("gstplayer initialized failed");
            GstPlayerDeInit();
            return ERR_NO_INIT;
        }
    }

    int ret = GstPlayerPrepare();
    if (ret != ERR_OK) {
        MEDIA_LOGE("GstPlayerPrepare failed");
        GstPlayerDeInit();
        return ERR_NO_INIT;
    }

    MEDIA_LOGI("GstPlayerInit out");
    gstPlayerInit_ = true;
    return ERR_OK;
}

void PlayerEngineGstImpl::GstPlayerDeInit()
{
    playerCtrl_ = nullptr;
    playerBuild_ = nullptr;
    if (playerThread_ != nullptr && playerThread_->joinable()) {
       playerThread_->join();
    }
    gstPlayerInit_ = false;
}

int32_t PlayerEngineGstImpl::GstPlayerPrepare()
{
    MEDIA_LOGI("GstPlayerPrepare In");
    CHECK_AND_RETURN_RET_LOG(playerCtrl_ != nullptr, ERR_NO_INIT, "playerCtrl_ is nullptr");
    int32_t ret = playerCtrl_->SetUri(fileUri_);
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, ERR_NO_INIT, "SetUri failed");

    ret = playerCtrl_->SetCallbacks(obs_, std::bind(&PlayerEngineGstImpl::Synchronize, this));
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, ERR_NO_INIT, "SetCallbacks failed");

    if (producerSurface_ == nullptr) {
        playerCtrl_->SetVideoTrack(FALSE);
    }

    return ERR_OK;
}

int32_t PlayerEngineGstImpl::Play()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerCtrl_ != nullptr, ERR_INVALID_OPERATION, "playerCtrl_ is nullptr");

    MEDIA_LOGD("Play in");
    playerCtrl_->Play();
    return ERR_OK;
}

int32_t PlayerEngineGstImpl::Pause()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerCtrl_ != nullptr, ERR_INVALID_OPERATION, "playerCtrl_ is nullptr");

    playerCtrl_->Pause();
    return ERR_OK;
}

int32_t PlayerEngineGstImpl::GetCurrentTime(uint64_t &currentTime)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerCtrl_ != nullptr, ERR_INVALID_OPERATION, "playerCtrl_ is nullptr");

    currentTime = playerCtrl_->GetPosition();
    MEDIA_LOGI("Time in milli seconds: %{public}llu", currentTime);
    return ERR_OK;
}

int32_t PlayerEngineGstImpl::GetDuration(uint64_t &duration)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerCtrl_ != nullptr, ERR_INVALID_OPERATION, "playerCtrl_ is nullptr");

    duration = playerCtrl_->GetDuration();
    MEDIA_LOGI("Duration in milli seconds: %{public}llu", duration);
    return ERR_OK;
}

double PlayerEngineGstImpl::ChangeModeToSpeed(const int32_t &mode) const
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
    MEDIA_LOGD("unknow return mode!");
    return SPEED_1_00_X;
}

int32_t PlayerEngineGstImpl::ChangeSpeedToMode(double rate) const
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
    MEDIA_LOGD("unknow return speed!");
    return  SPEED_FORWARD_1_00_X;
}

int32_t PlayerEngineGstImpl::SetPlaybackSpeed(int32_t mode)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerCtrl_ != nullptr, ERR_INVALID_OPERATION, "playerCtrl_ is nullptr");

    double rate = ChangeModeToSpeed(mode);
    playerCtrl_->SetRate(rate);
    return ERR_OK;
}

int32_t PlayerEngineGstImpl::GetPlaybackSpeed(int32_t &mode)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerCtrl_ != nullptr, ERR_INVALID_OPERATION, "playerCtrl_ is nullptr");

    double rate = playerCtrl_->GetRate();
    mode = ChangeSpeedToMode(rate);
    return ERR_OK;
}

int32_t PlayerEngineGstImpl::SetLooping(bool loop)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerCtrl_ != nullptr, ERR_INVALID_OPERATION, "playerCtrl_ is nullptr");

    MEDIA_LOGI("SetLooping in");
    playerCtrl_->SetLoop(loop);
    return ERR_OK;
}

int32_t PlayerEngineGstImpl::Stop()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerCtrl_ != nullptr, ERR_INVALID_OPERATION, "playerCtrl_ is nullptr");

    MEDIA_LOGI("Stop in");
    playerCtrl_->Stop();
    return ERR_OK;
}

int32_t PlayerEngineGstImpl::Reset()
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGI("Reset in");

    GstPlayerDeInit();
    return ERR_OK;
}

int32_t PlayerEngineGstImpl::Seek(uint64_t mSeconds, int32_t mode)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerCtrl_ != nullptr, ERR_INVALID_OPERATION, "playerCtrl_ is nullptr");
    MEDIA_LOGI("Seek in %{public}llu", mSeconds);

    playerCtrl_->Seek(mSeconds);
    return ERR_OK;
}

int32_t PlayerEngineGstImpl::SetVolume(float leftVolume, float rightVolume)
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGI("SetVolume in");

    CHECK_AND_RETURN_RET_LOG(playerCtrl_ != nullptr, ERR_INVALID_OPERATION, "playerCtrl_ is nullptr");
    playerCtrl_->SetVolume(leftVolume, rightVolume);
    return ERR_OK;
}

int GstPlayerFactoryMaker::Score(std::string uri) const
{
    (void)uri;
    constexpr int defaultScore = 100;
    return defaultScore;
}

IPlayerEngine *GstPlayerFactoryMaker::CreatePlayer() const
{
    MEDIA_LOGI("GstPlayerFactoryMaker::CreatePlayer");
    IPlayerEngine *impl = new(std::nothrow) PlayerEngineGstImpl();
    return impl;
}

REGISTER_MEDIAPLAYERBASE(std::make_shared<GstPlayerFactoryMaker>());
} // Media
} // OHOS