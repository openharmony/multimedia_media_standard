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

#ifndef AVCODEC_LISTENER_STUB_H
#define AVCODEC_LISTENER_STUB_H

#include "i_standard_avcodec_listener.h"
#include "avcodec_common.h"

namespace OHOS {
namespace Media {
class AVCodecListenerStub : public IRemoteStub<IStandardAVCodecListener> {
public:
    AVCodecListenerStub();
    virtual ~AVCodecListenerStub();
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format &format) override;
    void OnInputBufferAvailable(uint32_t index) override;
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) override;
    void SetCallback(const std::shared_ptr<AVCodecCallback> &callback);

private:
    std::shared_ptr<AVCodecCallback> callback_ = nullptr;
};
} // namespace Media
} // namespace OHOS
#endif // AVCODEC_LISTENER_STUB_H
