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
    constexpr uint32_t MAX_DEFAULT_WIDTH = 10000;
    constexpr uint32_t MAX_DEFAULT_HEIGHT = 10000;
    constexpr uint32_t DEFAULT_BUFFER_NUM = 8;
    constexpr uint32_t MAX_BUFFER_NUM = 10;
    constexpr uint32_t DEFAULT_STRIDE = 4;
}

namespace OHOS {
namespace Media {
const std::string SURFACE_TIME_TAG = "UpdateSurfaceBuffer";
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

    GstMemSinkCallbacks sinkCallbacks = { EosCb, nullptr, NewSampleCb };
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
    : producerSurface_(surface),
      surfaceTimeMonitor_(SURFACE_TIME_TAG)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
    SetSurfaceTimeFromSysPara();
    SetDumpFrameFromSysPara();
    SetDumpFrameInternalFromSysPara();
    SetKpiLogFromSysPara();
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
        if (audioSink_) {
            g_object_set(GST_BASE_SINK_CAST(videoSink_), "audio-sink", audioSink_,
                "enable-kpi-log", static_cast<gboolean>(kpiLogEnable_), nullptr);
        }

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

int32_t GstPlayerVideoRendererCtrl::PullVideoBuffer()
{
    CHECK_AND_RETURN_RET_LOG(videoSink_ != nullptr, MSERR_INVALID_OPERATION, "videoSink_ is nullptr..");

    GstSample *sample = nullptr;
    g_signal_emit_by_name(G_OBJECT(videoSink_), "pull-sample", &sample);
    CHECK_AND_RETURN_RET_LOG(sample != nullptr, MSERR_INVALID_OPERATION, "sample is nullptr..");

    GstBuffer *buf = gst_sample_get_buffer(sample);
    if (buf == nullptr) {
        MEDIA_LOGE("gst_sample_get_buffer err");
        gst_sample_unref(sample);
        return MSERR_INVALID_OPERATION;
    }

    int32_t ret = UpdateSurfaceBuffer(*buf);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("Failed to update surface buffer and please provide the sptr<Surface>!");
        gst_sample_unref(sample);
        return ret;
    }

    flushBufferNums_++;
    KpiFpsLog();
    if (firstRenderFrame_) {
        std::shared_ptr<IPlayerEngineObs> tempObs = obs_.lock();
        if (tempObs != nullptr) {
            Format format;
            tempObs->OnInfo(INFO_TYPE_MESSAGE, PlayerMessageType::PLAYER_INFO_VIDEO_RENDERING_START, format);
            firstRenderFrame_ = false;
            MEDIA_LOGW("KPI-TRACE: FIRST-VIDEO-FRAME rendered");
        }
    }

    gst_sample_unref(sample);
    return ret;
}

int32_t GstPlayerVideoRendererCtrl::PrerollVideoBuffer()
{
    CHECK_AND_RETURN_RET_LOG(videoSink_ != nullptr, MSERR_INVALID_OPERATION, "videoSink_ is nullptr..");

    if (firstRenderFrame_) {
        MEDIA_LOGI("first frame, do not render it when preroll");
        return MSERR_OK;
    }

    GstSample *sample = nullptr;
    g_signal_emit_by_name(G_OBJECT(videoSink_), "pull-preroll", &sample);
    CHECK_AND_RETURN_RET_LOG(sample != nullptr, MSERR_INVALID_OPERATION, "sample is nullptr..");

    GstBuffer *buf = gst_sample_get_buffer(sample);
    if (buf == nullptr) {
        MEDIA_LOGE("gst_sample_get_buffer err");
        gst_sample_unref(sample);
        return MSERR_INVALID_OPERATION;
    }

    int32_t ret = UpdateSurfaceBuffer(*buf);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("Failed to update surface buffer and please provide the sptr<Surface>!");
    }

    gst_sample_unref(sample);
    return ret;
}

void GstPlayerVideoRendererCtrl::SetSurfaceTimeFromSysPara()
{
    std::string timeEnable;
    int32_t res = OHOS::system::GetStringParameter("sys.media.time.surface", timeEnable, "");
    if (res != 0 || timeEnable.empty()) {
        surfaceTimeEnable_ = false;
        MEDIA_LOGD("sys.media.time.surface=false");
        return;
    }
    MEDIA_LOGD("sys.media.time.surface=%{public}s", timeEnable.c_str());

    if (timeEnable == "true") {
        surfaceTimeEnable_ = true;
    }
}

void GstPlayerVideoRendererCtrl::SetDumpFrameFromSysPara()
{
    std::string dumpFrameEnable;
    int32_t res = OHOS::system::GetStringParameter("sys.media.dump.frame.enable", dumpFrameEnable, "");
    if (res != 0 || dumpFrameEnable.empty()) {
        dumpFrameEnable_ = false;
        MEDIA_LOGD("sys.media.dump.frame.enable=false");
        return;
    }
    MEDIA_LOGD("sys.media.dump.frame.enable=%{public}s", dumpFrameEnable.c_str());

    if (dumpFrameEnable == "true") {
        dumpFrameEnable_ = true;
    } else if (dumpFrameEnable == "false") {
        dumpFrameEnable_ = false;
    }
}

void GstPlayerVideoRendererCtrl::SetKpiLogFromSysPara()
{
    std::string kpiLogEnable;
    int32_t res = OHOS::system::GetStringParameter("sys.media.kpi.log.enable", kpiLogEnable, "");
    if (res != 0 || kpiLogEnable.empty()) {
        kpiLogEnable_ = false;
        MEDIA_LOGD("sys.media.kpi.log.enable=false");
        return;
    }
    MEDIA_LOGD("sys.media.kpi.log.enable=%{public}s", kpiLogEnable.c_str());

    if (kpiLogEnable == "true") {
        kpiLogEnable_ = true;
    } else if (kpiLogEnable == "false") {
        kpiLogEnable_ = false;
    }
}

void GstPlayerVideoRendererCtrl::SetDumpFrameInternalFromSysPara()
{
    std::string dumpFrameInternal;
    int32_t res = OHOS::system::GetStringParameter("sys.media.dump.frame.internal", dumpFrameInternal, "");
    if (res != 0 || dumpFrameInternal.empty()) {
        MEDIA_LOGD("sys.media.dump.frame.internal");
        return;
    }
    MEDIA_LOGD("sys.media.dump.frame.internal=%{public}s", dumpFrameInternal.c_str());

    int32_t internal = -1;
    if (!StrToInt(dumpFrameInternal, internal) || (internal < 0)) {
        MEDIA_LOGD("sys.media.dump.frame.internal");
        return;
    }

    dumpFrameInternal_ = static_cast<uint32_t>(internal);
}

BufferRequestConfig GstPlayerVideoRendererCtrl::UpdateRequestConfig(const GstVideoMeta *videoMeta) const
{
    BufferRequestConfig config;
    config.width = static_cast<int32_t>(videoMeta->width);
    config.height = static_cast<int32_t>(videoMeta->height);
    constexpr int32_t strideAlignment = 8;
    const std::string surfaceFormat = "SURFACE_FORMAT";
    config.strideAlignment = strideAlignment;
    std::string format = producerSurface_->GetUserData(surfaceFormat);
    if (format == std::to_string(PIXEL_FMT_RGBA_8888)) {
        config.format = PIXEL_FMT_RGBA_8888;
    } else {
        config.format = PIXEL_FMT_YCRCB_420_SP;
    }
    config.usage = HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA;
    config.timeout = 0;
    return config;
}

sptr<SurfaceBuffer> GstPlayerVideoRendererCtrl::RequestBuffer(const GstVideoMeta *videoMeta)
{
    CHECK_AND_RETURN_RET_LOG(videoMeta != nullptr, nullptr, "gst_buffer_get_video_meta failed..");
    CHECK_AND_RETURN_RET_LOG(videoMeta->width < MAX_DEFAULT_WIDTH && videoMeta->height < MAX_DEFAULT_HEIGHT,
        nullptr, "video size too large. Video cannot be played.");
    BufferRequestConfig requestConfig = UpdateRequestConfig(videoMeta);
    sptr<SurfaceBuffer> surfaceBuffer = nullptr;
    int32_t releaseFence = -1;
    SurfaceError ret = SURFACE_ERROR_OK;
    do {
        ret = producerSurface_->RequestBuffer(surfaceBuffer, releaseFence, requestConfig);
        if (ret != SURFACE_ERROR_OK) {
            if (queueSize_ < MAX_BUFFER_NUM) {
                ++queueSize_;
                producerSurface_->SetQueueSize(queueSize_);
            }
        }
    } while (0);
    sptr<SyncFence> autoFence = new(std::nothrow) SyncFence(releaseFence);
    if (autoFence != nullptr) {
        autoFence->Wait(100); // 100ms
    }
    CHECK_AND_RETURN_RET_LOG(ret == SURFACE_ERROR_OK, nullptr, "RequestBuffer is not ok..");
    return surfaceBuffer;
}

void GstPlayerVideoRendererCtrl::SaveFrameToFile(const unsigned char *buffer, size_t size)
{
    dumpFrameNum_++;
    if (!dumpFrameEnable_) {
        return;
    }

    if ((dumpFrameNum_ % dumpFrameInternal_) != 0) {
        return;
    }

    std::string fileName = "video_frame_" + std::to_string(dumpFrameNum_);
    std::ofstream outfile;
    std::string path("/data/media/");
    outfile.open(path + fileName);
    if (!outfile.is_open()) {
        MEDIA_LOGE("failed to open file %{public}s", fileName.c_str());
        return;
    }

    for (size_t i = 0; i < size; ++i) {
        outfile << buffer[i];
    }
    outfile.close();
}

int32_t GstPlayerVideoRendererCtrl::CopyDefault(sptr<SurfaceBuffer> surfaceBuffer, const GstBuffer &buffer)
{
    auto buf = const_cast<GstBuffer *>(&buffer);
    auto surfaceBufferAddr = surfaceBuffer->GetVirAddr();
    CHECK_AND_RETURN_RET_LOG(surfaceBufferAddr != nullptr, MSERR_NO_MEMORY, "Buffer addr is nullptr.");

    gsize size = gst_buffer_get_size(buf);
    CHECK_AND_RETURN_RET_LOG(size > 0, MSERR_NO_MEMORY, "gst_buffer_get_size failed..");
    gsize sizeCopy = gst_buffer_extract(buf, 0, surfaceBufferAddr, size);
    if (sizeCopy != size) {
        MEDIA_LOGW("extract buffer from size : %" G_GSIZE_FORMAT " to size %" G_GSIZE_FORMAT, size, sizeCopy);
    }

    unsigned char *bufferAddr = static_cast<unsigned char *>(static_cast<void *>(surfaceBufferAddr));
    SaveFrameToFile(bufferAddr, sizeCopy);
    return MSERR_OK;
}

int32_t GstPlayerVideoRendererCtrl::CopyRgba(sptr<SurfaceBuffer> surfaceBuffer, const GstBuffer &buffer,
    const GstMapInfo &map, int32_t stride)
{
    auto buf = const_cast<GstBuffer *>(&buffer);
    GstVideoMeta *videoMeta = gst_buffer_get_video_meta(buf);

    auto surfaceBufferAddr = surfaceBuffer->GetVirAddr();
    CHECK_AND_RETURN_RET_LOG(surfaceBufferAddr != nullptr, MSERR_NO_MEMORY, "Buffer addr is nullptr.");
    unsigned char *bufferAddr = static_cast<unsigned char *>(static_cast<void *>(surfaceBufferAddr));

    gsize calcSize = videoMeta->width * videoMeta->height * DEFAULT_STRIDE;
    CHECK_AND_RETURN_RET_LOG(map.size == calcSize, MSERR_NO_MEMORY, "size is error.");

    int32_t rowCopy = videoMeta->width * DEFAULT_STRIDE;
    unsigned char *currDstPos = bufferAddr;
    unsigned char *currSrcPos = map.data;
    gsize sizeCopy = 0;
    for (uint32_t height = 0; height < videoMeta->height; height++) {
        if ((sizeCopy + stride) > surfaceBuffer->GetSize()) {
            MEDIA_LOGE("sizeCopy is error");
            return MSERR_NO_MEMORY;
        }

        errno_t rc = memcpy_s(currDstPos, static_cast<size_t>(rowCopy), currSrcPos, static_cast<size_t>(rowCopy));
        CHECK_AND_RETURN_RET_LOG(rc == EOK, MSERR_NO_MEMORY, "memcpy_s failed");

        currDstPos += stride;
        currSrcPos += rowCopy;
        sizeCopy += stride;
    }
    SaveFrameToFile(bufferAddr, sizeCopy);
    return MSERR_OK;
}

void GstPlayerVideoRendererCtrl::CopyToSurfaceBuffer(sptr<SurfaceBuffer> surfaceBuffer,
    const GstBuffer &buffer, bool &needFlush)
{
    auto buf = const_cast<GstBuffer *>(&buffer);
    GstVideoMeta *videoMeta = gst_buffer_get_video_meta(buf);

    auto bufferImpl = SurfaceBufferImpl::FromBase(surfaceBuffer);
    CHECK_AND_RETURN_LOG(bufferImpl != nullptr, "bufferImpl is nullptr.");
    BufferHandle *bufferHandle = bufferImpl->GetBufferHandle();
    CHECK_AND_RETURN_LOG(bufferHandle != nullptr, "bufferHandle is nullptr.");

    int32_t stride = bufferHandle->stride;
    if ((stride % videoMeta->width) == 0) {
        int32_t ret = CopyDefault(surfaceBuffer, buffer);
        CHECK_AND_RETURN_LOG(ret == MSERR_OK, "CopyDefault error.");
        needFlush = true;
    } else {
        GstMapInfo map = GST_MAP_INFO_INIT;
        if (gst_buffer_map(buf, &map, GST_MAP_READ) != true) {
            MEDIA_LOGE("gst_buffer_map error");
            return;
        }

        int32_t ret = CopyRgba(surfaceBuffer, buffer, map, stride);
        if (ret != MSERR_OK) {
            MEDIA_LOGE("CopyRgba error");
            gst_buffer_unmap(buf, &map);
            return;
        }

        needFlush = true;
        gst_buffer_unmap(buf, &map);
    }
}

void GstPlayerVideoRendererCtrl::KpiFpsLog()
{
    uint64_t curTime = g_get_monotonic_time();
    if (flushBufferNums_ == 1) {
        lastFlushBufferNums_ = flushBufferNums_;
        lastFlushBufferTime_ = curTime;
        return;
    }
    uint64_t diff = curTime > lastFlushBufferTime_ ? curTime - lastFlushBufferTime_ : 0;
    if (diff >= GST_MSECOND) {
        double timeSec = static_cast<double>(diff) / GST_MSECOND;
        double fps = (flushBufferNums_ - lastFlushBufferNums_) / timeSec;
        MEDIA_LOGW("KPI-TRACE: fps=%{public}f, timeSec=%{public}f, render nums=%{public}" PRIu64 "",
            fps, timeSec, flushBufferNums_);
        lastFlushBufferNums_ = flushBufferNums_;
        lastFlushBufferTime_ = curTime;
    }
}

int32_t GstPlayerVideoRendererCtrl::UpdateSurfaceBuffer(const GstBuffer &buffer)
{
    CHECK_AND_RETURN_RET_LOG(producerSurface_ != nullptr, MSERR_INVALID_OPERATION,
        "Surface is nullptr.Video cannot be played.");
    if (surfaceTimeEnable_) {
        surfaceTimeMonitor_.StartTime();
    }
    auto buf = const_cast<GstBuffer *>(&buffer);
    GstVideoMeta *videoMeta = gst_buffer_get_video_meta(buf);
    CHECK_AND_RETURN_RET_LOG(videoMeta != nullptr, MSERR_INVALID_VAL, "gst_buffer_get_video_meta failed..");
    sptr<SurfaceBuffer> surfaceBuffer = RequestBuffer(videoMeta);
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer != nullptr, MSERR_INVALID_OPERATION, "surfaceBuffer is nullptr..");

    bool needFlush = false;
    CopyToSurfaceBuffer(surfaceBuffer, buffer, needFlush);
    if (needFlush) {
        BufferFlushConfig flushConfig = {};
        flushConfig.damage.x = 0;
        flushConfig.damage.y = 0;
        flushConfig.damage.w = videoMeta->width;
        flushConfig.damage.h = videoMeta->height;
        SurfaceError ret = producerSurface_->FlushBuffer(surfaceBuffer, -1, flushConfig);
        CHECK_AND_RETURN_RET_LOG(ret == SURFACE_ERROR_OK, MSERR_INVALID_OPERATION,
            "FlushBuffer failed(ret = %{public}d)..", ret);
    } else {
        (void)producerSurface_->CancelBuffer(surfaceBuffer);
        return MSERR_INVALID_OPERATION;
    }
    if (surfaceTimeEnable_) {
        surfaceTimeMonitor_.FinishTime();
    }
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
} // Media
} // OHOS
