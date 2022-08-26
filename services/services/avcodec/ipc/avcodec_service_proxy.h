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

#ifndef AVCODEC_SERVICE_PROXY_H
#define AVCODEC_SERVICE_PROXY_H

#include "i_standard_avcodec_service.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class AVCodecServiceProxy : public IRemoteProxy<IStandardAVCodecService>, public NoCopyable {
public:
    explicit AVCodecServiceProxy(const sptr<IRemoteObject> &impl);
    virtual ~AVCodecServiceProxy();

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

private:
    static inline BrokerDelegator<AVCodecServiceProxy> delegator_;

    class AVCodecBufferCache;
    std::unique_ptr<AVCodecBufferCache> inputBufferCache_;
    std::unique_ptr<AVCodecBufferCache> outputBufferCache_;
};
} // namespace Media
} // namespace OHOS
#endif // AVCODEC_SERVICE_PROXY_H
