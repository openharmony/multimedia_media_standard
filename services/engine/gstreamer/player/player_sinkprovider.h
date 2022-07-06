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

#ifndef PLAYER_SINKPROVIDER_H
#define PLAYER_SINKPROVIDER_H

#include <cstdint>
#include <memory>
#include <gst/gst.h>
#include "gst_mem_sink.h"
#include "playbin_sink_provider.h"
#include "i_player_engine.h"

namespace OHOS {
namespace Media {
class PlayerSinkProvider : public PlayBinSinkProvider, public NoCopyable {
public:
    explicit PlayerSinkProvider(const sptr<Surface> &surface);
    ~PlayerSinkProvider();

    SinkPtr CreateAudioSink() override;
    SinkPtr CreateVideoSink() override;
    SinkPtr GetVideoSink() override;

    void SetAppInfo(int32_t uid, int32_t pid) override;
    void SetVideoScaleType(const uint32_t videoScaleType) override;
    void SetMsgNotifier(PlayBinMsgNotifier notifier) override;

    PlayBinMsgNotifier notifier_;

private:
    const sptr<Surface> GetProducerSurface() const;
    GstElement *DoCreateAudioSink(const GstCaps *caps, const gpointer userData);
    GstElement *DoCreateVideoSink(const GstCaps *caps, const gpointer userData);
    bool EnableKpiAVSyncLog() const;
    void SetFirstRenderFrameFlag(bool firstRenderFrame);
    bool GetFirstRenderFrameFlag() const;

    static GstPadProbeReturn SinkPadProbeCb(GstPad *pad, GstPadProbeInfo *info, gpointer userData);
    static void EosCb(GstMemSink *memSink, gpointer userData);
    static GstFlowReturn NewPrerollCb(GstMemSink *memSink, GstBuffer *sample, gpointer userData);
    static GstFlowReturn NewSampleCb(GstMemSink *memSink, GstBuffer *sample, gpointer userData);
    static void FirstRenderFrame(gpointer userData);

    GstElement *audioSink_ = nullptr;
    GstElement *videoSink_ = nullptr;
    GstCaps *audioCaps_ = nullptr;
    GstCaps *videoCaps_ = nullptr;
    sptr<Surface> producerSurface_ = nullptr;
    uint32_t queueSize_ = 0;
    int32_t uid_ = 0;
    int32_t pid_ = 0;
    uint32_t videoScaleType_ = 0;
    bool firstRenderFrame_ = true;
};
}
}

#endif // PLAYER_SINKPROVIDER_H
