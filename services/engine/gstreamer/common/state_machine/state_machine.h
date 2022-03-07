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

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <string>
#include <memory>
#include <mutex>
#include "nocopyable.h"
#include "inner_msg_define.h"

namespace OHOS {
namespace Media {
class StateMachine;

class State : public NoCopyable {
public:
    explicit State(const std::string &name) : name_(name) {}
    virtual ~State() = default;

    std::string GetStateName() const;

protected:
    // do not do the time-consuming operation when enter state
    virtual void StateEnter() {}
    // do not do the time-consuming operation when exit state
    virtual void StateExit() {}
    virtual void OnMessageReceived(const InnerMessage &msg) = 0;

    friend class StateMachine;

private:
    std::string name_;
};

class StateMachine : public NoCopyable {
public:
    StateMachine() = default;
    virtual ~StateMachine() = default;

protected:
    void HandleMessage(const InnerMessage &msg);
    void ChangeState(const std::shared_ptr<State> &state);
    std::shared_ptr<State> GetCurrState();

private:
    std::recursive_mutex recMutex_;
    std::shared_ptr<State> currState_;
};
} // namespace Media
} // namespace OHOS
#endif // STATE_MACHINE_H