/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "player_mock.h"
#include "media_errors.h"
#include "ui/rs_surface_node.h"
#include "window_option.h"

using namespace OHOS::Media::PlayerTestParam;

namespace OHOS {
namespace Media {
void PlayerSignal::SetState(PlayerStates state)
{
    state_ = state;
}

void PlayerSignal::SetSeekResult(bool seekDoneFlag)
{
    seekDoneFlag_ = seekDoneFlag;
}

void PlayerCallbackTest::OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    switch (type) {
        case INFO_TYPE_SEEKDONE:
            seekDoneFlag_ = true;
<<<<<<< HEAD
            signal_->SetSeekResult(true);
=======
            test_->SetSeekResult(true);
>>>>>>> 6caf11e6965040c256681a262d9723296064ab23
            SeekNotify(extra, infoBody);
            break;
        case INFO_TYPE_STATE_CHANGE:
            state_ = static_cast<PlayerStates>(extra);
<<<<<<< HEAD
            signal_->SetState(state_);
=======
            test_->SetState(state_);
>>>>>>> 6caf11e6965040c256681a262d9723296064ab23
            Notify(state_);
            break;
        case INFO_TYPE_POSITION_UPDATE:
            position_ = extra;
            break;
        default:
            break;
    }
}

void PlayerCallbackTest::Notify(PlayerStates currentState)
{
    if (currentState == PLAYER_PREPARED) {
<<<<<<< HEAD
        signal_->condVarPrepare_.notify_all();
    } else if (currentState == PLAYER_STARTED) {
        signal_->condVarPlay_.notify_all();
    } else if (currentState == PLAYER_PAUSED) {
        signal_->condVarPause_.notify_all();
    } else if (currentState == PLAYER_STOPPED) {
        signal_->condVarStop_.notify_all();
    } else if (currentState == PLAYER_IDLE) {
        signal_->condVarReset_.notify_all();
=======
        test_->condVarPrepare_.notify_all();
    } else if (currentState == PLAYER_STARTED) {
        test_->condVarPlay_.notify_all();
    } else if (currentState == PLAYER_PAUSED) {
        test_->condVarPause_.notify_all();
    } else if (currentState == PLAYER_STOPPED) {
        test_->condVarStop_.notify_all();
    } else if (currentState == PLAYER_IDLE) {
        test_->condVarReset_.notify_all();
>>>>>>> 6caf11e6965040c256681a262d9723296064ab23
    }
}

void PlayerCallbackTest::SeekNotify(int32_t extra, const Format &infoBody)
{
    if (signal_->seekMode_ == PlayerSeekMode::SEEK_CLOSEST) {
        if (signal_->seekPosition_ == extra) {
            signal_->condVarSeek_.notify_all();
        }
    } else if (signal_->seekMode_ == PlayerSeekMode::SEEK_PREVIOUS_SYNC) {
        if (signal_->seekPosition_ - extra < DELTA_TIME && extra - signal_->seekPosition_ >= 0) {
            signal_->condVarSeek_.notify_all();
        }
    } else if (signal_->seekMode_ == PlayerSeekMode::SEEK_NEXT_SYNC) {
        if (extra - signal_->seekPosition_ < DELTA_TIME && signal_->seekPosition_ - extra >= 0) {
            signal_->condVarSeek_.notify_all();
        }
    } else if (abs(signal_->seekPosition_ - extra) <= DELTA_TIME) {
        signal_->condVarSeek_.notify_all();
    } else {
        signal_->SetSeekResult(false);
    }
}

sptr<Surface> PlayerMock::GetVideoSurface()
{
    sptr<Rosen::WindowOption> option = new Rosen::WindowOption();
    option->SetWindowRect({ 0, 0, width_, height_ });
    option->SetWindowType(Rosen::WindowType::WINDOW_TYPE_APP_LAUNCHING);
    option->SetWindowMode(Rosen::WindowMode::WINDOW_MODE_FLOATING);
    previewWindow_ = Rosen::Window::Create("xcomponent_window_unittest", option);
    if (previewWindow_ == nullptr || previewWindow_->GetSurfaceNode() == nullptr) {
        return nullptr;
    }
    previewWindow_->Show();
    return previewWindow_->GetSurfaceNode()->GetSurface();
}

PlayerMock::PlayerMock(std::shared_ptr<PlayerSignal> signal) : signal_(signal) {}

PlayerMock::~PlayerMock()
{
    if (previewWindow_ != nullptr) {
        previewWindow_->Destroy();
        previewWindow_ = nullptr;
    }
}

bool PlayerMock::CreatePlayer()
{
    player_ = PlayerFactory::CreatePlayer();
    return player_ != nullptr;
}

int32_t PlayerMock::SetSource(const std::string url)
{
    std::unique_lock<std::mutex> lock(mutex_);
    int32_t ret = player_->SetSource(url);
    return ret;
}

int32_t PlayerMock::Prepare()
{
    std::unique_lock<std::mutex> lock(mutex_);
    int32_t ret = player_->Prepare();
    if (ret == MSERR_OK && signal_->state_ != PLAYER_PREPARED) {
        std::unique_lock<std::mutex> lockPrepare(signal_->mutexPrepare_);
        signal_->condVarPrepare_.wait_for(lockPrepare, std::chrono::seconds(WAITSECOND));
        if (signal_->state_ != PLAYER_PREPARED) {
            return -1;
        }
    }
    return ret;
}

int32_t PlayerMock::PrepareAsync()
{
    std::unique_lock<std::mutex> lock(mutex_);
    int ret = player_->PrepareAsync();
    if (ret == MSERR_OK && signal_->state_ != PLAYER_PREPARED) {
        std::unique_lock<std::mutex> lockPrepare(signal_->mutexPrepare_);
        signal_->condVarPrepare_.wait_for(lockPrepare, std::chrono::seconds(WAITSECOND));
        if (signal_->state_ != PLAYER_PREPARED) {
            return -1;
        }
    }
    return ret;
}

int32_t PlayerMock::Play()
{
    std::unique_lock<std::mutex> lock(mutex_);
    int32_t ret = player_->Play();
    if (ret == MSERR_OK && signal_->state_ != PLAYER_STARTED) {
        std::unique_lock<std::mutex> lockPlay(signal_->mutexPlay_);
        signal_->condVarPlay_.wait_for(lockPlay, std::chrono::seconds(WAITSECOND));
        if (signal_->state_ != PLAYER_STARTED) {
            return -1;
        }
    }
    return ret;
}

int32_t PlayerMock::Pause()
{
    std::unique_lock<std::mutex> lock(mutex_);
    int32_t ret = player_->Pause();
    if (ret == MSERR_OK && signal_->state_ != PLAYER_PAUSED) {
        std::unique_lock<std::mutex> lockPause(signal_->mutexPause_);
        signal_->condVarPause_.wait_for(lockPause, std::chrono::seconds(WAITSECOND));
        if (signal_->state_ != PLAYER_PAUSED) {
            return -1;
        }
    }
    return ret;
}

int32_t PlayerMock::Stop()
{
    std::unique_lock<std::mutex> lock(mutex_);
    int32_t ret = player_->Stop();
    if (ret == MSERR_OK && signal_->state_ != PLAYER_STOPPED) {
        std::unique_lock<std::mutex> lockStop(signal_->mutexStop_);
        signal_->condVarStop_.wait_for(lockStop, std::chrono::seconds(WAITSECOND));
        if (signal_->state_ != PLAYER_STOPPED) {
            return -1;
        }
    }
    return ret;
}

int32_t PlayerMock::Reset()
{
    int32_t ret = player_->Reset();
    if (ret == MSERR_OK && signal_->state_ != PLAYER_IDLE) {
        std::unique_lock<std::mutex> lockReset(signal_->mutexReset_);
        signal_->condVarReset_.wait_for(lockReset, std::chrono::seconds(WAITSECOND));
        if (signal_->state_ != PLAYER_IDLE) {
            return -1;
        }
    }
    return ret;
}

int32_t PlayerMock::Release()
{
    if (previewWindow_ != nullptr) {
        previewWindow_->Destroy();
        previewWindow_ = nullptr;
    }
    return player_->Release();
}

void PlayerMock::SeekPrePare(int32_t &mseconds, PlayerSeekMode &mode)
{
    int32_t duration = 0;
    player_->GetDuration(duration);
    if (mseconds < 0) {
        signal_->seekPosition_ = 0;
    } else if (mseconds > duration) {
        signal_->seekPosition_ = duration;
    } else {
        signal_->seekPosition_ = mseconds;
    }
}

int32_t PlayerMock::Seek(int32_t mseconds, PlayerSeekMode mode)
{
    std::unique_lock<std::mutex> lock(mutex_);
    SeekPrePare(mseconds, mode);
    int32_t ret = player_->Seek(mseconds, mode);
    if (ret == MSERR_OK && signal_->seekDoneFlag_ == false) {
        std::unique_lock<std::mutex> lockSeek(signal_->mutexSeek_);
        signal_->condVarSeek_.wait_for(lockSeek, std::chrono::seconds(WAITSECOND));
        if (signal_->seekDoneFlag_ != true) {
            return -1;
        }
    }
    return ret;
}

int32_t PlayerMock::SetVolume(float leftVolume, float rightVolume)
{
    std::unique_lock<std::mutex> lock(mutex_);
    return player_->SetVolume(leftVolume, rightVolume);
}

int32_t PlayerMock::SetLooping(bool loop)
{
    std::unique_lock<std::mutex> lock(mutex_);
    return player_->SetLooping(loop);
}

int32_t PlayerMock::GetCurrentTime(int32_t &currentTime)
{
    std::unique_lock<std::mutex> lock(mutex_);
    return player_->GetCurrentTime(currentTime);
}

bool PlayerMock::IsPlaying()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return player_->IsPlaying();
}

bool PlayerMock::IsLooping()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return player_->IsLooping();
}

int32_t PlayerMock::SetPlayerCallback(const std::shared_ptr<PlayerCallback> &callback)
{
    std::unique_lock<std::mutex> lock(mutex_);
    return player_->SetPlayerCallback(callback);
}

int32_t PlayerMock::SetVideoSurface(sptr<Surface> surface)
{
    std::unique_lock<std::mutex> lock(mutex_);
    return player_->SetVideoSurface(surface);
}

PlayerCallbackTest::PlayerCallbackTest(std::shared_ptr<PlayerSignal> signal)
    : signal_(signal) {}
} // namespace Media
} // namespace OHOS
