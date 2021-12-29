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
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class SinkSurfaceImpl : public SinkBase {
public:
    SinkSurfaceImpl();
    ~SinkSurfaceImpl() override;
    DISALLOW_COPY_AND_MOVE(SinkSurfaceImpl);

    int32_t Init() override;
    int32_t Configure(std::shared_ptr<ProcessorConfig> config) override;
    int32_t Flush() override;
    int32_t SetOutputSurface(sptr<Surface> surface) override;
    int32_t SetParameter(const Format &format) override;
    int32_t ReleaseOutputBuffer(uint32_t index, bool render) override;
    int32_t SetCallback(const std::weak_ptr<IAVCodecEngineObs> &obs) override;

private:
    static GstFlowReturn OutputAvailableCb(GstElement *sink, gpointer userData);
    static void EosCb(GstElement *sink, gpointer userData);

    int32_t HandleOutputCb();
    int32_t UpdateSurfaceBuffer(const GstBuffer &buffer);
    sptr<SurfaceBuffer> RequestBuffer(GstVideoMeta *videoMeta);

    std::mutex mutex_;
    gulong signalSample_ = 0;
    gulong signalEOS_ = 0;
    uint32_t bufferCount_ = 0;
    sptr<Surface> producerSurface_ = nullptr;
    std::vector<std::shared_ptr<BufferWrapper>> bufferList_;
    std::weak_ptr<IAVCodecEngineObs> obs_;
    bool isFirstFrame_ = true;
    Format bufferFormat_;
};
} // Media
} // OHOS
#endif // SINK_SURFACE_IMPL_H