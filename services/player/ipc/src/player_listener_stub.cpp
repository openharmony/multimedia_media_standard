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

#include "player_listener_stub.h"
#include "media_log.h"
#include "errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerListenerStub"};
}

namespace OHOS {
namespace Media {
PlayerListenerStub::PlayerListenerStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayerListenerStub::~PlayerListenerStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int PlayerListenerStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    switch (code) {
        case PlayerListenerMsg::ON_ERROR: {
            int32_t errorType = data.ReadInt32();
            int32_t errorCode = data.ReadInt32();
            OnError(errorType, errorCode);
            return ERR_OK;
        }
        case ON_SEEK_DONE: {
            uint64_t currentPositon = data.ReadUint64();
            OnSeekDone(currentPositon);
            return ERR_OK;
        }
        case ON_END_OF_STREAM: {
            bool isLooping = data.ReadBool();
            OnEndOfStream(isLooping);
            return ERR_OK;
        }
        case ON_STATE_CHANGED: {
            int32_t state = data.ReadInt32();
            OnStateChanged(static_cast<PlayerStates>(state));
            return ERR_OK;
        }
        case ON_POSITION_UPDATED: {
            uint64_t position = data.ReadUint64();
            OnPositionUpdated(position);
            return ERR_OK;
        }
        case ON_MESSAGE: {
            int32_t type = data.ReadInt32();
            int32_t extra = data.ReadInt32();
            OnMessage(type, extra);
            return ERR_OK;
        }
        default: {
            MEDIA_LOGE("default case, need check PlayerListenerStub");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }
}

void PlayerListenerStub::OnError(int32_t errorType, int32_t errorCode)
{
    std::shared_ptr<PlayerCallback> cb = callback_.lock();
    if (cb != nullptr) {
        cb->OnError(errorType, errorCode);
    }
}

void PlayerListenerStub::OnSeekDone(uint64_t currentPositon)
{
    std::shared_ptr<PlayerCallback> cb = callback_.lock();
    if (cb != nullptr) {
        cb->OnSeekDone(currentPositon);
    }
}

void PlayerListenerStub::OnEndOfStream(bool isLooping)
{
    std::shared_ptr<PlayerCallback> cb = callback_.lock();
    if (cb != nullptr) {
        cb->OnEndOfStream(isLooping);
    }
}

void PlayerListenerStub::OnStateChanged(PlayerStates state)
{
    std::shared_ptr<PlayerCallback> cb = callback_.lock();
    if (cb != nullptr) {
        MEDIA_LOGD("on state changed, state: %{public}d", state);
        cb->OnStateChanged(state);
    }
}

void PlayerListenerStub::OnPositionUpdated(uint64_t position)
{
    std::shared_ptr<PlayerCallback> cb = callback_.lock();
    if (cb != nullptr) {
        cb->OnPositionUpdated(position);
    }
}

void PlayerListenerStub::OnMessage(int32_t type, int32_t extra)
{
    std::shared_ptr<PlayerCallback> cb = callback_.lock();
    if (cb != nullptr) {
        cb->OnMessage(type, extra);
    }
}

void PlayerListenerStub::SetPlayerCallback(const std::weak_ptr<PlayerCallback> &callback)
{
    callback_ = callback;
}
} // namespace Media
} // namespace OHOS