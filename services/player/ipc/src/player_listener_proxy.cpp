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

#include "player_listener_proxy.h"
#include "media_log.h"
#include "errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerListenerProxy"};
}

namespace OHOS {
namespace Media {
PlayerListenerProxy::PlayerListenerProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardPlayerListener>(impl)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayerListenerProxy::~PlayerListenerProxy()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void PlayerListenerProxy::OnError(int32_t errorType, int32_t errorCode)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    data.WriteInt32(errorType);
    data.WriteInt32(errorCode);
    int error = Remote()->SendRequest(PlayerListenerMsg::ON_ERROR, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("on error failed, error: %{public}d", error);
    }
}

void PlayerListenerProxy::OnSeekDone(uint64_t currentPositon)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    data.WriteUint64(currentPositon);
    int error = Remote()->SendRequest(PlayerListenerMsg::ON_SEEK_DONE, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("on seek done failed, error: %{public}d", error);
    }
}

void PlayerListenerProxy::OnEndOfStream(bool isLooping)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    data.WriteBool(isLooping);
    int error = Remote()->SendRequest(PlayerListenerMsg::ON_END_OF_STREAM, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("on end of stream failed, error: %{public}d", error);
    }
}

void PlayerListenerProxy::OnStateChanged(PlayerStates state)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    data.WriteInt32(static_cast<int32_t>(state));
    int error = Remote()->SendRequest(PlayerListenerMsg::ON_STATE_CHANGED, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("on state changed failed, error: %{public}d", error);
    }
}

void PlayerListenerProxy::OnPositionUpdated(uint64_t postion)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    data.WriteUint64(postion);
    int error = Remote()->SendRequest(PlayerListenerMsg::ON_POSITION_UPDATED, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("on seek done failed, error: %{public}d", error);
    }
}

void PlayerListenerProxy::OnMessage(int32_t type, int32_t extra)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    data.WriteInt32(type);
    data.WriteInt32(extra);
    int error = Remote()->SendRequest(PlayerListenerMsg::ON_MESSAGE, data, reply, option);
    if (error != ERR_OK) {
        MEDIA_LOGE("on info failed, error: %{public}d", error);
    }
}

PlayerListenerCallback::PlayerListenerCallback(const sptr<IStandardPlayerListener> &listener,
                                               const sptr<MediaDeathRecipient> &deathRecipient)
    : listener_(listener), deathRecipient_(deathRecipient)
{
    if (listener_ != nullptr && listener_->AsObject() != nullptr && deathRecipient_ != nullptr) {
        (void)listener_->AsObject()->AddDeathRecipient(deathRecipient_);
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayerListenerCallback::~PlayerListenerCallback()
{
    if (listener_ != nullptr && listener_->AsObject() != nullptr && deathRecipient_ != nullptr) {
        listener_->AsObject()->RemoveDeathRecipient(deathRecipient_);
        deathRecipient_ = nullptr;
        listener_ = nullptr;
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destory", FAKE_POINTER(this));
}

void PlayerListenerCallback::OnError(int32_t errorType, int32_t errorCode)
{
    MEDIA_LOGE("player callback onError, errorType: %{public}d, errorCode: %{public}d", errorType, errorCode);
    if (listener_ != nullptr) {
        listener_->OnError(errorType, errorCode);
    }
}

void PlayerListenerCallback::OnSeekDone(uint64_t currentPositon)
{
    if (listener_ != nullptr) {
        listener_->OnSeekDone(currentPositon);
    }
}

void PlayerListenerCallback::OnEndOfStream(bool isLooping)
{
    if (listener_ != nullptr) {
        listener_->OnEndOfStream(isLooping);
    }
}

void PlayerListenerCallback::OnStateChanged(PlayerStates state)
{
    if (listener_ != nullptr) {
        MEDIA_LOGD("on state changed, state: %{public}d", state);
        listener_->OnStateChanged(state);
    }
}

void PlayerListenerCallback::OnPositionUpdated(uint64_t postion)
{
    if (listener_ != nullptr) {
        listener_->OnPositionUpdated(postion);
    }
}

void PlayerListenerCallback::OnMessage(int32_t type, int32_t extra)
{
    if (listener_ != nullptr) {
        listener_->OnMessage(type, extra);
    }
}
} // namespace Media
} // namespace OHOS