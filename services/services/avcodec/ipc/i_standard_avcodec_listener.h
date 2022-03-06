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

#ifndef I_STANDARD_AVCODEC_LISTENER_H
#define I_STANDARD_AVCODEC_LISTENER_H

#include "ipc_types.h"
#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"
#include "avcodec_common.h"

namespace OHOS {
namespace Media {
class IStandardAVCodecListener : public IRemoteBroker {
public:
    virtual ~IStandardAVCodecListener() = default;
    virtual void OnError(AVCodecErrorType errorType, int32_t errorCode) = 0;
    virtual void OnOutputFormatChanged(const Format &format) = 0;
    virtual void OnInputBufferAvailable(uint32_t index) = 0;
    virtual void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) = 0;

    /**
     * IPC code ID
     */
    enum AVCodecListenerMsg {
        ON_ERROR = 0,
        ON_OUTPUT_FORMAT_CHANGED,
        ON_INPUT_BUFFER_AVAILABLE,
        ON_OUTPUT_BUFFER_AVAILABLE
    };

    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardAVCodecListener");
};
} // namespace Media
} // namespace OHOS
#endif // I_STANDARD_AVCODEC_LISTENER_H
