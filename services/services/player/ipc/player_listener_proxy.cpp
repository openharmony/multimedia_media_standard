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
#include "media_errors.h"

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

void PlayerListenerProxy::OnError(PlayerErrorType errorType, int32_t errorCode)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    data.WriteInt32(errorType);
    data.WriteInt32(errorCode);
    int error = Remote()->SendRequest(PlayerListenerMsg::ON_ERROR, data, reply, option);
    if (error != MSERR_OK) {
        MEDIA_LOGE("on error failed, error: %{public}d", error);
    }
}

void PlayerListenerProxy::OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    data.WriteInt32(type);
    data.WriteInt32(extra);
    int error = Remote()->SendRequest(PlayerListenerMsg::ON_INFO, data, reply, option);
    if (error != MSERR_OK) {
        MEDIA_LOGE("on info failed, error: %{public}d", error);
    }
}

PlayerListenerCallback::PlayerListenerCallback(const sptr<IStandardPlayerListener> &listener) : listener_(listener)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayerListenerCallback::~PlayerListenerCallback()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destory", FAKE_POINTER(this));
}

void PlayerListenerCallback::OnError(PlayerErrorType errorType, int32_t errorCode)
{
    MEDIA_LOGE("player callback onError, errorType: %{public}d, errorCode: %{public}d", errorType, errorCode);
    if (listener_ != nullptr) {
        listener_->OnError(errorType, errorCode);
    }
}

void PlayerListenerCallback::OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    if (listener_ != nullptr) {
        listener_->OnInfo(type, extra, infoBody);
    }
}
} // namespace Media
} // namespace OHOS
