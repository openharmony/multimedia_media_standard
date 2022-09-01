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

#include "state_machine.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "StateMachine"};
}

namespace OHOS {
namespace Media {
std::string State::GetStateName() const
{
    return name_;
}

void StateMachine::HandleMessage(const InnerMessage &msg)
{
    if (currState_ != nullptr) {
        currState_->OnMessageReceived(msg);
    }
}

void StateMachine::ChangeState(const std::shared_ptr<State> &state)
{
    {
        std::unique_lock<std::recursive_mutex> lock(recMutex_);

        if (state == nullptr || (state == currState_)) {
            return;
        }

        if (currState_ != nullptr && currState_->GetStateName() == "stopping_state" &&
            state->GetStateName() != "stopped_state") {
            MEDIA_LOGW("now is stopping change state to %{public}s fail", state->name_.c_str());
            return;
        }

        if (currState_) {
            MEDIA_LOGD("exit state %{public}s", currState_->name_.c_str());
            currState_->StateExit();
        }

        MEDIA_LOGI("change state to %{public}s", state->name_.c_str());
        currState_ = state;
    }
    state->StateEnter();
}

std::shared_ptr<State> StateMachine::GetCurrState()
{
    std::unique_lock<std::recursive_mutex> lock(recMutex_);
    return currState_;
}
} // namespace Media
} // namespace OHOS