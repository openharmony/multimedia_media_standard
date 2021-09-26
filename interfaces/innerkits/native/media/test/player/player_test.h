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

#ifndef PLAYER_TEST_H
#define PLAYER_TEST_H

#include "securec.h"
#include "player.h"
#include "surface.h"
#include "display_type.h"
#include "window_manager.h"
#include "nocopyable.h"

namespace MediaTest {
    const int32_t HEIGHT = 720;
    const int32_t WIDTH = 1280;
    const int32_t POSITION_UPDATE_INTERVAL = 100;
    const std::map<OHOS::Media::PlayerStates, std::string> STATE_MAP = {
        {OHOS::Media::PlayerStates::PLAYER_STATE_ERROR, "Error"},
        {OHOS::Media::PlayerStates::PLAYER_IDLE, "Idle"},
        {OHOS::Media::PlayerStates::PLAYER_INITIALIZED, "Initialized"},
        {OHOS::Media::PlayerStates::PLAYER_PREPARED, "Prepared"},
        {OHOS::Media::PlayerStates::PLAYER_STARTED, "Started"},
        {OHOS::Media::PlayerStates::PLAYER_PAUSED, "Paused"},
        {OHOS::Media::PlayerStates::PLAYER_STOPPED, "Stopped"},
        {OHOS::Media::PlayerStates::PLAYER_PLAYBACK_COMPLETE, "Complete"},
    };
};

namespace OHOS {
namespace Media {
class PlayerTest {
public:
    PlayerTest() = default;
    virtual ~PlayerTest() = default;
    DISALLOW_COPY_AND_MOVE(PlayerTest);
    sptr<Surface> GetVideoSurface();
    void TestCase(const std::string &path);

private:
    void DoNext();
    void Seek(const std::string cmd);
    void SetLoop(const std::string cmd);
    void SetPlaybackSpeed(const std::string cmd);
    int32_t GetPlaying();
    int32_t GetLooping();
    int32_t GetPlaybackSpeed();
    int32_t SetDataSrc(const std::string &path, bool seekable);
    int32_t SelectSource(const std::string &path);
    int32_t ChangeModeToSpeed(const PlaybackRateMode &mode, double &rate) const;
    int32_t ChangeSpeedToMode(const double &rate, PlaybackRateMode &mode) const;
    void RegisterTable();
    sptr<Window> mwindow_ = nullptr;
    std::map<std::string, std::function<int32_t()>> playerTable_;
    std::shared_ptr<Player> player_ = nullptr;
};

class PlayerCallbackTest : public PlayerCallback {
public:
    PlayerCallbackTest() = default;
    virtual ~PlayerCallbackTest() = default;
    DISALLOW_COPY_AND_MOVE(PlayerCallbackTest);
    void OnError(PlayerErrorType errorType, int32_t errorCode) override;
    void OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody = {}) override;

private:
    void PrintState(PlayerStates state) const;
    int32_t updateCount_ = 0;
    PlayerStates state_ = PLAYER_STATE_ERROR;
};
}
}
#endif
