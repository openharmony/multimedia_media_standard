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
    switch (type)
    {
        case INFO_TYPE_SEEKDONE:
            seekDoneFlag_ = true;
            test_->SetSeekResult(true);
            SeekNotify(extra, infoBody);
            break;
        case INFO_TYPE_STATE_CHANGE:
            state_ = static_cast<PlayerStates>(extra);
            test_->SetState(state_);  
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
        test_->condVarPrepare_.notify_all();
    } else if (currentState == PLAYER_STARTED) {
        test_->condVarPlay_.notify_all();
    } else if (currentState == PLAYER_PAUSED) {
        test_->condVarPause_.notify_all();
    } else if (currentState == PLAYER_STOPPED) {
        test_->condVarStop_.notify_all();
    } else if(currentState == PLAYER_IDLE) {
        test_->condVarReset_.notify_all();
    }
}

void PlayerCallbackTest::SeekNotify(int32_t extra, const Format &infoBody)
{
    if (test_->seekMode_ == PlayerSeekMode::SEEK_CLOSEST) {
        if (test_->seekPosition_ == extra) {
            test_->condVarSeek_.notify_all();
        } 
    } else if (test_->seekMode_ == PlayerSeekMode::SEEK_PREVIOUS_SYNC) {
        if (test_->seekPosition_ - extra < DELTA_TIME && extra - test_->seekPosition_ >= 0) {
            test_->condVarSeek_.notify_all();
        }
    } else if (test_->seekMode_ == PlayerSeekMode::SEEK_NEXT_SYNC) {
        if (extra - test_->seekPosition_ < DELTA_TIME && test_->seekPosition_ - extra >= 0) {
            test_->condVarSeek_.notify_all();
        }
    } else if (abs(test_->seekPosition_ - extra) <= DELTA_TIME) {
        test_->condVarSeek_.notify_all();
    } else {
        test_->SetSeekResult(false);
    }
}

sptr<Surface> Player_mock::GetVideoSurface()
{
    sptr<Rosen::WindowOption> option = new Rosen::WindowOption();
    option->SetWindowRect({0, 0, width, height});
    option->SetWindowType(Rosen::WindowType::WINDOW_TYPE_APP_LAUNCHING);
    option->SetWindowMode(Rosen::WindowMode::WINDOW_MODE_FLOATING);
    previewWindow_ = Rosen::Window::Create("xcomponent_window_unittest", option);
    if (previewWindow_ == nullptr || previewWindow_->GetSurfaceNode() == nullptr) {
        return nullptr;
    }
    previewWindow_->Show();
    return previewWindow_->GetSurfaceNode()->GetSurface();
}

Player_mock::Player_mock(std::shared_ptr<PlayerSignal> test)
    : test_(test)
{

}

Player_mock::~Player_mock()
{
    if (previewWindow_ != nullptr) {
        previewWindow_->Destroy();
        previewWindow_ = nullptr;
    }
}

bool Player_mock::CreatePlayer()
{
    player_ = PlayerFactory::CreatePlayer();
    return player_ != nullptr;
}

int32_t Player_mock::SetSource(const std::string url)
{
    int32_t ret = player_->SetSource(url);
    return ret;
}

int32_t Player_mock::Prepare()
{
    int32_t ret = player_->Prepare();
    if (ret == MSERR_OK && test_->state_ != PLAYER_PREPARED) {
        std::unique_lock<std::mutex> lockPrepare(test_->mutexPrepare_);
        test_->condVarPrepare_.wait_for(lockPrepare, std::chrono::seconds(WAITSECOND));
        if (test_->state_ != PLAYER_PREPARED) {
            return -1;
        }
    }
    return ret;
}

int32_t Player_mock::PrepareAsync()
{
    int ret = player_->PrepareAsync();
    if (ret == MSERR_OK && test_->state_ != PLAYER_PREPARED) {
        std::unique_lock<std::mutex> lockPrepare(test_->mutexPrepare_);
        test_->condVarPrepare_.wait_for(lockPrepare, std::chrono::seconds(WAITSECOND));
        if (test_->state_ != PLAYER_PREPARED) {
            return -1;
        }
    }
    return ret;
}

int32_t Player_mock::Play()
{
    int32_t ret = player_->Play();
    if (ret == MSERR_OK && test_->state_ != PLAYER_STARTED) {
        std::unique_lock<std::mutex> lockPlay(test_->mutexPlay_);
        test_->condVarPlay_.wait_for(lockPlay, std::chrono::seconds(WAITSECOND));
        if (test_->state_ != PLAYER_STARTED) {
            return -1;
        }
    }
    return ret;
}

int32_t Player_mock::Pause()
{
    int32_t ret = player_->Pause();
    if (ret == MSERR_OK && test_->state_ != PLAYER_PAUSED) {
        std::unique_lock<std::mutex> lockPause(test_->mutexPause_);
        test_->condVarPause_.wait_for(lockPause, std::chrono::seconds(WAITSECOND));
        if (test_->state_ != PLAYER_PAUSED) {
            return -1;
        }
    }
    return ret;    
}

int32_t Player_mock::Stop()
{
    int32_t ret = player_->Stop();
    if (ret == MSERR_OK && test_->state_ != PLAYER_STOPPED) {
        std::unique_lock<std::mutex> lockStop(test_->mutexStop_);
        test_->condVarStop_.wait_for(lockStop, std::chrono::seconds(WAITSECOND));
        if (test_->state_ != PLAYER_STOPPED) {
            return -1;
        }
    }
    return ret;
}

int32_t Player_mock::Reset()
{
    int32_t ret = player_->Reset(); 
    if (ret == MSERR_OK && test_->state_ != PLAYER_IDLE) {
        std::unique_lock<std::mutex> lockReset(test_->mutexReset_);
        test_->condVarReset_.wait_for(lockReset, std::chrono::seconds(WAITSECOND));
        if (test_->state_ != PLAYER_IDLE) {
            return -1;
        }
    }
    return ret;
}

int32_t Player_mock::Release()
{
    if (previewWindow_ != nullptr) {
        previewWindow_->Destroy();
        previewWindow_ = nullptr;
    }
    return player_->Release();
}

int32_t Player_mock::Seek(int32_t mseconds, PlayerSeekMode mode)
{
    test_->seekDoneFlag_ = false;
    int32_t duration = 0;
    player_->GetDuration(duration);
    if (mseconds < 0) {
        test_->seekPosition_ = 0;
    } else if (mseconds > duration) {
        test_->seekPosition_ = duration;
    } else {
        test_->seekPosition_ = mseconds;
    }
    test_->seekMode_ = mode;
    int32_t ret = player_->Seek(mseconds, mode);
    if (ret == MSERR_OK && test_->seekDoneFlag_ == false) {
        std::unique_lock<std::mutex> lockSeek(test_->mutexSeek_);
        test_->condVarSeek_.wait_for(lockSeek, std::chrono::seconds(WAITSECOND));
        if (test_->seekDoneFlag_ != true) {
            return -1;
        }
    }
    return ret;
}

int32_t Player_mock::SetVolume(float leftVolume, float rightVolume)
{
    return player_->SetVolume(leftVolume, rightVolume);
}

int32_t Player_mock::SetLooping(bool loop)
{
    return player_->SetLooping(loop);
}

int32_t Player_mock::GetCurrentTime(int32_t &currentTime)
{
    return player_->GetCurrentTime(currentTime);
}

bool Player_mock::IsPlaying()
{
    return player_->IsPlaying();
}

bool Player_mock::IsLooping()
{
    return player_->IsLooping();
}

int32_t Player_mock::SetPlayerCallback(const std::shared_ptr<PlayerCallback> &callback)
{
    return player_->SetPlayerCallback(callback);
}

int32_t Player_mock::SetVideoSurface(sptr<Surface> surface)
{
    return player_->SetVideoSurface(surface);
}

PlayerCallbackTest::PlayerCallbackTest(std::shared_ptr<PlayerSignal> test)
    : test_(test)
{

}

} // namespace Media
} // namespace OHOS
