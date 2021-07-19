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
#ifndef PLAYER_IMPL_H
#define PLAYER_IMPL_H

#include "player.h"
#include "nocopyable.h"
#include "i_player_service.h"

namespace OHOS {
namespace Media {
class PlayerImpl : public Player {
public:
    PlayerImpl();
    ~PlayerImpl();
    DISALLOW_COPY_AND_MOVE(PlayerImpl);

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
    int32_t SetPlayerCallback(const std::shared_ptr<PlayerCallback> &callback) override;
    int32_t Init();
private:
    std::shared_ptr<IPlayerService> playerService_ = nullptr;
};
} // Media
} // OHOS

#endif // PLAYER_IMPL_H