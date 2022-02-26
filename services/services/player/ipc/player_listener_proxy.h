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
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class PlayerListenerCallback : public PlayerCallback, public NoCopyable {
public:
    explicit PlayerListenerCallback(const sptr<IStandardPlayerListener> &listener);
    virtual ~PlayerListenerCallback();

    void OnError(PlayerErrorType errorType, int32_t errorCode) override;
    void OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody = {}) override;

private:
    sptr<IStandardPlayerListener> listener_ = nullptr;
};

class PlayerListenerProxy : public IRemoteProxy<IStandardPlayerListener>, public NoCopyable {
public:
    explicit PlayerListenerProxy(const sptr<IRemoteObject> &impl);
    virtual ~PlayerListenerProxy();

    void OnError(PlayerErrorType errorType, int32_t errorCode) override;
    void OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody = {}) override;

private:
    static inline BrokerDelegator<PlayerListenerProxy> delegator_;
};
}
} // namespace OHOS
#endif // PLAYER_LISTENER_PROXY_H
