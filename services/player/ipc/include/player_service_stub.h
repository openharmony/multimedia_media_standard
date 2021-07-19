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

#ifndef PLAYER_SERVICE_STUB_H
#define PLAYER_SERVICE_STUB_H

#include <map>
#include "i_standard_player_service.h"
#include "i_standard_player_listener.h"
#include "media_death_recipient.h"
#include "player_server.h"

namespace OHOS {
namespace Media {
class PlayerServiceStub : public IRemoteStub<IStandardPlayerService> {
public:
    static sptr<PlayerServiceStub> Create();
    virtual ~PlayerServiceStub();
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

    using PlayerStubFunc = int32_t(PlayerServiceStub::*)(MessageParcel &data, MessageParcel &reply);
    int32_t SetListenerObject(const sptr<IRemoteObject> &object) override;
    int32_t SetSource(const std::string &uri) override;
    int32_t Play() override;
    int32_t Prepare() override;
    int32_t PrepareAsync() override;
    int32_t Pause() override;
    int32_t Stop() override;
    int32_t Reset() override;
    int32_t Release() override;
    int32_t SetVolume(float leftVolume, float rightVolume) override;
    int32_t Seek(uint64_t mSeconds, int32_t mode) override;
    int32_t GetCurrentTime(uint64_t &currentTime) override;
    int32_t GetDuration(uint64_t &duration) override;
    int32_t SetPlaybackSpeed(int32_t mode) override;
    int32_t GetPlaybackSpeed(int32_t &mode) override;
    int32_t SetVideoSurface(sptr<Surface> surface) override;
    bool IsPlaying() override;
    bool IsLooping() override;
    int32_t SetLooping(bool loop) override;
    int32_t DestroyStub() override;
    int32_t SetPlayerCallback() override;

private:
    PlayerServiceStub();
    int32_t Init();
    void Destory();
    void ClientDied();
    int32_t SetListenerObject(MessageParcel &data, MessageParcel &reply);
    int32_t SetSource(MessageParcel &data, MessageParcel &reply);
    int32_t Play(MessageParcel &data, MessageParcel &reply);
    int32_t Prepare(MessageParcel &data, MessageParcel &reply);
    int32_t PrepareAsync(MessageParcel &data, MessageParcel &reply);
    int32_t Pause(MessageParcel &data, MessageParcel &reply);
    int32_t Stop(MessageParcel &data, MessageParcel &reply);
    int32_t Reset(MessageParcel &data, MessageParcel &reply);
    int32_t Release(MessageParcel &data, MessageParcel &reply);
    int32_t SetVolume(MessageParcel &data, MessageParcel &reply);
    int32_t Seek(MessageParcel &data, MessageParcel &reply);
    int32_t GetCurrentTime(MessageParcel &data, MessageParcel &reply);
    int32_t GetDuration(MessageParcel &data, MessageParcel &reply);
    int32_t SetPlaybackSpeed(MessageParcel &data, MessageParcel &reply);
    int32_t GetPlaybackSpeed(MessageParcel &data, MessageParcel &reply);
    int32_t SetVideoSurface(MessageParcel &data, MessageParcel &reply);
    int32_t IsPlaying(MessageParcel &data, MessageParcel &reply);
    int32_t IsLooping(MessageParcel &data, MessageParcel &reply);
    int32_t SetLooping(MessageParcel &data, MessageParcel &reply);
    int32_t DestroyStub(MessageParcel &data, MessageParcel &reply);
    int32_t SetPlayerCallback(MessageParcel &data, MessageParcel &reply);

private:
    std::shared_ptr<PlayerCallback> playerCallback_ = nullptr;
    std::shared_ptr<IPlayerService> playerServer_ = nullptr;
    sptr<MediaDeathRecipient> deathRecipient_ = nullptr;
    std::map<uint32_t, PlayerStubFunc> playerFuncs_;
    std::mutex mutex_;
};
}
} // namespace OHOS
#endif // PLAYER_SERVICE_STUB_H