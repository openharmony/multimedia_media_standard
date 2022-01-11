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

#ifndef PLAYER_SERVICE_SERVER_H
#define PLAYER_SERVICE_SERVER_H

#include "i_player_service.h"
#include "i_player_engine.h"
#include "time_monitor.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class PlayerServer : public IPlayerService, public IPlayerEngineObs {
public:
    static std::shared_ptr<IPlayerService> Create();
    PlayerServer();
    virtual ~PlayerServer();
    DISALLOW_COPY_AND_MOVE(PlayerServer);

    int32_t SetSource(const std::string &url) override;
    int32_t SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc) override;
    int32_t Play() override;
    int32_t Prepare() override;
    int32_t PrepareAsync() override;
    int32_t Pause() override;
    int32_t Stop() override;
    int32_t Reset() override;
    int32_t Release() override;
    int32_t SetVolume(float leftVolume, float rightVolume) override;
    int32_t Seek(int32_t mSeconds, PlayerSeekMode mode) override;
    int32_t GetCurrentTime(int32_t &currentTime) override;
    int32_t GetVideoTrackInfo(std::vector<Format> &videoTrack) override;
    int32_t GetAudioTrackInfo(std::vector<Format> &audioTrack) override;
    int32_t GetVideoWidth() override;
    int32_t GetVideoHeight() override;
    int32_t GetDuration(int32_t &duration) override;
    int32_t SetPlaybackSpeed(PlaybackRateMode mode) override;
    int32_t GetPlaybackSpeed(PlaybackRateMode &mode) override;
    int32_t SetVideoSurface(sptr<Surface> surface) override;
    bool IsPlaying() override;
    bool IsLooping() override;
    int32_t SetLooping(bool loop) override;
    int32_t SetParameter(const Format &param) override;
    int32_t SetPlayerCallback(const std::shared_ptr<PlayerCallback> &callback) override;

    // IPlayerEngineObs override
    void OnError(PlayerErrorType errorType, int32_t errorCode) override;
    void OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody = {}) override;

private:
    int32_t Init();
    bool IsValidSeekMode(PlayerSeekMode mode);
    int32_t OnReset();
    int32_t InitPlayEngine(const std::string &url);
    int32_t OnPrepare(bool async);

    std::unique_ptr<IPlayerEngine> playerEngine_ = nullptr;
    std::shared_ptr<PlayerCallback> playerCb_ = nullptr;
    sptr<Surface> surface_ = nullptr;
    PlayerStates status_ = PLAYER_IDLE;
    std::mutex mutex_;
    std::mutex mutexCb_;
    bool looping_ = false;
    TimeMonitor startTimeMonitor_;
    TimeMonitor stopTimeMonitor_;
    std::shared_ptr<IMediaDataSource> dataSrc_ = nullptr;
    float leftVolume_ = 1.0f; // audiotrack volume range [0, 1]
    float rightVolume_ = 1.0f; // audiotrack volume range [0, 1]
    PlaybackRateMode speedMode_ = SPEED_FORWARD_1_00_X;
};
} // namespace Media
} // namespace OHOS
#endif // PLAYER_SERVICE_SERVER_H
