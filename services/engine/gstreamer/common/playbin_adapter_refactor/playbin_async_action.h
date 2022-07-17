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

#ifndef PLAYBIN_ASYNC_ACTION_H
#define PLAYBIN_ASYNC_ACTION_H

#include <gst/gst.h>
#include "i_playbin_ctrler.h"
#include "playbin_msg_define.h"

namespace OHOS {
namespace Media {
namespace PlayBin {
enum ActionType {
    ACTION_CHANGE_STATE,
    ACTION_SEEK,
    ACTION_SET_SPEED,
};

class ActionObserver {
public:
    virtual ~ActionObserver() = default;

    virtual void OnActionDone(const AsyncAction &doneAction) = 0;
};

class AsyncAction {
public:
    AsyncAction(ActionType type, ActionObserver& observer)
        : type_(type), observer_(observer) {}
    virtual ~AsyncAction() = default;

    virtual int32_t Execute() = 0;
    virtual void HandleMessage(const InnerMessage &inMsg) = 0;

    ActionType GetType()
    {
        return type_;
    }

protected:
    ActionType type_;
    ActionObserver &observer_;
};

class StateOperator {
public:
    virtual ~StateOperator() = default;
    virtual PlayBinState GetPlayBinState() const = 0;
    virtual int32_t SetPlayBinState(PlayBinState targetState) = 0;
    virtual GstState GetGstState() const = 0;
    virtual int32_t SetGstState(GstState targetState) = 0;
};

class ChangeStateAction : public AsyncAction {
public:
    ChangeStateAction(StateOperator &stateOperator, PlayBinState targetState, ActionObserver& observer)
        : AsyncAction(ActionType::ACTION_CHANGE_STATE, observer),
          stateOperator_(stateOperator),
          targetState_(targetState)
    {}
    ~ChangeStateAction() = default;

    int32_t Execute() override;
    void HandleMessage(const InnerMessage &inMsg) override;

private:
    PlayBinState targetState_;
    StateOperator &stateOperator_;
    GstState startGstState_ = GST_STATE_VOID_PENDING;
    GstState targetGstState_ = GST_STATE_VOID_PENDING;
    PlayBinState startState_ = PLAYBIN_STATE_BUTT;
};

class EventMsgSender {
public:
    virtual ~EventMsgSender() = default;
    virtual int32_t SendSeekEvent(int64_t position, IPlayBinCtrler::SeekMode mode) = 0;
    virtual int32_t SendSpeedEvent(double rate) = 0;
    virtual void SendMessage(const InnerMessage &msg) = 0;
};

class SeekAction : public AsyncAction {
public:
    SeekAction(const StateOperator &stateOperator, EventMsgSender &sender,
        int64_t position, IPlayBinCtrler::SeekMode mode, ActionObserver& observer)
        : AsyncAction(ActionType::ACTION_SEEK, observer),
          stateOperator_(stateOperator)
          sender_(sender),
          position_(position),
          mode_(mode)
    {}
    ~SeekAction() = default;

    int32_t Execute() override;
    void HandleMessage(const InnerMessage &inMsg) override;

private:
    const StateOperator stateOperator_;
    EventMsgSender &sender_;
    int64_t position_;
    IPlayBinCtrler::SeekMode mode_;
};

class SetSpeedAction : public AsyncAction {
public:
    SetSpeedAction(const StateOperator &stateOperator, EventMsgSender &sender, double speed, ActionObserver& observer)
        : AsyncAction(ActionType::ACTION_SET_SPEED, observer),
          stateOperator_(stateOperator)
          sender_(sender),
          speed_(speed)
    {}
    ~SetSpeedAction() = default;

    int32_t Execute() override;
    void HandleMessage(const InnerMessage &inMsg) override;

private:
    const StateOperator stateOperator_;
    EventMsgSender &sender_;
    double speed_;
};
}
}
}

#endif