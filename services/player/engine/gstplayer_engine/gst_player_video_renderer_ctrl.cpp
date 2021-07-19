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
#include "display_type.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "GstPlayerVideoRendererCtrl"};
}

namespace OHOS {
namespace Media {
class GstPlayerVideoRendererCap {
public:
    GstPlayerVideoRendererCap() = delete;
    ~GstPlayerVideoRendererCap() = delete;
    static GstElement *CreateSink(GstPlayerVideoRenderer *renderer, GstPlayer *player);
    using DataAvailableFunc = GstFlowReturn (*)(GstAppSink *appsink, gpointer userData);
    static GstElement *CreateAudioSink(GstCaps *caps, DataAvailableFunc callback, gpointer userData);
    static GstElement *CreateVideoSink(GstCaps *caps, DataAvailableFunc callback, gpointer userData);
    static GstFlowReturn VideoDataAvailableCb(GstAppSink *appsink, gpointer userData);
    static GstPadProbeReturn SinkPadProbeCb(GstPad *pad, GstPadProbeInfo *info, gpointer userData);
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

static GstPlayerVideoRenderer *player_video_renderer_new(std::shared_ptr<GstPlayerVideoRendererCtrl> rendererCtrl)
{
    MEDIA_LOGI("enter");
    PLAYER_VIDEO_RENDERER(nullptr);
    PLAYER_IS_VIDEO_RENDERER(nullptr);
    PlayerVideoRenderer *self = (PlayerVideoRenderer *)g_object_new(PLAYER_TYPE_VIDEO_RENDERER, nullptr);
    CHECK_AND_RETURN_RET_LOG(self != nullptr, nullptr, "g_object_new failed..");

    self->rendererCtrl = rendererCtrl.get();
    return reinterpret_cast<GstPlayerVideoRenderer *>(self);
}

GstElement *GstPlayerVideoRendererCap::CreateSink(GstPlayerVideoRenderer *renderer, GstPlayer *player)
{
    MEDIA_LOGI("CreateVideoSink in.");
    GstPlayerVideoRendererCtrl *userData = (reinterpret_cast<PlayerVideoRenderer *>(renderer))->rendererCtrl;
    CHECK_AND_RETURN_RET_LOG(userData != nullptr, nullptr, "userData is nullptr..");

    GstElement *playbin = gst_player_get_pipeline(player);
    CHECK_AND_RETURN_RET_LOG(playbin != nullptr, nullptr, "playbin is nullptr..");

    (void)userData->InitAudioSink(playbin);
    (void)userData->InitVideoSink(playbin);

    gst_object_unref(playbin);
    return const_cast<GstElement *>(userData->GetVideoSink());
}

GstElement *GstPlayerVideoRendererCap::CreateAudioSink(GstCaps *caps, DataAvailableFunc callback, gpointer userData)
{
    (void)callback;
    MEDIA_LOGI("CreateVideoSink in.");
    CHECK_AND_RETURN_RET_LOG(caps != nullptr, nullptr, "input caps is nullptr..");
    CHECK_AND_RETURN_RET_LOG(userData != nullptr, nullptr, "input userData is nullptr..");

    auto sink = gst_element_factory_make("audioserversink", nullptr);
    CHECK_AND_RETURN_RET_LOG(sink != nullptr, nullptr, "gst_element_factory_make failed..");

    GstPad *pad = gst_element_get_static_pad(sink, "sink");
    CHECK_AND_RETURN_RET_LOG(pad != nullptr, nullptr, "gst_element_get_static_pad failed..");

    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM, SinkPadProbeCb, userData, nullptr);
    return sink;
}

GstElement *GstPlayerVideoRendererCap::CreateVideoSink(GstCaps *caps, DataAvailableFunc callback, gpointer userData)
{
    MEDIA_LOGI("CreateVideoSink in.");
    CHECK_AND_RETURN_RET_LOG(caps != nullptr, nullptr, "input caps is nullptr..");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, nullptr, "input callback is nullptr..");
    CHECK_AND_RETURN_RET_LOG(userData != nullptr, nullptr, "input userData is nullptr..");

    auto sink = gst_element_factory_make("appsink", nullptr);
    CHECK_AND_RETURN_RET_LOG(sink != nullptr, nullptr, "gst_element_factory_make failed..");

    GstAppSinkCallbacks sinkCallbacks = {nullptr, nullptr, callback};
    gst_app_sink_set_callbacks(GST_APP_SINK(sink), &sinkCallbacks, userData, nullptr);
    gst_app_sink_set_caps(GST_APP_SINK(sink), caps);

    GstPad *pad = gst_element_get_static_pad(sink, "sink");
    CHECK_AND_RETURN_RET_LOG(pad != nullptr, nullptr, "gst_element_get_static_pad failed..");

    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM, SinkPadProbeCb, userData, nullptr);
    return sink;
}

GstFlowReturn GstPlayerVideoRendererCap::VideoDataAvailableCb(GstAppSink *appsink, gpointer userData)
{
    CHECK_AND_RETURN_RET_LOG(appsink != nullptr, GST_FLOW_ERROR, "appsink is nullptr..");
    CHECK_AND_RETURN_RET_LOG(userData != nullptr, GST_FLOW_ERROR, "userData is nullptr..");

    auto ctrl = reinterpret_cast<GstPlayerVideoRendererCtrl *>(userData);
    int32_t ret = ctrl->PullVideoBuffer();
    if (ret != ERR_OK) {
        MEDIA_LOGE("Failed to PullVideoBuffer!");
    }
    return GST_FLOW_OK;
}

GstPadProbeReturn GstPlayerVideoRendererCap::SinkPadProbeCb(GstPad *pad, GstPadProbeInfo *info, gpointer userData)
{
    (void)userData;
    GstQuery *query = GST_PAD_PROBE_INFO_QUERY(info);
    if (GST_QUERY_TYPE(query) == GST_QUERY_ALLOCATION) {
        GstCaps *caps = nullptr;
        gboolean needPool;
        gst_query_parse_allocation(query, &caps, &needPool);

        auto s = gst_caps_get_structure(caps, 0);
        auto mediaType = gst_structure_get_name(s);
        bool isVideo = g_str_has_prefix(mediaType, "video/");
        if (isVideo) {
            gst_query_add_allocation_meta(query, GST_VIDEO_META_API_TYPE, nullptr);
        }
    }
    return GST_PAD_PROBE_OK;
}

GstPlayerVideoRendererCtrl::GstPlayerVideoRendererCtrl(sptr<Surface> surface)
    : producerSurface_(surface)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

GstPlayerVideoRendererCtrl::~GstPlayerVideoRendererCtrl()
{
    if (videoSink_ != nullptr) {
        gst_object_unref(videoSink_);
    }

    if (audioSink_ != nullptr) {
        gst_object_unref(audioSink_);
    }

    if (audioCaps_ != nullptr) {
        g_clear_pointer(&audioCaps_, gst_caps_unref);
    }

    if (audioCaps_ != nullptr) {
        g_clear_pointer(&videoCaps_, gst_caps_unref);
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
        std::string formatName = GetVideoSinkFormat();
        videoCaps_ = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, formatName.c_str(), nullptr);
        CHECK_AND_RETURN_RET_LOG(videoCaps_ != nullptr, ERR_INVALID_OPERATION, "gst_caps_new_simple failed..");

        videoSink_ = GstPlayerVideoRendererCap::CreateVideoSink(videoCaps_,
            GstPlayerVideoRendererCap::VideoDataAvailableCb, reinterpret_cast<gpointer>(this));
        CHECK_AND_RETURN_RET_LOG(videoSink_ != nullptr, ERR_INVALID_OPERATION, "CreateVideoSink failed..");

        g_object_set(const_cast<GstElement *>(playbin), "video-sink", videoSink_, nullptr);
    }
    return ERR_OK;
}

std::string GstPlayerVideoRendererCtrl::GetVideoSinkFormat() const
{
    std::string formatName = "NV21";
    if (producerSurface_ != nullptr) {
        const std::string SURFACE_FORMAT = "SURFACE_FORMAT";
        std::string format = producerSurface_->GetUserData(SURFACE_FORMAT);
        MEDIA_LOGD("surface_format is %{public}s!", format.c_str());
        if (format == std::to_string(PIXEL_FMT_RGBA_8888)) {
            formatName = "RGBA";
        } else {
            formatName = "NV21";
        }
    }
    MEDIA_LOGI("gst_caps_new_simple format is %{public}s!", formatName.c_str());
    return formatName;
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
        CHECK_AND_RETURN_RET_LOG(audioCaps_ != nullptr, ERR_INVALID_OPERATION, "gst_caps_new_simple failed..");

        audioSink_ = GstPlayerVideoRendererCap::CreateAudioSink(audioCaps_, nullptr, reinterpret_cast<gpointer>(this));
        CHECK_AND_RETURN_RET_LOG(audioSink_ != nullptr, ERR_INVALID_OPERATION, "CreateAudioSink failed..");

        g_object_set(const_cast<GstElement *>(playbin), "audio-sink", audioSink_, nullptr);
    }
    return ERR_OK;
}

int32_t GstPlayerVideoRendererCtrl::PullVideoBuffer()
{
    CHECK_AND_RETURN_RET_LOG(videoSink_ != nullptr, ERR_INVALID_OPERATION, "videoSink_ is nullptr..");

    GstAppSink *appsink = GST_APP_SINK(videoSink_);
    CHECK_AND_RETURN_RET_LOG(appsink != nullptr, ERR_INVALID_OPERATION, "appsink is nullptr..");

    GstSample *sample = gst_app_sink_pull_sample(appsink);
    CHECK_AND_RETURN_RET_LOG(sample != nullptr, ERR_INVALID_OPERATION, "sample is nullptr..");

    auto buf = gst_sample_get_buffer(sample);
    if (!buf) {
        MEDIA_LOGE("gst_sample_get_buffer err");
        gst_sample_unref(sample);
        return ERR_INVALID_OPERATION;
    }

    int32_t ret = UpdateSurfaceBuffer(*buf);
    if (ret != ERR_OK) {
        MEDIA_LOGE("Failed to update surface buffer and please provide the sptr<Surface>!");
    }

    gst_sample_unref(sample);
    return ret;
}

int32_t GstPlayerVideoRendererCtrl::UpdateSurfaceBuffer(const GstBuffer &buffer)
{
    CHECK_AND_RETURN_RET_LOG(producerSurface_ != nullptr, ERR_INVALID_OPERATION,
        "Surface is nullptr.Video cannot be played.");

    auto buf = const_cast<GstBuffer *>(&buffer);
    CHECK_AND_RETURN_RET_LOG(buf != nullptr, ERR_INVALID_VALUE, "input buf is nullptr..");

    GstVideoMeta *videoMeta = gst_buffer_get_video_meta(buf);
    CHECK_AND_RETURN_RET_LOG(videoMeta != nullptr, ERR_INVALID_VALUE, "gst_buffer_get_video_meta failed..");

    gsize size = gst_buffer_get_size(buf);
    CHECK_AND_RETURN_RET_LOG(size > 0, ERR_INVALID_VALUE, "gst_buffer_get_size failed(%{public}d)..", size);

    BufferRequestConfig requestConfig;
    requestConfig.width = videoMeta->width;
    requestConfig.height = videoMeta->height;
    constexpr int32_t strideAlignment = 8;
    const std::string SURFACE_FORMAT = "SURFACE_FORMAT";
    requestConfig.strideAlignment = strideAlignment;
    std::string format = producerSurface_->GetUserData(SURFACE_FORMAT);
    if (format == std::to_string(PIXEL_FMT_RGBA_8888)) {
        requestConfig.format = PIXEL_FMT_RGBA_8888;
    } else {
        requestConfig.format = PIXEL_FMT_YCRCB_420_SP;
    }
    requestConfig.usage = HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA;
    requestConfig.timeout = 0;

    int32_t releaseFence;
    sptr<SurfaceBuffer> surfaceBuffer;

    SurfaceError ret = producerSurface_->RequestBuffer(surfaceBuffer, releaseFence, requestConfig);
    CHECK_AND_RETURN_RET_LOG(ret == SURFACE_ERROR_OK, ERR_INVALID_OPERATION,
        "RequestBuffer failed(ret = %{public}d)..", ret);

    CHECK_AND_RETURN_RET_LOG(surfaceBuffer != nullptr, ERR_INVALID_OPERATION, "surfaceBuffer is nullptr..");
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer->GetVirAddr() != 0, ERR_INVALID_OPERATION, "Buffer addr is nullptr..");
    gst_buffer_extract(buf, 0, surfaceBuffer->GetVirAddr(), size);

    BufferFlushConfig flushConfig = {};
    flushConfig.damage.x = 0;
    flushConfig.damage.y = 0;
    flushConfig.damage.w = videoMeta->width;
    flushConfig.damage.h =  videoMeta->height;
    producerSurface_->FlushBuffer(surfaceBuffer, -1, flushConfig);

    return ERR_OK;
}

GstPlayerVideoRenderer *GstPlayerVideoRendererFactory::Create(std::shared_ptr<GstPlayerVideoRendererCtrl> rendererCtrl)
{
    CHECK_AND_RETURN_RET_LOG(rendererCtrl != nullptr, nullptr, "rendererCtrl is nullptr..");
    return player_video_renderer_new(rendererCtrl);
}

void GstPlayerVideoRendererFactory::Destroy(GstPlayerVideoRenderer *renderer)
{
    CHECK_AND_RETURN_LOG(renderer != nullptr, "renderer is nullptr");
    gst_object_unref(renderer);
}
} // Media
} // OHOS