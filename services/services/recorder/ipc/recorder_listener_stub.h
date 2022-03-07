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

#ifndef RECORDER_LISTENER_STUB_H
#define RECORDER_LISTENER_STUB_H

#include "i_standard_recorder_listener.h"
#include "recorder.h"

namespace OHOS {
namespace Media {
class RecorderListenerStub : public IRemoteStub<IStandardRecorderListener> {
public:
    RecorderListenerStub();
    virtual ~RecorderListenerStub();
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
    void OnError(int32_t errorType, int32_t errorCode) override;
    void OnInfo(int32_t type, int32_t extra) override;
    void SetRecorderCallback(const std::shared_ptr<RecorderCallback> &callback);

private:
    std::shared_ptr<RecorderCallback> callback_ = nullptr;
};
} // namespace Media
} // namespace OHOS
#endif // RECORDER_LISTENER_STUB_H
