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

#ifndef RECORDER_LISTENER_PROXY_H
#define RECORDER_LISTENER_PROXY_H

#include "i_standard_recorder_listener.h"
#include "media_death_recipient.h"
#include "recorder.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class RecorderListenerCallback : public RecorderCallback, public NoCopyable {
public:
    explicit RecorderListenerCallback(const sptr<IStandardRecorderListener> &listener);
    virtual ~RecorderListenerCallback();

    void OnError(RecorderErrorType errorType, int32_t errorCode) override;
    void OnInfo(int32_t type, int32_t extra) override;

private:
    sptr<IStandardRecorderListener> listener_ = nullptr;
};

class RecorderListenerProxy : public IRemoteProxy<IStandardRecorderListener>, public NoCopyable {
public:
    explicit RecorderListenerProxy(const sptr<IRemoteObject> &impl);
    virtual ~RecorderListenerProxy();

    void OnError(int32_t errorType, int32_t errorCode) override;
    void OnInfo(int32_t type, int32_t extra) override;

private:
    static inline BrokerDelegator<RecorderListenerProxy> delegator_;
};
}
} // namespace OHOS
#endif // RECORDER_LISTENER_PROXY_H
