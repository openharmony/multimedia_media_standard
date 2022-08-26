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

#ifndef I_STANDARD_AVCODEC_SERVICE_H
#define I_STANDARD_AVCODEC_SERVICE_H

#include "ipc_types.h"
#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"
#include "avcodec_common.h"
#include "avcodec_info.h"
#include "avsharedmemory.h"
#include "surface.h"

namespace OHOS {
namespace Media {
class IStandardAVCodecService : public IRemoteBroker {
public:
    virtual ~IStandardAVCodecService() = default;

    virtual int32_t SetListenerObject(const sptr<IRemoteObject> &object) = 0;
    virtual int32_t InitParameter(AVCodecType type, bool isMimeType, const std::string &name) = 0;
    virtual int32_t Configure(const Format &format) = 0;
    virtual int32_t Prepare() = 0;
    virtual int32_t Start() = 0;
    virtual int32_t Stop() = 0;
    virtual int32_t Flush() = 0;
    virtual int32_t Reset() = 0;
    virtual int32_t Release() = 0;
    virtual int32_t NotifyEos() = 0;
    virtual sptr<OHOS::Surface> CreateInputSurface() = 0;
    virtual int32_t SetOutputSurface(sptr<OHOS::Surface> surface) = 0;
    virtual std::shared_ptr<AVSharedMemory> GetInputBuffer(uint32_t index) = 0;
    virtual int32_t QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) = 0;
    virtual std::shared_ptr<AVSharedMemory> GetOutputBuffer(uint32_t index) = 0;
    virtual int32_t GetOutputFormat(Format &format) = 0;
    virtual int32_t ReleaseOutputBuffer(uint32_t index, bool render = false) = 0;
    virtual int32_t SetParameter(const Format &format) = 0;
    virtual int32_t DestroyStub() = 0;

    /**
     * IPC code ID
     */
    enum AVCodecServiceMsg {
        SET_LISTENER_OBJ = 0,
        INIT_PARAMETER,
        CONFIGURE,
        PREPARE,
        START,
        STOP,
        FLUSH,
        RESET,
        RELEASE,
        NOTIFY_EOS,
        CREATE_INPUT_SURFACE,
        SET_OUTPUT_SURFACE,
        GET_INPUT_BUFFER,
        QUEUE_INPUT_BUFFER,
        GET_OUTPUT_BUFFER,
        GET_OUTPUT_FORMAT,
        RELEASE_OUTPUT_BUFFER,
        SET_PARAMETER,
        DESTROY
    };

    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardAVCodecService");
};
} // namespace Media
} // namespace OHOS
#endif // I_STANDARD_AVCODEC_SERVICE_H
