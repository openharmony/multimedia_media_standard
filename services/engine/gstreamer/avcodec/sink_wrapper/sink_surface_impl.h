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

#ifndef SINK_SURFACE_IMPL_H
#define SINK_SURFACE_IMPL_H

#include "sink_base.h"
#include <gst/player/player.h>
#include "gst_surface_mem_sink.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class SinkSurfaceImpl : public SinkBase, public NoCopyable {
public:
    SinkSurfaceImpl();
    ~SinkSurfaceImpl() override;

    int32_t Init() override;
    int32_t Configure(std::shared_ptr<ProcessorConfig> config) override;
    int32_t Flush() override;
    int32_t SetOutputSurface(sptr<Surface> surface) override;
    int32_t SetOutputBuffersCount(uint32_t maxBuffers) override;
    int32_t SetCacheBuffersCount(uint32_t cacheBuffers) override;
    int32_t SetParameter(const Format &format) override;
    int32_t ReleaseOutputBuffer(uint32_t index, bool render) override;
    int32_t SetCallback(const std::weak_ptr<IAVCodecEngineObs> &obs) override;

private:
    static void EosCb(GstMemSink *memSink, gpointer userData);
    static GstFlowReturn NewSampleCb(GstMemSink *memSink, GstBuffer *sample, gpointer userData);

    int32_t HandleNewSampleCb(GstBuffer *buffer);
    int32_t FindBufferIndex(uint32_t &index, sptr<SurfaceBuffer> buffer);

    std::mutex mutex_;
    std::vector<std::shared_ptr<BufferWrapper>> bufferList_;
    std::weak_ptr<IAVCodecEngineObs> obs_;
    bool isFirstFrame_ = true;
    Format bufferFormat_;
};
} // namespace Media
} // namespace OHOS
#endif // SINK_SURFACE_IMPL_H