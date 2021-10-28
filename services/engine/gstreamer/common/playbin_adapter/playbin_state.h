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

#ifndef PLAYBIN_STATE_H
#define PLAYBIN_STATE_H

#include "playbin_ctrler_base.h"
#include "state_machine.h"
#include "inner_msg_define.h"

namespace OHOS {
namespace Media {
class PlayBinCtrlerBase::BaseState : public State {
public:
    BaseState(PlayBinCtrlerBase &ctrler, const std::string &name) : State(name), ctrler_(ctrler) {}
    virtual ~BaseState() = default;

    virtual int32_t Prepare();
    virtual int32_t Play();
    virtual int32_t Pause();
    virtual int32_t Seek(int64_t timeUs, int32_t option);
    virtual int32_t Stop();

protected:
    void OnMessageReceived(const InnerMessage &msg) final;
    virtual void ProcessMessage(const InnerMessage &msg) {}
    void ReportInvalidOperation();
    int32_t ChangePlayBinState(GstState targetState);

    PlayBinCtrlerBase &ctrler_;
};

class PlayBinCtrlerBase::IdleState : public PlayBinCtrlerBase::BaseState {
public:
    explicit IdleState(PlayBinCtrlerBase &ctrler) : BaseState(ctrler, "idle_state") {}
    ~IdleState() = default;

protected:
    void StateEnter() override;
};

class PlayBinCtrlerBase::InitializedState : public PlayBinCtrlerBase::BaseState {
public:
    explicit InitializedState(PlayBinCtrlerBase &ctrler) : BaseState(ctrler, "inited_state") {}
    ~InitializedState() = default;

    int32_t Prepare() override;
};

class PlayBinCtrlerBase::PreparingState : public PlayBinCtrlerBase::BaseState {
public:
    explicit PreparingState(PlayBinCtrlerBase &ctrler) : BaseState(ctrler, "preparing_state") {}
    ~PreparingState() = default;

    int32_t Stop() override;

protected:
    void ProcessMessage(const InnerMessage &msg) override;
    void StateEnter() override;
    void StateExit() override;
};

class PlayBinCtrlerBase::PreparedState : public PlayBinCtrlerBase::BaseState {
public:
    explicit PreparedState(PlayBinCtrlerBase &ctrler) : BaseState(ctrler, "prepared_state") {}
    ~PreparedState() = default;

    int32_t Prepare() override;
    int32_t Play() override;
    int32_t Seek(int64_t timeUs, int32_t option) override;
    int32_t Stop() override;

protected:
    void ProcessMessage(const InnerMessage &msg) override;
    void StateEnter() override;
};

class PlayBinCtrlerBase::PlayingState : public PlayBinCtrlerBase::BaseState {
public:
    explicit PlayingState(PlayBinCtrlerBase &ctrler) : BaseState(ctrler, "playing_state") {}
    ~PlayingState() = default;

    int32_t Play() override;
    int32_t Pause() override;
    int32_t Seek(int64_t timeUs, int32_t option) override;
    int32_t Stop() override;

protected:
    void ProcessMessage(const InnerMessage &msg) override;
    void StateEnter() override;

    bool seekPending_ = false;
};

class PlayBinCtrlerBase::PausedState : public PlayBinCtrlerBase::BaseState {
public:
    explicit PausedState(PlayBinCtrlerBase &ctrler) : BaseState(ctrler, "paused_state") {}
    ~PausedState() = default;

    int32_t Play() override;
    int32_t Pause() override;
    int32_t Seek(int64_t timeUs, int32_t option) override;
    int32_t Stop() override;

protected:
    void ProcessMessage(const InnerMessage &msg) override;
    void StateEnter() override;
};

class PlayBinCtrlerBase::StoppedState : public PlayBinCtrlerBase::BaseState {
public:
    explicit StoppedState(PlayBinCtrlerBase &ctrler) : BaseState(ctrler, "stopped_state") {}
    ~StoppedState() = default;

    int32_t Prepare() override;
    int32_t Stop() override;

protected:
    void ProcessMessage(const InnerMessage &msg) override;
    void StateEnter() override;
};
}
}

#endif