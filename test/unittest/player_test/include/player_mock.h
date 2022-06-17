/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef PLAYER_TEST_H
#define PLAYER_TEST_H

#include "player.h"
#include "TestParamsConfig.h"
#include "window.h"
#include "unordered_map"
namespace OHOS {
namespace Media{ 

class PlayerSignal {
public:
    PlayerStates state_ = PLAYER_IDLE;
    int32_t seekPosition_;
    bool seekDoneFlag_;
    PlayerSeekMode seekMode_ = PlayerSeekMode::SEEK_CLOSEST;
    std::mutex mutexPrepare_;
    std::mutex mutexPlay_;
    std::mutex mutexPause_;
    std::mutex mutexStop_;
    std::mutex mutexReset_;
    std::mutex mutexSeek_;
    std::condition_variable condVarPrepare_;
    std::condition_variable condVarPlay_;
    std::condition_variable condVarPause_;   
    std::condition_variable condVarStop_;   
    std::condition_variable condVarReset_;
    std::condition_variable condVarSeek_;    
    //std::unordered_multimap<PlayerStates, std::condition_variable> condVarMultiMap_;
    void SetState(PlayerStates state);
    void SetSeekResult(bool seekDoneFlag);
    //explicit PlayerSignal(std::unordered_multimap condVarMultiMap);
};

class Player_mock : public NoCopyable{
public:
    std::shared_ptr<Player> player_ = nullptr;
    sptr<Rosen::Window> window_ = nullptr;
    sptr<Rosen::Window> previewWindow_ = nullptr;
    explicit Player_mock(std::shared_ptr<PlayerSignal> test);
    virtual ~Player_mock();
    bool CreatePlayer();
    int32_t SetSource(const std::string url);
    int32_t Prepare();
    int32_t PrepareAsync();
    int32_t Play();
    int32_t Pause();
    int32_t Stop();
    int32_t Reset();
    int32_t Release();
    int32_t Seek(int32_t mseconds, PlayerSeekMode mode);
    int32_t SetVolume(float leftVolume, float rightVolume);
    int32_t SetLooping(bool loop);
    int32_t GetCurrentTime(int32_t &currentTime);
    bool IsPlaying();
    bool IsLooping();
    int32_t SetPlayerCallback(const std::shared_ptr<PlayerCallback> &callback);
    int32_t SetVideoSurface(sptr<Surface> surface);
    sptr<Surface> GetVideoSurface();
private:
    std::shared_ptr<PlayerSignal> test_;
    int32_t height = 1080;
    int32_t width = 1920;
};    

class PlayerCallbackTest : public PlayerCallback, public NoCopyable {
public:
    int32_t position_ = 0;
    PlayerStates state_ = PLAYER_STATE_ERROR;
    explicit PlayerCallbackTest(std::shared_ptr<PlayerSignal> test);
    ~PlayerCallbackTest() {}
    void OnError(PlayerErrorType errorType, int32_t errorCode) override {}
    void OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody) override;
    void SeekNotify(int32_t extra, const Format &infoBody);
    void notify(PlayerStates currentState);
private:
    std::shared_ptr<PlayerSignal> test_;
    bool seekDoneFlag_ = false;
};


} // namespace Media
} // namespace OHOS
#endif