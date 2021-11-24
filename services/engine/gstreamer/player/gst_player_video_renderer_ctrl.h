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
class GstPlayerVideoRendererCtrl {
public:
    explicit GstPlayerVideoRendererCtrl(const sptr<Surface> &surface);
    ~GstPlayerVideoRendererCtrl();
    DISALLOW_COPY_AND_MOVE(GstPlayerVideoRendererCtrl);
    int32_t InitVideoSink(const GstElement *playbin);
    int32_t InitAudioSink(const GstElement *playbin);
    const GstElement *GetVideoSink() const;
    int32_t PullVideoBuffer();
    sptr<SurfaceBuffer> RequestBuffer(const GstVideoMeta *videoMeta) const;
    int32_t UpdateSurfaceBuffer(const GstBuffer &buffer);

private:
    BufferRequestConfig UpdateRequestConfig(const GstVideoMeta *videoMeta) const;
    std::string GetVideoSinkFormat() const;
    void SetSurfaceTimeFromSysPara();

    sptr<Surface> producerSurface_ = nullptr;
    GstElement *videoSink_ = nullptr;
    GstElement *audioSink_ = nullptr;
    GstCaps *videoCaps_ = nullptr;
    GstCaps *audioCaps_ = nullptr;
    bool surfaceTimeEnable_ = false;
    TimeMonitor surfaceTimeMonitor_;
    gulong signalId_ = 0;
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
