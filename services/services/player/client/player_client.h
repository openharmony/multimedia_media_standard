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

#ifndef PLAYER_SERVICE_CLIENT_H
#define PLAYER_SERVICE_CLIENT_H

#include "i_player_service.h"
#include "i_standard_player_service.h"
#include "player_listener_stub.h"

namespace OHOS {
namespace Media {
class PlayerClient : public IPlayerService {
public:
    static std::shared_ptr<PlayerClient> Create(const sptr<IStandardPlayerService> &ipcProxy);
    explicit PlayerClient(const sptr<IStandardPlayerService> &ipcProxy);
    ~PlayerClient();

    // IPlayerService override
    int32_t SetSource(const std::string &uri) override;
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
    int32_t GetDuration(int32_t &duration) override;
    int32_t SetPlaybackSpeed(PlaybackRateMode mode) override;
    int32_t GetPlaybackSpeed(PlaybackRateMode &mode) override;
    int32_t SetVideoSurface(sptr<Surface> surface) override;
    bool IsPlaying() override;
    bool IsLooping() override;
    int32_t SetLooping(bool loop) override;
    int32_t SetPlayerCallback(const std::shared_ptr<PlayerCallback> &callback) override;

    // PlayerClient
    void MediaServerDied();

private:
    int32_t CreateListenerObject();

private:
    sptr<IStandardPlayerService> playerProxy_ = nullptr;
    sptr<PlayerListenerStub> listenerStub_ = nullptr;
    std::shared_ptr<PlayerCallback> callback_ = nullptr;
    std::mutex mutex_;
};
} // namespace Media
} // namespace OHOS
#endif // PLAYER_SERVICE_CLIENT_H
