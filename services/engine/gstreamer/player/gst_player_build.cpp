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

#include "gst_player_build.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "GstPlayerBuild"};
}

namespace OHOS {
namespace Media {
class GstPlayerSingnalDispatcherFactory {
public:
    GstPlayerSingnalDispatcherFactory() = delete;
    ~GstPlayerSingnalDispatcherFactory() = delete;

    static GstPlayerSignalDispatcher *Create(GMainContext *context)
    {
        CHECK_AND_RETURN_RET_LOG(context != nullptr, nullptr, "context is nullptr");
        return gst_player_g_main_context_signal_dispatcher_new(context);
    }

    static void Destroy(GstPlayerSignalDispatcher *dispatcher)
    {
        CHECK_AND_RETURN_LOG(dispatcher != nullptr, "dispatcher is nullptr");
        gst_object_unref(dispatcher);
    }
};

class GstPlayerFactory {
public:
    GstPlayerFactory() = delete;
    ~GstPlayerFactory() = delete;
    static GstPlayer *Create(GstPlayerVideoRenderer *renderer, GstPlayerSignalDispatcher *dispatcher)
    {
        CHECK_AND_RETURN_RET_LOG(renderer != nullptr, nullptr, "renderer is nullptr");
        CHECK_AND_RETURN_RET_LOG(dispatcher != nullptr, nullptr, "dispatcher is nullptr");
        return gst_player_new(renderer, dispatcher);
    }

    static void Destroy(GstPlayer *player)
    {
        CHECK_AND_RETURN_LOG(player != nullptr, "player is nullptr");
        gst_object_unref(player);
    }
};

GstPlayerBuild::GstPlayerBuild()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

GstPlayerBuild::~GstPlayerBuild()
{
    Release();
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

std::shared_ptr<GstPlayerCtrl> GstPlayerBuild::Build(sptr<Surface> surface)
{
    if (surface == nullptr) {
        MEDIA_LOGI("This is an audio scene.");
    } else {
        MEDIA_LOGI("This is an video scene.");
    }

    context_ =  g_main_context_new();
    CHECK_AND_RETURN_RET_LOG(context_ != nullptr, nullptr, "g_main_context_new failed..");

    g_main_context_push_thread_default(context_);

    rendererCtrl_ = std::make_shared<GstPlayerVideoRendererCtrl>(surface);
    if (rendererCtrl_ == nullptr) {
        Release();
        MEDIA_LOGE("rendererCtrl_ is nullptr");
        return nullptr;
    }

    videoRenderer_ = GstPlayerVideoRendererFactory::Create(rendererCtrl_);
    signalDispatcher_ = GstPlayerSingnalDispatcherFactory::Create(context_);
    if (signalDispatcher_ == nullptr || videoRenderer_ == nullptr) {
        Release();
        MEDIA_LOGE("signalDispatcher_ or videoRenderer_ is nullptr");
        return nullptr;
    }

    gstPlayer_ = GstPlayerFactory::Create(videoRenderer_, signalDispatcher_);
    playerCtrl_ = std::make_shared<GstPlayerCtrl>(gstPlayer_);
    if (gstPlayer_ == nullptr || playerCtrl_ == nullptr) {
        Release();
        MEDIA_LOGE("gstPlayer_ or playerCtrl_ is nullptr");
        return nullptr;
    }

    return playerCtrl_;
}

void GstPlayerBuild::CreateLoop()
{
    MEDIA_LOGI("Create the loop for the current context");
    CHECK_AND_RETURN_LOG(context_ != nullptr, "context_ is nullptr");

    loop_ = g_main_loop_new(context_, FALSE);
    CHECK_AND_RETURN_LOG(loop_ != nullptr, "gstPlayer_ is nullptr");

    GSource *source = g_idle_source_new();
    g_source_set_callback(source, (GSourceFunc)GstPlayerBuild::MainLoopRunCb, this,
    nullptr);
    guint ret = g_source_attach(source, context_);
    CHECK_AND_RETURN_LOG(ret > 0, "add idle source failed");
    g_source_unref(source);

    g_main_loop_run(loop_);
    // wait g_main_loop_quit
    g_main_loop_unref(loop_);
    loop_ = nullptr;
}

gboolean GstPlayerBuild::MainLoopRunCb(GstPlayerBuild *build)
{
    if (build == nullptr) {
        return G_SOURCE_REMOVE;
    }

    std::unique_lock<std::mutex> lock(build->mutex_);
    build->needWaiting_ = false;
    build->cond_.notify_one();

    return G_SOURCE_REMOVE;
}

void GstPlayerBuild::WaitMainLoopStart()
{
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this] { return !needWaiting_; }); // wait main loop run done
}

void GstPlayerBuild::DestroyLoop() const
{
    if (loop_ != nullptr && g_main_loop_is_running(loop_)) {
        MEDIA_LOGI("Main loop still running, quit");
        g_main_loop_quit(loop_);
    }
}

void GstPlayerBuild::Release()
{
    playerCtrl_ = nullptr;
    rendererCtrl_ = nullptr;

    if (gstPlayer_ != nullptr) {
        GstPlayerFactory::Destroy(gstPlayer_);
        gstPlayer_ = nullptr;
    }

    if (signalDispatcher_ != nullptr) {
        GstPlayerSingnalDispatcherFactory::Destroy(signalDispatcher_);
        signalDispatcher_ = nullptr;
    }

    if (videoRenderer_ != nullptr) {
        GstPlayerVideoRendererFactory::Destroy(videoRenderer_);
        videoRenderer_ = nullptr;
    }

    if (context_ != nullptr) {
        g_main_context_pop_thread_default(context_);
        g_main_context_unref(context_);
        context_ = nullptr;
    }
}
}
}
