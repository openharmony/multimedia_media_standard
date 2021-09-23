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

#ifndef MEDIA_DEATH_RECIPIENT_H
#define MEDIA_DEATH_RECIPIENT_H

#include "iremote_object.h"

namespace OHOS {
namespace Media {
class MediaDeathRecipient : public IRemoteObject::DeathRecipient {
public:
    explicit MediaDeathRecipient(pid_t pid) : pid_(pid) {};
    virtual ~MediaDeathRecipient() = default;
    void OnRemoteDied(const wptr<IRemoteObject> &remote)
    {
        (void)remote;
        if (diedCb_ != nullptr) {
            diedCb_(pid_);
        }
    }
    using NotifyCbFunc = std::function<void(pid_t)>;
    void SetNotifyCb(NotifyCbFunc func)
    {
        diedCb_ = func;
    }

private:
    pid_t pid_ = 0;
    NotifyCbFunc diedCb_ = nullptr;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_DEATH_RECIPIENT_H
