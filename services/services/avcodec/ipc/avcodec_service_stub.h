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

#ifndef AVCODEC_SERVICE_STUB_H
#define AVCODEC_SERVICE_STUB_H

#include <map>
#include "i_standard_avcodec_listener.h"
#include "i_standard_avcodec_service.h"
#include "avcodec_server.h"
#include "media_death_recipient.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class AVCodecServiceStub : public IRemoteStub<IStandardAVCodecService>, public NoCopyable {
public:
    static sptr<AVCodecServiceStub> Create();
    virtual ~AVCodecServiceStub();

    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
    using AVCodecStubFunc = int32_t(AVCodecServiceStub::*)(MessageParcel &data, MessageParcel &reply);
    int32_t SetListenerObject(const sptr<IRemoteObject> &object) override;
    int32_t InitParameter(AVCodecType type, bool isMimeType, const std::string &name) override;
    int32_t Configure(const Format &format) override;
    int32_t Prepare() override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t Flush() override;
    int32_t Reset() override;
    int32_t Release() override;
    int32_t NotifyEos() override;
    sptr<OHOS::Surface> CreateInputSurface() override;
    int32_t SetOutputSurface(sptr<OHOS::Surface> surface) override;
    std::shared_ptr<AVSharedMemory> GetInputBuffer(uint32_t index) override;
    int32_t QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) override;
    std::shared_ptr<AVSharedMemory> GetOutputBuffer(uint32_t index) override;
    int32_t GetOutputFormat(Format &format) override;
    int32_t ReleaseOutputBuffer(uint32_t index, bool render) override;
    int32_t SetParameter(const Format &format) override;
    int32_t DestroyStub() override;
    int32_t DumpInfo(int32_t fd);

private:
    AVCodecServiceStub();
    int32_t Init();
    int32_t SetListenerObject(MessageParcel &data, MessageParcel &reply);
    int32_t InitParameter(MessageParcel &data, MessageParcel &reply);
    int32_t Configure(MessageParcel &data, MessageParcel &reply);
    int32_t Prepare(MessageParcel &data, MessageParcel &reply);
    int32_t Start(MessageParcel &data, MessageParcel &reply);
    int32_t Stop(MessageParcel &data, MessageParcel &reply);
    int32_t Flush(MessageParcel &data, MessageParcel &reply);
    int32_t Reset(MessageParcel &data, MessageParcel &reply);
    int32_t Release(MessageParcel &data, MessageParcel &reply);
    int32_t NotifyEos(MessageParcel &data, MessageParcel &reply);
    int32_t CreateInputSurface(MessageParcel &data, MessageParcel &reply);
    int32_t SetOutputSurface(MessageParcel &data, MessageParcel &reply);
    int32_t GetInputBuffer(MessageParcel &data, MessageParcel &reply);
    int32_t QueueInputBuffer(MessageParcel &data, MessageParcel &reply);
    int32_t GetOutputBuffer(MessageParcel &data, MessageParcel &reply);
    int32_t GetOutputFormat(MessageParcel &data, MessageParcel &reply);
    int32_t ReleaseOutputBuffer(MessageParcel &data, MessageParcel &reply);
    int32_t SetParameter(MessageParcel &data, MessageParcel &reply);
    int32_t DestroyStub(MessageParcel &data, MessageParcel &reply);

    std::shared_ptr<IAVCodecService> codecServer_ = nullptr;
    std::map<uint32_t, AVCodecStubFunc> recFuncs_;
    std::mutex mutex_;

    class AVCodecBufferCache;
    std::unique_ptr<AVCodecBufferCache> inputBufferCache_;
    std::unique_ptr<AVCodecBufferCache> outputBufferCache_;
};
} // namespace Media
} // namespace OHOS
#endif // AVCODEC_SERVICE_STUB_H
