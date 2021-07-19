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

#ifndef PLAYER_LISTENER_PROXY_H
#define PLAYER_LISTENER_PROXY_H

#include "i_standard_player_listener.h"
#include "media_death_recipient.h"
#include "player_server.h"

namespace OHOS {
namespace Media {
class PlayerListenerCallback : public PlayerCallback {
public:
    explicit PlayerListenerCallback(const sptr<IStandardPlayerListener> &listener,
                                    const sptr<MediaDeathRecipient> &deathRecipient);
    virtual ~PlayerListenerCallback();
    void OnError(int32_t errorType, int32_t errorCode) override;
    void OnSeekDone(uint64_t currentPositon) override;
    void OnEndOfStream(bool isLooping) override;
    void OnStateChanged(PlayerStates state) override;
    void OnMessage(int32_t type, int32_t extra) override;
    void OnPositionUpdated(uint64_t postion) override;

private:
    sptr<IStandardPlayerListener> listener_ = nullptr;
    sptr<MediaDeathRecipient> deathRecipient_ = nullptr;
};

class PlayerListenerProxy : public IRemoteProxy<IStandardPlayerListener> {
public:
    explicit PlayerListenerProxy(const sptr<IRemoteObject> &impl);
    virtual ~PlayerListenerProxy();
    void OnError(int32_t errorType, int32_t errorCode) override;
    void OnSeekDone(uint64_t currentPositon) override;
    void OnEndOfStream(bool isLooping) override;
    void OnStateChanged(PlayerStates state) override;
    void OnMessage(int32_t type, int32_t extra) override;
    void OnPositionUpdated(uint64_t postion) override;

private:
    static inline BrokerDelegator<PlayerListenerProxy> delegator_;
};
}
} // namespace OHOS
#endif // PLAYER_LISTENER_PROXY_H