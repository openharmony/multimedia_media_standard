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

#ifndef AVCODEC_SERVICE_CLIENT_H
#define AVCODEC_SERVICE_CLIENT_H

#include "i_avcodec_service.h"
#include "i_standard_avcodec_service.h"
#include "avcodec_listener_stub.h"

namespace OHOS {
namespace Media {
class AVCodecClient : public IAVCodecService {
public:
    static std::shared_ptr<AVCodecClient> Create(const sptr<IStandardAVCodecService> &ipcProxy);
    explicit AVCodecClient(const sptr<IStandardAVCodecService> &ipcProxy);
    ~AVCodecClient();
    // IAVCodecService override
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
    int32_t SetOutputSurface(sptr<Surface> surface) override;
    std::shared_ptr<AVSharedMemory> GetInputBuffer(uint32_t index) override;
    int32_t QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) override;
    std::shared_ptr<AVSharedMemory> GetOutputBuffer(uint32_t index) override;
    int32_t GetOutputFormat(Format &format) override;
    int32_t ReleaseOutputBuffer(uint32_t index, bool render) override;
    int32_t SetParameter(const Format &format) override;
    int32_t SetCallback(const std::shared_ptr<AVCodecCallback> &callback) override;
    // AVCodecClient
    void MediaServerDied();

private:
    int32_t CreateListenerObject();

    sptr<IStandardAVCodecService> codecProxy_ = nullptr;
    sptr<AVCodecListenerStub> listenerStub_ = nullptr;
    std::shared_ptr<AVCodecCallback> callback_ = nullptr;
    std::mutex mutex_;
};
} // namespace Media
} // namespace OHOS
#endif // AVCODEC_SERVICE_CLIENT_H
