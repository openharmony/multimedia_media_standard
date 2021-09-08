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

#include "media_service_proxy.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaServiceProxy"};
}

namespace OHOS {
namespace Media {
MediaServiceProxy::MediaServiceProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardMediaService>(impl)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaServiceProxy::~MediaServiceProxy()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

sptr<IRemoteObject> MediaServiceProxy::GetSubSystemAbility(IStandardMediaService::MediaSystemAbility subSystemId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(static_cast<int32_t>(subSystemId));
    int error = Remote()->SendRequest(MediaServiceMsg::GET_SUBSYSTEM, data, reply, option);
    if (error != MSERR_OK) {
        MEDIA_LOGE("Create player proxy failed, error: %{public}d", error);
        return nullptr;
    }

    return reply.ReadRemoteObject();
}

int32_t MediaServiceProxy::SetListenerObject(const sptr<IRemoteObject> &object)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    (void)data.WriteRemoteObject(object);
    int error = Remote()->SendRequest(SET_LISTENER_OBJ, data, reply, option);
    if (error != MSERR_OK) {
        MEDIA_LOGE("Set listener obj failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}
} // namespace Media
} // namespace OHOS
