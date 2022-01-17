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

#ifndef PLAYER_DEMO_H
#define PLAYER_DEMO_H

#include "securec.h"
#include "player.h"
#include "surface.h"
#include "display_type.h"
#include "window_manager.h"
#include "nocopyable.h"
#include "media_data_source_demo.h"
#include "foundation/windowmanager/interfaces/innerkits/wm/window.h"

namespace MediaDemo {
    const int32_t HEIGHT = 360;
    const int32_t WIDTH = 640;
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
class PlayerDemo {
public:
    PlayerDemo() = default;
    virtual ~PlayerDemo() = default;
    DISALLOW_COPY_AND_MOVE(PlayerDemo);
    sptr<Surface> GetVideoSurface();
    void RunCase(const std::string &path);

private:
    void DoNext();
    void Seek(const std::string cmd);
    void SetLoop(const std::string cmd);
    void SetPlaybackSpeed(const std::string cmd) const;
    int32_t GetPlaying();
    int32_t GetLooping();
    void GetCurrentTime();
    int32_t GetVideoTrackInfo();
    int32_t GetAudioTrackInfo();
    int32_t GetTrackInfo();
    int32_t GetPlaybackSpeed() const;
    int32_t SetDataSrc(const std::string &path, bool seekable);
    int32_t SelectSource(const std::string &path);
    int32_t SelectBufferingOut();
    int32_t ChangeModeToSpeed(const PlaybackRateMode &mode, double &rate) const;
    int32_t ChangeSpeedToMode(const double &rate, PlaybackRateMode &mode) const;
    void RegisterTable();
    sptr<Window> mwindow_ = nullptr;
    sptr<Rosen::Window> previewWindow_ = nullptr;
    std::map<std::string, std::function<int32_t()>> playerTable_;
    std::shared_ptr<Player> player_ = nullptr;
    std::shared_ptr<MediaDataSourceDemo> dataSrc_ = nullptr;
};

class PlayerCallbackDemo : public PlayerCallback {
public:
    PlayerCallbackDemo() = default;
    virtual ~PlayerCallbackDemo() = default;
    DISALLOW_COPY_AND_MOVE(PlayerCallbackDemo);
    void OnError(PlayerErrorType errorType, int32_t errorCode) override;
    void OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody) override;
    void SetBufferingOut(int32_t bufferingOut);

private:
    void PrintState(PlayerStates state) const;
    void PrintResolution(const Format &infoBody) const;
    void PrintBufferingUpdate(const Format &infoBody) const;
    int32_t updateCount_ = 0;
    int32_t bufferingOut_ = 0;
    PlayerStates state_ = PLAYER_STATE_ERROR;
};
}
}
#endif
