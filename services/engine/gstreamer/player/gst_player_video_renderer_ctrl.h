
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

#ifndef GST_PLAYER_VIDEO_RENDERER_CTRL_H
#define GST_PLAYER_VIDEO_RENDERER_CTRL_H

#include <memory>
#include <string>
#include <gst/gst.h>
#include <gst/player/player.h>
#include "i_player_engine.h"
#include "time_monitor.h"

namespace OHOS {
namespace Media {
class GstPlayerVideoRendererCtrl : public NoCopyable {
public:
    explicit GstPlayerVideoRendererCtrl(const sptr<Surface> &surface);
    ~GstPlayerVideoRendererCtrl();

    int32_t InitVideoSink(const GstElement *playbin);
    int32_t InitAudioSink(const GstElement *playbin);
    const GstElement *GetVideoSink() const;
    const sptr<Surface> GetProducerSurface() const;
    int32_t PullVideoBuffer();
    int32_t PrerollVideoBuffer();
    sptr<SurfaceBuffer> RequestBuffer(const GstVideoMeta *videoMeta);
    int32_t UpdateSurfaceBuffer(const GstBuffer &buffer);
    int32_t SetCallbacks(const std::weak_ptr<IPlayerEngineObs> &obs);

private:
    BufferRequestConfig UpdateRequestConfig(const GstVideoMeta *videoMeta) const;
    void SetSurfaceTimeFromSysPara();
    void SetDumpFrameFromSysPara();
    void SetKpiLogFromSysPara();
    void SetDumpFrameInternalFromSysPara();
    void SaveFrameToFile(const unsigned char *buffer, size_t size);
    void CopyToSurfaceBuffer(sptr<SurfaceBuffer> surfaceBuffer, const GstBuffer &buffer, bool &needFlush);
    int32_t CopyDefault(sptr<SurfaceBuffer> surfaceBuffer, const GstBuffer &buffer);
    int32_t CopyRgba(sptr<SurfaceBuffer> surfaceBuffer, const GstBuffer &buffer,
        const GstMapInfo &map, int32_t stride);
    void KpiFpsLog();
    sptr<Surface> producerSurface_ = nullptr;
    GstElement *videoSink_ = nullptr;
    GstElement *audioSink_ = nullptr;
    GstCaps *videoCaps_ = nullptr;
    GstCaps *audioCaps_ = nullptr;
    bool surfaceTimeEnable_ = false;
    bool dumpFrameEnable_ = false;
    bool kpiLogEnable_ = false;
    bool firstRenderFrame_ = true;
    uint32_t dumpFrameNum_ = 0;
    uint32_t dumpFrameInternal_ = 1;
    uint32_t queueSize_ = 0;
    TimeMonitor surfaceTimeMonitor_;
    std::vector<gulong> signalIds_;
    std::weak_ptr<IPlayerEngineObs> obs_;
    uint64_t flushBufferNums_ = 0;
    uint64_t lastFlushBufferNums_ = 0;
    uint64_t lastFlushBufferTime_ = 0;
};

class GstPlayerVideoRendererFactory {
public:
    GstPlayerVideoRendererFactory() = delete;
    ~GstPlayerVideoRendererFactory() = delete;
    static GstPlayerVideoRenderer *Create(const std::shared_ptr<GstPlayerVideoRendererCtrl> &rendererCtrl);
    static void Destroy(GstPlayerVideoRenderer *renderer);
};
} // Media
} // OHOS
#endif // GST_PLAYER_VIDEO_RENDERER_CTRL_H
