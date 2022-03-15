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

#include "gst_player_video_renderer_ctrl.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <sync_fence.h>
#include "securec.h"
#include "string_ex.h"
#include "display_type.h"
#include "media_log.h"
#include "param_wrapper.h"
#include "media_errors.h"
#include "surface_buffer_impl.h"
#include "gst_mem_sink.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "GstPlayerVideoRendererCtrl"};
    constexpr uint32_t DEFAULT_BUFFER_NUM = 8;
}

namespace OHOS {
namespace Media {
class GstPlayerVideoRendererCap {
public:
    GstPlayerVideoRendererCap() = delete;
    ~GstPlayerVideoRendererCap() = delete;
    static GstElement *CreateSink(GstPlayerVideoRenderer *renderer, GstPlayer *player);
    using DataAvailableFunc = GstFlowReturn (*)(const GstElement *appsink, gpointer userData);
    static GstElement *CreateAudioSink(const GstCaps *caps, const DataAvailableFunc callback, const gpointer userData);
    static GstElement *CreateVideoSink(const GstCaps *caps, const gpointer userData);
    static GstPadProbeReturn SinkPadProbeCb(GstPad *pad, GstPadProbeInfo *info, gpointer userData);
    static void EosCb(GstMemSink *memSink, gpointer userData);
    static GstFlowReturn NewSampleCb(GstMemSink *memSink, GstBuffer *sample, gpointer userData);
    static GstFlowReturn NewPrerollCb(GstMemSink *memSink, GstBuffer *sample, gpointer userData);
};

struct _PlayerVideoRenderer {
    GObject parent;
    GstPlayerVideoRendererCtrl *rendererCtrl;
};

static void player_video_renderer_interface_init(GstPlayerVideoRendererInterface *iface)
{
    MEDIA_LOGI("Video renderer interface init");
    iface->create_video_sink = GstPlayerVideoRendererCap::CreateSink;
}

#define PLAYER_TYPE_VIDEO_RENDERER player_video_renderer_get_type()
    G_DECLARE_FINAL_TYPE(PlayerVideoRenderer, player_video_renderer, PLAYER, VIDEO_RENDERER, GObject)

G_DEFINE_TYPE_WITH_CODE(PlayerVideoRenderer, player_video_renderer, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(GST_TYPE_PLAYER_VIDEO_RENDERER, player_video_renderer_interface_init));

static void player_video_renderer_init(PlayerVideoRenderer *self)
{
    (void)self;
}

static void player_video_renderer_class_init(PlayerVideoRendererClass *klass)
{
    (void)klass;
}

static GstPlayerVideoRenderer *player_video_renderer_new(
    const std::shared_ptr<GstPlayerVideoRendererCtrl> &rendererCtrl)
{
    MEDIA_LOGI("enter");
    (void)PLAYER_VIDEO_RENDERER(nullptr);
    (void)PLAYER_IS_VIDEO_RENDERER(nullptr);
    PlayerVideoRenderer *self = (PlayerVideoRenderer *)g_object_new(PLAYER_TYPE_VIDEO_RENDERER, nullptr);
    CHECK_AND_RETURN_RET_LOG(self != nullptr, nullptr, "g_object_new failed..");

    self->rendererCtrl = rendererCtrl.get();
    return reinterpret_cast<GstPlayerVideoRenderer *>(self);
}

GstElement *GstPlayerVideoRendererCap::CreateSink(GstPlayerVideoRenderer *renderer, GstPlayer *player)
{
    MEDIA_LOGI("CreateSink in.");
    CHECK_AND_RETURN_RET_LOG(renderer != nullptr, nullptr, "renderer is nullptr..");
    GstPlayerVideoRendererCtrl *userData = (reinterpret_cast<PlayerVideoRenderer *>(renderer))->rendererCtrl;
    CHECK_AND_RETURN_RET_LOG(userData != nullptr, nullptr, "userData is nullptr..");

    GstElement *playbin = gst_player_get_pipeline(player);
    CHECK_AND_RETURN_RET_LOG(playbin != nullptr, nullptr, "playbin is nullptr..");

    (void)userData->InitAudioSink(playbin);
    (void)userData->InitVideoSink(playbin);

    gst_object_unref(playbin);
    return const_cast<GstElement *>(userData->GetVideoSink());
}

GstElement *GstPlayerVideoRendererCap::CreateAudioSink(const GstCaps *caps,
    const DataAvailableFunc callback, const gpointer userData)
{
    (void)callback;
    (void)caps;
    MEDIA_LOGI("CreateAudioSink in.");
    CHECK_AND_RETURN_RET_LOG(userData != nullptr, nullptr, "input userData is nullptr..");

    auto sink = gst_element_factory_make("audioserversink", nullptr);
    CHECK_AND_RETURN_RET_LOG(sink != nullptr, nullptr, "gst_element_factory_make failed..");

    GstPad *pad = gst_element_get_static_pad(sink, "sink");
    if (pad == nullptr) {
        gst_object_unref(sink);
        MEDIA_LOGE("gst_element_get_static_pad failed..");
        return nullptr;
    }

    (void)gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM, SinkPadProbeCb, userData, nullptr);
    return sink;
}

GstElement *GstPlayerVideoRendererCap::CreateVideoSink(const GstCaps *caps, const gpointer userData)
{
    MEDIA_LOGI("CreateVideoSink in.");
    CHECK_AND_RETURN_RET_LOG(caps != nullptr, nullptr, "input caps is nullptr..");
    CHECK_AND_RETURN_RET_LOG(userData != nullptr, nullptr, "input userData is nullptr..");
    GstPlayerVideoRendererCtrl *rendererCtrl = reinterpret_cast<GstPlayerVideoRendererCtrl *>(userData);

    auto sink = gst_element_factory_make("surfacememsink", "sink");
    CHECK_AND_RETURN_RET_LOG(sink != nullptr, nullptr, "gst_element_factory_make failed..");
    gst_base_sink_set_async_enabled(GST_BASE_SINK(sink), FALSE);

    g_object_set(G_OBJECT(sink), "caps", caps, nullptr);
    g_object_set(G_OBJECT(sink), "surface", static_cast<gpointer>(rendererCtrl->GetProducerSurface()), nullptr);

    GstMemSinkCallbacks sinkCallbacks = { EosCb, NewPrerollCb, NewSampleCb };
    gst_mem_sink_set_callback(GST_MEM_SINK(sink), &sinkCallbacks, userData, nullptr);
    return sink;
}

void GstPlayerVideoRendererCap::EosCb(GstMemSink *memSink, gpointer userData)
{
    (void)memSink;
    (void)userData;
    MEDIA_LOGI("EOS in");
}

GstFlowReturn GstPlayerVideoRendererCap::NewSampleCb(GstMemSink *memSink, GstBuffer *sample, gpointer userData)
{
    (void)userData;
    MEDIA_LOGI("NewSampleCb in");
    CHECK_AND_RETURN_RET(gst_mem_sink_app_render(memSink, sample) == GST_FLOW_OK, GST_FLOW_ERROR);
    return GST_FLOW_OK;
}

GstFlowReturn GstPlayerVideoRendererCap::NewPrerollCb(GstMemSink *memSink, GstBuffer *sample, gpointer userData)
{
    (void)userData;
    MEDIA_LOGI("NewPrerollCb in");
    CHECK_AND_RETURN_RET(gst_mem_sink_app_preroll_render(memSink, sample) == GST_FLOW_OK, GST_FLOW_ERROR);
    return GST_FLOW_OK;
}

GstPadProbeReturn GstPlayerVideoRendererCap::SinkPadProbeCb(GstPad *pad, GstPadProbeInfo *info, gpointer userData)
{
    (void)pad;
    (void)userData;
    GstQuery *query = GST_PAD_PROBE_INFO_QUERY(info);
    if (GST_QUERY_TYPE(query) == GST_QUERY_ALLOCATION) {
        GstCaps *caps = nullptr;
        gboolean needPool;
        gst_query_parse_allocation(query, &caps, &needPool);

        auto s = gst_caps_get_structure(caps, 0);
        auto mediaType = gst_structure_get_name(s);
        gboolean isVideo = g_str_has_prefix(mediaType, "video/");
        if (isVideo) {
            gst_query_add_allocation_meta(query, GST_VIDEO_META_API_TYPE, nullptr);
        }
    }
    return GST_PAD_PROBE_OK;
}

GstPlayerVideoRendererCtrl::GstPlayerVideoRendererCtrl(const sptr<Surface> &surface)
    : producerSurface_(surface)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

GstPlayerVideoRendererCtrl::~GstPlayerVideoRendererCtrl()
{
    for (auto signalId : signalIds_) {
        g_signal_handler_disconnect(G_OBJECT(videoSink_), signalId);
    }

    producerSurface_ = nullptr;
    if (videoSink_ != nullptr) {
        gst_object_unref(videoSink_);
        videoSink_ = nullptr;
    }
    if (audioSink_ != nullptr) {
        gst_object_unref(audioSink_);
        audioSink_ = nullptr;
    }
    if (audioCaps_ != nullptr) {
        gst_caps_unref(audioCaps_);
        audioCaps_ = nullptr;
    }
    if (videoCaps_ != nullptr) {
        gst_caps_unref(videoCaps_);
        videoCaps_ = nullptr;
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

const GstElement *GstPlayerVideoRendererCtrl::GetVideoSink() const
{
    return videoSink_;
}

int32_t GstPlayerVideoRendererCtrl::InitVideoSink(const GstElement *playbin)
{
    if (videoCaps_ == nullptr) {
        videoCaps_ = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGBA", nullptr);
        CHECK_AND_RETURN_RET_LOG(videoCaps_ != nullptr, MSERR_INVALID_OPERATION, "gst_caps_new_simple failed..");

        videoSink_ = GstPlayerVideoRendererCap::CreateVideoSink(videoCaps_, reinterpret_cast<gpointer>(this));
        CHECK_AND_RETURN_RET_LOG(videoSink_ != nullptr, MSERR_INVALID_OPERATION, "CreateVideoSink failed..");

        g_object_set(const_cast<GstElement *>(playbin), "video-sink", videoSink_, nullptr);
    }
    if (producerSurface_ != nullptr) {
        producerSurface_->SetQueueSize(DEFAULT_BUFFER_NUM);
        queueSize_ = DEFAULT_BUFFER_NUM;
    }
    return MSERR_OK;
}

const sptr<Surface> GstPlayerVideoRendererCtrl::GetProducerSurface() const
{
    return producerSurface_;
}

int32_t GstPlayerVideoRendererCtrl::InitAudioSink(const GstElement *playbin)
{
    constexpr gint rate = 44100;
    constexpr gint channels = 2;
    if (audioCaps_ == nullptr) {
        audioCaps_ = gst_caps_new_simple("audio/x-raw",
                                         "format", G_TYPE_STRING, "S16LE",
                                         "rate", G_TYPE_INT, rate,
                                         "channels", G_TYPE_INT, channels, nullptr);
        CHECK_AND_RETURN_RET_LOG(audioCaps_ != nullptr, MSERR_INVALID_OPERATION, "gst_caps_new_simple failed..");

        audioSink_ = GstPlayerVideoRendererCap::CreateAudioSink(audioCaps_, nullptr, reinterpret_cast<gpointer>(this));
        CHECK_AND_RETURN_RET_LOG(audioSink_ != nullptr, MSERR_INVALID_OPERATION, "CreateAudioSink failed..");

        g_object_set(const_cast<GstElement *>(playbin), "audio-sink", audioSink_, nullptr);
    }
    return MSERR_OK;
}

int32_t GstPlayerVideoRendererCtrl::SetCallbacks(const std::weak_ptr<IPlayerEngineObs> &obs)
{
    obs_ = obs;
    return MSERR_OK;
}

GstPlayerVideoRenderer *GstPlayerVideoRendererFactory::Create(
    const std::shared_ptr<GstPlayerVideoRendererCtrl> &rendererCtrl)
{
    CHECK_AND_RETURN_RET_LOG(rendererCtrl != nullptr, nullptr, "rendererCtrl is nullptr..");
    return player_video_renderer_new(rendererCtrl);
}

void GstPlayerVideoRendererFactory::Destroy(GstPlayerVideoRenderer *renderer)
{
    CHECK_AND_RETURN_LOG(renderer != nullptr, "renderer is nullptr");
    gst_object_unref(renderer);
}
} // namespace Media
} // namespace OHOS
