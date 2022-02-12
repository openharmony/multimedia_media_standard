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

#ifndef SINK_BYTEBUFFER_IMPL_H
#define SINK_BYTEBUFFER_IMPL_H

#include "sink_base.h"
#include <mutex>
#include <vector>
#include "avsharedmemory.h"
#include "gst_shared_mem_sink.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class SinkBytebufferImpl : public SinkBase {
public:
    SinkBytebufferImpl();
    ~SinkBytebufferImpl() override;
    DISALLOW_COPY_AND_MOVE(SinkBytebufferImpl);

    int32_t Init() override;
    int32_t Configure(std::shared_ptr<ProcessorConfig> config) override;
    int32_t Flush() override;
    std::shared_ptr<AVSharedMemory> GetOutputBuffer(uint32_t index) override;
    int32_t ReleaseOutputBuffer(uint32_t index, bool render = false) override;
    int32_t SetCallback(const std::weak_ptr<IAVCodecEngineObs> &obs) override;
    bool IsEos() override;

private:
    static void EosCb(GstMemSink *memSink, gpointer userData);
    static GstFlowReturn NewSampleCb(GstMemSink *memSink, GstBuffer *sample, gpointer userData);

    int32_t HandleNewSampleCb(GstBuffer *buffer);
    int32_t FindBufferIndex(uint32_t &index, std::shared_ptr<AVSharedMemory> mem);

    std::mutex mutex_;
    std::vector<std::shared_ptr<BufferWrapper>> bufferList_;
    std::weak_ptr<IAVCodecEngineObs> obs_;
    bool isFirstFrame_ = true;
    Format bufferFormat_;
    bool isEos_ = false;
};
} // Media
} // OHOS
#endif // SINK_BYTEBUFFER_IMPL_H