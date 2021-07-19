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

#ifndef PLAYER_LISTENER_STUB_H
#define PLAYER_LISTENER_STUB_H

#include "i_standard_player_listener.h"
#include "player.h"

namespace OHOS {
namespace Media {
class PlayerListenerStub : public IRemoteStub<IStandardPlayerListener> {
public:
    PlayerListenerStub();
    virtual ~PlayerListenerStub();
    // IStandardPlayerListener override
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
    void OnError(int32_t errorType, int32_t errorCode) override;
    void OnSeekDone(uint64_t currentPositon) override;
    void OnEndOfStream(bool isLooping) override;
    void OnStateChanged(PlayerStates state) override;
    void OnMessage(int32_t type, int32_t extra) override;
    void OnPositionUpdated(uint64_t postion) override;

    // PlayerListenerStub
    void SetPlayerCallback(const std::weak_ptr<PlayerCallback> &callback);

private:
    std::weak_ptr<PlayerCallback> callback_;
};
}
} // namespace OHOS
#endif // PLAYER_LISTENER_STUB_H