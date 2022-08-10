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

#ifndef AVCODEC_ENGINE_CTRL_H
#define AVCODEC_ENGINE_CTRL_H

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include "avcodec_engine_factory.h"
#include "i_avcodec_engine.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class AVCodecEngineCtrl : public NoCopyable {
public:
    AVCodecEngineCtrl();
    ~AVCodecEngineCtrl();

    int32_t Init(AVCodecType type, bool useSoftware, const std::string &name);
    int32_t Prepare(std::shared_ptr<ProcessorConfig> inputConfig, std::shared_ptr<ProcessorConfig> outputConfig);
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Release();
    int32_t NotifyEos();
    void SetObs(const std::weak_ptr<IAVCodecEngineObs> &obs);
    sptr<Surface> CreateInputSurface(std::shared_ptr<ProcessorConfig> inputConfig);
    int32_t SetOutputSurface(sptr<Surface> surface);
    std::shared_ptr<AVSharedMemory> GetInputBuffer(uint32_t index);
    int32_t QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag);
    std::shared_ptr<AVSharedMemory> GetOutputBuffer(uint32_t index);
    int32_t ReleaseOutputBuffer(uint32_t index, bool render);
    int32_t SetParameter(const Format &format);
    int32_t SetConfigParameter(const Format &format);

private:
    static GstBusSyncReply BusSyncHandler(GstBus *bus, GstMessage *message, gpointer userData);

    int32_t InnerFlush() const;
    AVCodecType codecType_ = AVCODEC_TYPE_VIDEO_ENCODER;
    GstPipeline *gstPipeline_ = nullptr;
    GstBus *bus_ = nullptr;
    GstElement *codecBin_ = nullptr;
    bool useSurfaceInput_ = false;
    bool useSurfaceRender_ = false;
    std::condition_variable gstPipeCond_;
    std::mutex gstPipeMutex_;
    std::weak_ptr<IAVCodecEngineObs> obs_;
    std::unique_ptr<SrcBase> src_;
    std::unique_ptr<SinkBase> sink_;
    bool isEncoder_ = false;
    bool flushAtStart_ = false;
    bool isStart_ = false;
    bool isUseSoftWare_ = false;
};
} // namespace Media
} // namespace OHOS
#endif // AVCODEC_ENGINE_CTRL_H
