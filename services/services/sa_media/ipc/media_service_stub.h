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

#ifndef MEDIA_SERVICE_STUB_H
#define MEDIA_SERVICE_STUB_H

#include <map>
#include "i_standard_media_service.h"
#include "i_standard_media_listener.h"
#include "media_death_recipient.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class MediaServiceStub : public IRemoteStub<IStandardMediaService>, public NoCopyable {
public:
    MediaServiceStub();
    virtual ~MediaServiceStub();

    using MediaStubFunc = int32_t(MediaServiceStub::*)(MessageParcel &data, MessageParcel &reply);
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

protected:
    int32_t SetDeathListener(const sptr<IRemoteObject> &object);

private:
    void Init();
    int32_t GetSystemAbility(MessageParcel &data, MessageParcel &reply);
    void ClientDied(pid_t pid);
    int32_t DestroyStubForPid(pid_t pid);

    std::map<pid_t, sptr<MediaDeathRecipient>> deathRecipientMap_;
    std::map<pid_t, sptr<IStandardMediaListener>> mediaListenerMap_;
    std::map<uint32_t, MediaStubFunc> mediaFuncs_;
    std::mutex mutex_;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_SERVICE_STUB_H
