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

#include "media_service_stub.h"
#include "media_log.h"
#include "errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaServiceStub"};
}

namespace OHOS {
namespace Media {
MediaServiceStub::MediaServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaServiceStub::~MediaServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int MediaServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    switch (code) {
        case MediaServiceMsg::GET_SUBSYSTEM: {
            MEDIA_LOGI("GET_SUBSYSTEM");
            MediaSystemAbility id = static_cast<MediaSystemAbility>(data.ReadInt32());
            sptr<IRemoteObject> object = GetSubSystemAbility(id);
            if (object == nullptr) {
                MEDIA_LOGE("failed to get subsystem service object");
                return ERR_NO_MEMORY;
            }
            (void)reply.WriteRemoteObject(object);
            return ERR_OK;
        }
        default: {
            MEDIA_LOGE("default case, need check MediaServiceStub");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }
}
} // namespace Media
} // namespace OHOS