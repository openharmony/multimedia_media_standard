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

#ifndef I_STANDARD_PLAYER_SERVICE_H
#define I_STANDARD_PLAYER_SERVICE_H

#include "ipc_types.h"
#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"
#include "player.h"

namespace OHOS {
namespace Media {
class IStandardPlayerService : public IRemoteBroker {
public:
    virtual ~IStandardPlayerService() = default;
    virtual int32_t SetListenerObject(const sptr<IRemoteObject> &object) = 0;
    virtual int32_t SetSource(const std::string &uri) = 0;
    virtual int32_t SetSource(const sptr<IRemoteObject> &object) = 0;
    virtual int32_t Play() = 0;
    virtual int32_t Prepare() = 0;
    virtual int32_t PrepareAsync() = 0;
    virtual int32_t Pause() = 0;
    virtual int32_t Stop() = 0;
    virtual int32_t Reset() = 0;
    virtual int32_t Release() = 0;
    virtual int32_t SetVolume(float leftVolume, float rightVolume) = 0;
    virtual int32_t Seek(int32_t mSeconds, PlayerSeekMode mode) = 0;
    virtual int32_t GetCurrentTime(int32_t &currentTime) = 0;
    virtual int32_t GetDuration(int32_t &duration) = 0;
    virtual int32_t SetPlaybackSpeed(PlaybackRateMode mode) = 0;
    virtual int32_t GetPlaybackSpeed(PlaybackRateMode &mode) = 0;
    virtual int32_t SetVideoSurface(sptr<Surface> surface) = 0;
    virtual bool IsPlaying() = 0;
    virtual bool IsLooping() = 0;
    virtual int32_t SetLooping(bool loop) = 0;
    virtual int32_t DestroyStub() = 0;
    virtual int32_t SetPlayerCallback() = 0;

    /**
     * IPC code ID
     */
    enum PlayerServiceMsg {
        SET_LISTENER_OBJ = 0,
        SET_SOURCE,
        SET_MEDIA_DATA_SRC_OBJ,
        PLAY,
        PREPARE,
        PREPAREASYNC,
        PAUSE,
        STOP,
        RESET,
        RELEASE,
        SET_VOLUME,
        SEEK,
        GET_CURRENT_TIME,
        GET_DURATION,
        SET_PLAYERBACK_SPEED,
        GET_PLAYERBACK_SPEED,
        SET_VIDEO_SURFACE,
        IS_PLAYING,
        IS_LOOPING,
        SET_LOOPING,
        DESTROY,
        SET_CALLBACK,
    };

    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardPlayerService");
};
} // namespace Media
} // namespace OHOS
#endif // I_STANDARD_PLAYER_SERVICE_H
