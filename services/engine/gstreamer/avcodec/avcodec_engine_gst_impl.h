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

#ifndef AVCODEC_ENGINE_GST_IMPL_H
#define AVCODEC_ENGINE_GST_IMPL_H

#include <mutex>
#include "i_avcodec_engine.h"
#include "avcodec_engine_ctrl.h"
#include "avcodec_engine_factory.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class AVCodecEngineGstImpl : public IAVCodecEngine, public NoCopyable {
public:
    AVCodecEngineGstImpl();
    ~AVCodecEngineGstImpl();

    int32_t Init(AVCodecType type, bool isMimeType, const std::string &name) override;
    int32_t Configure(const Format &format) override;
    int32_t Prepare() override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t Flush() override;
    int32_t Reset() override;
    int32_t NotifyEos() override;
    sptr<Surface> CreateInputSurface() override;
    int32_t SetOutputSurface(const sptr<Surface> &surface) override;
    std::shared_ptr<AVSharedMemory> GetInputBuffer(uint32_t index) override;
    int32_t QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) override;
    std::shared_ptr<AVSharedMemory> GetOutputBuffer(uint32_t index) override;
    int32_t GetOutputFormat(Format &format) override;
    int32_t ReleaseOutputBuffer(uint32_t index, bool render) override;
    int32_t SetParameter(const Format &format) override;
    int32_t SetObs(const std::weak_ptr<IAVCodecEngineObs> &obs) override;

private:
    std::string FindMimeTypeByName(AVCodecType type, const std::string &name);
    int32_t HandleMimeType(AVCodecType type, const std::string &name);
    int32_t HandlePluginName(AVCodecType type, const std::string &name);
    int32_t QueryIsSoftPlugin(const std::string &name, bool &isSoftware);
    void CheckSurfaceFormat(Format &format);

    AVCodecType type_ = AVCODEC_TYPE_VIDEO_ENCODER;
    bool useSoftWare_ = false;
    std::string pluginName_ = "";
    std::unique_ptr<AVCodecEngineCtrl> ctrl_ = nullptr;
    std::unique_ptr<ProcessorBase> processor_ = nullptr;
    std::mutex mutex_;
    std::weak_ptr<IAVCodecEngineObs> obs_;
    Format format_;
    CapabilityData capData_;
};
} // namespace Media
} // namespace OHOS
#endif // AVCODEC_ENGINE_GST_IMPL_H
