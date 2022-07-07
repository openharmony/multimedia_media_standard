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

#ifndef PLAYER_SERVER_STATE_H
#define PLAYER_SERVER_STATE_H

#include "player_server.h"

namespace OHOS {
namespace Media {
class PlayerServer::BaseState : public PlayerServerState {
public:
    BaseState(PlayerServer &server, const std::string &name) : PlayerServerState(name), server_(server) {}
    virtual ~BaseState() = default;

    virtual int32_t Prepare();
    virtual int32_t Play();
    virtual int32_t Pause();
    virtual int32_t Seek(int32_t mSeconds, PlayerSeekMode mode);
    virtual int32_t SetPlaybackSpeed(PlaybackRateMode mode);
    virtual int32_t Stop();

protected:
    int32_t OnMessageReceived(PlayerOnInfoType type, int32_t extra, const Format &infoBody) final;
    virtual void HandleStateChange(int32_t newState) {}
    virtual void HandlePlaybackComplete(int32_t extra) {}
    void ReportInvalidOperation() const;
    virtual void HandleEos() {}

    PlayerServer &server_;
};

class PlayerServer::IdleState : public PlayerServer::BaseState {
public:
    explicit IdleState(PlayerServer &server) : BaseState(server, "idle_state") {}
    ~IdleState() = default;

protected:
    void StateEnter() override;
};

class PlayerServer::InitializedState : public PlayerServer::BaseState {
public:
    explicit InitializedState(PlayerServer &server) : BaseState(server, "inited_state") {}
    ~InitializedState() = default;

    int32_t Prepare() override;
};

class PlayerServer::PreparingState : public PlayerServer::BaseState {
public:
    explicit PreparingState(PlayerServer &server) : BaseState(server, "preparing_state") {}
    ~PreparingState() = default;

    int32_t Stop() override;

protected:
    void HandleStateChange(int32_t newState) override;
    void StateEnter() override;
};

class PlayerServer::PreparedState : public PlayerServer::BaseState {
public:
    explicit PreparedState(PlayerServer &server) : BaseState(server, "prepared_state") {}
    ~PreparedState() = default;

    int32_t Prepare() override;
    int32_t Play() override;
    int32_t Seek(int32_t mSeconds, PlayerSeekMode mode) override;
    int32_t Stop() override;
    int32_t SetPlaybackSpeed(PlaybackRateMode mode) override;

protected:
    void HandleStateChange(int32_t newState) override;
};

class PlayerServer::PlayingState : public PlayerServer::BaseState {
public:
    explicit PlayingState(PlayerServer &server) : BaseState(server, "playing_state") {}
    ~PlayingState() = default;

    int32_t Play() override;
    int32_t Pause() override;
    int32_t Seek(int32_t mSeconds, PlayerSeekMode mode) override;
    int32_t Stop() override;
    int32_t SetPlaybackSpeed(PlaybackRateMode mode) override;

protected:
    void HandleStateChange(int32_t newState) override;
    void HandlePlaybackComplete(int32_t extra) override;
    void HandleEos() override;
};

class PlayerServer::PausedState : public PlayerServer::BaseState {
public:
    explicit PausedState(PlayerServer &server) : BaseState(server, "paused_state") {}
    ~PausedState() = default;

    int32_t Play() override;
    int32_t Pause() override;
    int32_t Seek(int32_t mSeconds, PlayerSeekMode mode) override;
    int32_t Stop() override;
    int32_t SetPlaybackSpeed(PlaybackRateMode mode) override;

protected:
    void HandleStateChange(int32_t newState) override;
};

class PlayerServer::StoppedState : public PlayerServer::BaseState {
public:
    explicit StoppedState(PlayerServer &server) : BaseState(server, "stopped_state") {}
    ~StoppedState() = default;

    int32_t Prepare() override;
    int32_t Stop() override;
};

class PlayerServer::PlaybackCompletedState : public PlayerServer::BaseState {
public:
    explicit PlaybackCompletedState(PlayerServer &server) : BaseState(server, "playbackCompleted_state") {}
    ~PlaybackCompletedState() = default;

    int32_t Play() override;
    int32_t Stop() override;

protected:
    void HandleStateChange(int32_t newState) override;
};
}
}
#endif // PLAYER_SERVER_STATE_H
