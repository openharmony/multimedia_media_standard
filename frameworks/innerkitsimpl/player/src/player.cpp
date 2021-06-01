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

#include "player.h"
#include "audio_renderer_sink.h"
#include "display_type.h"
#include "hi_type.h"
#include "hilog/log.h"
#include "sdk.h"
#include "volume_control.h"

#include <gst/gst.h>
#include <gst/app/app.h>
#include <gst/player/player.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <pthread.h>
#include <securec.h>
#include <unistd.h>

using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;
using namespace std;

namespace PlayerConstants {
    const int FILE_URI_PREFIX_SIZE = 7;
    const std::string SOURCE_URI_PREFIX = "file://";
}

namespace OHOS {
namespace Media {
namespace {
    constexpr HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "Player"};
}

bool g_isSdkInit = false;
mutex g_mtx;
condition_variable g_cv;

struct PlayerPrivate : Player {
    PlayerPrivate(const GstCaps &audioCaps, const GstCaps &videoCaps);
    virtual ~PlayerPrivate();

    struct ErrorInfo {
        int32_t errorType;
        int32_t errorCode;
    };

    int32_t SetSource(const std::string &uri) override;
    int32_t Prepare() override;
    int32_t Play() const override;
    bool IsPlaying() const override;
    int32_t Pause() const override;
    int32_t Stop() const override;
    int32_t Rewind(int64_t seconds, int32_t mode) const override;
    int32_t SetVolume(float leftVolume, float rightVolume) const override;
    int32_t GetCurrentTime(int64_t &time) const override;
    int32_t GetDuration(int64_t &duration) const override;
    int32_t Reset() const override;
    int32_t Release() const override;
    int32_t EnableSingleLooping(bool loop) override;
    bool IsSingleLooping() const override;
    void SetPlayerCallback(const PlayerCallback &cb) override;
    int32_t SetVideoSurface(sptr<Surface> surface) override;

    int32_t PullVideoBuffer() const;
    int32_t UpdateSurfaceBuffer(const GstBuffer &buffer) const;

    void UpdateCoreErrorInfo(ErrorInfo &errorInfo, gint code) const;
    ErrorInfo GetErrorInfo(GQuark domain, gint code) const;

    static void OnMessageCb(const Player::State newState, const PlayerPrivate *self);
    static void StateChangedCb(const GstPlayer *player, GstPlayerState state,
                               const PlayerPrivate *self);
    static void EndOfStreamCb(const GstPlayer *player, const PlayerPrivate *self);
    static void ErrorCb(const GstPlayer *player, const GError *err, const PlayerPrivate *self);
    static void MediaInfoUpdatedCb(const GstPlayer *player, const GstPlayerMediaInfo *info,
                                   const PlayerPrivate *self);
    static void SeekDoneCb(const GstPlayer *player, guint64 position,
                           const PlayerPrivate *self);
    static void PositionUpdatedCb(const GstPlayer *player, guint64 position,
                                  const PlayerPrivate *self);

    using DataAvailableFunc = GstFlowReturn (*)(GstAppSink *appsink, gpointer user_data);
    static GstElement *CreateSink(GstCaps *caps, DataAvailableFunc callback, gpointer user_data);
    static GstElement *CreateVideoSink(GstPlayerVideoRenderer *self, GstPlayer *player);
    static GstFlowReturn VideoDataAvailableCb(GstAppSink *appsink, gpointer user_data);
    static GstFlowReturn AudioDataAvailableCb(GstAppSink *appsink, gpointer user_data);
    static GstPadProbeReturn SinkPadProbeCb(GstPad *pad, GstPadProbeInfo *info,
                                                gpointer user_data);

    gboolean mRepeat;
    GstPlayer *mPlayer;
    GMainLoop *mLoop;
    GMainContext *mContext;
    pthread_t mPlayerThread;
    State mState;
    PlayerCallback *mPlayerCallback;
    std::string mLocation;
    GstElement *mVideoSink;
    GstElement *mAudioSink;
    GstCaps *mVideoCaps;
    GstCaps *mAudioCaps;
    sptr<Surface> mProducerSurface;
    AudioRendererSink *mAudioRendererSink_;
    std::shared_ptr<VolumeControl> mVolumeControl_;
};

#define CHK_NULL_RETURN(ptr) \
do { \
    if ((ptr) == nullptr) { \
        HiLog::Error(LABEL, "%s null", #ptr); \
        return PLAYER_FAIL; \
    } \
} while (0)

#define CHK_NULL_RETURN_ERROR(ptr, ret) \
do { \
    if ((ptr) == nullptr) { \
        HiLog::Error(LABEL, "%s null", #ptr); \
        return ret; \
    } \
} while (0)

#define CHECK_NOT_POSITIVE_RETURN(val, ret) \
if (val < 0) { \
    return ret; \
}

struct _PlayerVideoRenderer {
    GObject parent;
    PlayerPrivate *player_;
};

static bool g_audioRendererSinkInitialized = FALSE;

static void player_video_renderer_interface_init(GstPlayerVideoRendererInterface *iface)
{
    HiLog::Info(LABEL, "Video renderer interface init");
    iface->create_video_sink = PlayerPrivate::CreateVideoSink;
}

#define PLAYER_TYPE_VIDEO_RENDERER player_video_renderer_get_type()
    G_DECLARE_FINAL_TYPE(PlayerVideoRenderer, player_video_renderer,
                            PLAYER, VIDEO_RENDERER, GObject)

G_DEFINE_TYPE_WITH_CODE(PlayerVideoRenderer, player_video_renderer, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(GST_TYPE_PLAYER_VIDEO_RENDERER,
                                              player_video_renderer_interface_init));

static void player_video_renderer_init(PlayerVideoRenderer *self) {}

static void player_video_renderer_class_init(PlayerVideoRendererClass *klass) {}

static GstPlayerVideoRenderer *player_video_renderer_new(PlayerPrivate *player)
{
    HiLog::Info(LABEL, "Video renderer new in");
    PLAYER_VIDEO_RENDERER(nullptr);
    PLAYER_IS_VIDEO_RENDERER(nullptr);
    PlayerVideoRenderer *self = (PlayerVideoRenderer *)g_object_new(PLAYER_TYPE_VIDEO_RENDERER,
                                                                    nullptr);
    self->player_ = player;
    return (GstPlayerVideoRenderer *)self;
}

Player::~Player() = default;
Player::PlayerCallback::~PlayerCallback() = default;

std::shared_ptr<Player> Player::Create(const std::string &audioType, const std::string &videoType)
{
    HiLog::Info(LABEL, "Player Create in");

    if (!g_setenv("GST_PLUGIN_PATH", "/system/lib", TRUE)) {
        HiLog::Info(LABEL, "environment set failed");
        return nullptr;
    }

    GstCaps *audioCaps = gst_caps_new_empty_simple(audioType.c_str());
    if (audioCaps == nullptr) {
        g_print("audioCaps is null");
        return nullptr;
    }
    GstCaps *videoCaps = gst_caps_new_empty_simple(videoType.c_str());
    if (videoCaps == nullptr) {
        g_print("videoCaps is null");
        gst_caps_unref(audioCaps);
        return nullptr;
    }
    GstCaps audCaps = *audioCaps;
    GstCaps vidCaps = *videoCaps;
    gst_caps_unref(audioCaps);
    gst_caps_unref(videoCaps);
    return std::make_shared<PlayerPrivate>(audCaps, vidCaps);
}

PlayerPrivate::PlayerPrivate(const GstCaps &audioCaps, const GstCaps &videoCaps)
{
    HiLog::Info(LABEL, "Player ctor");

    HI_S32 ret = sdk_init();
    if (ret == SDK_INIT_SUCC) {
        g_isSdkInit = true;
        HiLog::Error(LABEL, "sdk intialized successfully");
    } else if (ret == SDK_INIT_REENTRY) {
        HiLog::Error(LABEL, "sdk already intialized");
    } else {
        HiLog::Error(LABEL, "sdk init failed, ret = %d", ret);
    }

    mRepeat = false;
    mPlayerCallback = nullptr;
    mState = Player::PLAYER_STATE_STOPPED;
    mPlayer = nullptr;
    mLoop = nullptr;
    mContext = nullptr;
    GstCaps *audioGstCaps = (GstCaps *)&audioCaps;
    GstCaps *videoGstCaps = (GstCaps *)&videoCaps;
    mAudioCaps = audioGstCaps ? gst_caps_ref(audioGstCaps) : nullptr;
    mVideoCaps = videoGstCaps ? gst_caps_ref(videoGstCaps) : nullptr;
    mAudioSink = nullptr;
    mVideoSink = nullptr;
    mProducerSurface = nullptr;
    mAudioRendererSink_ = nullptr;
    mVolumeControl_ = nullptr;
}

PlayerPrivate::~PlayerPrivate()
{
    HiLog::Info(LABEL, "Player dtor");
    g_clear_pointer(&mAudioCaps, gst_caps_unref);
    g_clear_pointer(&mVideoCaps, gst_caps_unref);

    delete mAudioRendererSink_;

    if (g_isSdkInit) {
        sdk_exit();
        g_isSdkInit = false;
        HiLog::Info(LABEL, "sdk exited successfully");
    }
}

int32_t PlayerPrivate::SetSource(const std::string &uri)
{
    CHK_NULL_RETURN(uri.c_str());
    HiLog::Info(LABEL, "SetSource in");
    if (uri.empty() || (uri.find(PlayerConstants::SOURCE_URI_PREFIX) != 0)) {
        HiLog::Error(LABEL, "Uri string is invalid");
        return EINVAL;
    }

    char uriPath[PATH_MAX];
    std::string path = uri.substr(PlayerConstants::FILE_URI_PREFIX_SIZE);
    if (realpath(path.c_str(), uriPath) == nullptr) {
        HiLog::Error(LABEL, "Uri path not found");
        return ENOENT;
    }

    if (access(uriPath, R_OK) == -1) {
        HiLog::Error(LABEL, "Uri access failed");
        return EACCES;
    }

    std::ifstream inputStream(path, std::ios::ate);
    if (inputStream.tellg() == 0) {
        HiLog::Error(LABEL, "Source file is empty");
        return ENODATA;
    }

    mLocation = uri;

    return PLAYER_SUCCESS;
}

int32_t SetCallbacks(const PlayerPrivate &player)
{
    int32_t ret;
    auto self = const_cast<PlayerPrivate *>(&player);

    ret = g_signal_connect(self->mPlayer, "state-changed",
                           G_CALLBACK(PlayerPrivate::StateChangedCb), self);
    CHECK_NOT_POSITIVE_RETURN(ret, PLAYER_PREPARE_ERR);
    ret = g_signal_connect(self->mPlayer, "end-of-stream",
                           G_CALLBACK(PlayerPrivate::EndOfStreamCb), self);
    CHECK_NOT_POSITIVE_RETURN(ret, PLAYER_PREPARE_ERR);
    ret = g_signal_connect(self->mPlayer, "error", G_CALLBACK(PlayerPrivate::ErrorCb), self);
    CHECK_NOT_POSITIVE_RETURN(ret, PLAYER_PREPARE_ERR);
    ret = g_signal_connect(self->mPlayer, "media-info-updated",
                           G_CALLBACK(PlayerPrivate::MediaInfoUpdatedCb), self);
    CHECK_NOT_POSITIVE_RETURN(ret, PLAYER_PREPARE_ERR);
    ret = g_signal_connect(self->mPlayer, "seek-done",
                           G_CALLBACK(PlayerPrivate::SeekDoneCb), self);
    CHECK_NOT_POSITIVE_RETURN(ret, PLAYER_PREPARE_ERR);
    ret = g_signal_connect(self->mPlayer, "position-updated",
                           G_CALLBACK(PlayerPrivate::PositionUpdatedCb), self);
    CHECK_NOT_POSITIVE_RETURN(ret, PLAYER_PREPARE_ERR);

    return PLAYER_SUCCESS;
}

static void *PlayerLoop(void *param)
{
    HiLog::Info(LABEL, "Player thread in");

    if (param == nullptr) {
        return nullptr;
    }

    auto self = (PlayerPrivate *)param;
    self->mContext =  g_main_context_new();
    g_main_context_push_thread_default(self->mContext);

    auto videoRenderer = player_video_renderer_new(self);
    self->mPlayer = gst_player_new(videoRenderer,
                                   gst_player_g_main_context_signal_dispatcher_new(self->mContext));
    g_object_set(self->mPlayer, "uri", self->mLocation.c_str(), NULL);

    if (SetCallbacks(*self) == PLAYER_PREPARE_ERR) {
        HiLog::Error(LABEL, "Set callbacks failed");
        gst_object_unref(self->mPlayer);
        self->mPlayer = nullptr;
        g_main_context_unref(self->mContext);
        self->mContext = nullptr;
        return nullptr;
    }

    g_cv.notify_all();

    HiLog::Info(LABEL, "Create the loop for the current context");
    self->mLoop = g_main_loop_new(self->mContext, FALSE);
    g_main_loop_run(self->mLoop);
    g_main_loop_unref (self->mLoop);
    self->mLoop = nullptr;
    g_main_context_pop_thread_default(self->mContext);

    return nullptr;
}

int32_t PlayerPrivate::Prepare()
{
    HiLog::Info(LABEL, "Prepare in");

    gst_init(nullptr, nullptr);

    if (mAudioRendererSink_ == nullptr) {
        mAudioRendererSink_ = new AudioRendererSink();
    }

    if (mVolumeControl_ == nullptr) {
        mVolumeControl_ = std::make_shared<VolumeControl>();
    }

    pthread_create(&mPlayerThread, nullptr, &PlayerLoop, this);

    if (!mPlayer) {
        HiLog::Info(LABEL, "Player not yet initialized, wait for 1 second");
        unique_lock<mutex> lock(g_mtx);
        g_cv.wait_for(lock, std::chrono::seconds(1));
        CHK_NULL_RETURN_ERROR(mPlayer, PLAYER_PREPARE_ERR);
    }

    return PLAYER_SUCCESS;
}

int32_t PlayerPrivate::Play() const
{
    HiLog::Info(LABEL, "Play in");
    // Set pipeline playing state
    gst_player_play(mPlayer);

    return PLAYER_SUCCESS;
}

int32_t PlayerPrivate::Pause() const
{
    HiLog::Info(LABEL, "Pause in");
    if (mState == Player::PLAYER_STATE_PLAYING) {
        gst_player_pause(mPlayer);
    } else {
        gst_player_play(mPlayer);
    }

    return PLAYER_SUCCESS;
}

int32_t PlayerPrivate::GetDuration(int64_t &duration) const
{
    int64_t currDuration = gst_player_get_duration(mPlayer);
    currDuration /= MICRO;
    HiLog::Info(LABEL, "Duration in milli seconds: %lld", currDuration);
    duration = currDuration;

    return PLAYER_SUCCESS;
}

int32_t PlayerPrivate::GetCurrentTime(int64_t &time) const
{
    int64_t currTime = gst_player_get_position(mPlayer);
    currTime /= MICRO;
    HiLog::Info(LABEL, "Time in milli seconds: %lld", currTime);
    time = currTime;

    return PLAYER_SUCCESS;
}

int32_t PlayerPrivate::EnableSingleLooping(bool loop)
{
    HiLog::Info(LABEL, "EnableSingleLooping in");
    mRepeat = loop;

    return PLAYER_SUCCESS;
}

int32_t PlayerPrivate::Rewind(int64_t seconds, int32_t mode) const
{
    HiLog::Info(LABEL, "Rewind in");
    gst_player_seek(mPlayer, (seconds * NANO));

    return PLAYER_SUCCESS;
}

int32_t PlayerPrivate::SetVolume(float leftVolume, float rightVolume) const
{
    HiLog::Info(LABEL, "SetVolume in");
    if ((leftVolume < 0) || (leftVolume > 1) ||
        (rightVolume < 0) || (rightVolume > 1)) {
        HiLog::Info(LABEL, "Invalid Volume Input!");
        return PLAYER_SET_VOLUME_ERR;
    }

    /* If AudioRendererSink is not initialized, we will save the volume to
     * apply during play, on receive of first Audio frame
     */
    if (!g_audioRendererSinkInitialized) {
        mVolumeControl_->SaveVolume(leftVolume, rightVolume);
    } else {
        mVolumeControl_->SetVolume(leftVolume, rightVolume);
    }

    return PLAYER_SUCCESS;
}

bool PlayerPrivate::IsSingleLooping() const
{
    HiLog::Info(LABEL, "IsSingleLooping in");

    return mRepeat;
}

bool PlayerPrivate::IsPlaying() const
{
    HiLog::Info(LABEL, "IsPlaying in");
    return mState == State::PLAYER_STATE_PLAYING;
}

int32_t PlayerPrivate::Stop() const
{
    HiLog::Info(LABEL, "Stop in");
    if ((mState == State::PLAYER_STATE_PLAYING) || (mState == State::PLAYER_STATE_PAUSED)) {
        gst_player_stop(mPlayer);
    } else {
        return PLAYER_STOP_ERR;
    }

    return PLAYER_SUCCESS;
}

int32_t PlayerPrivate::Reset() const
{
    HiLog::Info(LABEL, "Reset in");
    gst_player_stop(mPlayer);

    return PLAYER_SUCCESS;
}

int32_t PlayerPrivate::Release() const
{
    HiLog::Info(LABEL, "Release in");
    gst_object_unref(mPlayer);
    g_main_context_unref(mContext);

    if (mLoop && g_main_loop_is_running(mLoop)) {
        HiLog::Info(LABEL, "Main loop still running, quit");
        g_main_loop_quit(mLoop);
    }

    HiLog::Info(LABEL, "Wait for playback thread to complete");
    pthread_join(mPlayerThread, nullptr);

    gst_deinit();

    mAudioRendererSink_->Close();
    return PLAYER_SUCCESS;
}

void PlayerPrivate::SetPlayerCallback(const PlayerCallback &cb)
{
    auto playerCallBackPtr = const_cast<PlayerCallback *>(&cb);
    mPlayerCallback = playerCallBackPtr;
}

void PlayerPrivate::EndOfStreamCb(const GstPlayer *player, const PlayerPrivate *self)
{
    HiLog::Info(LABEL, "End of stream");
    auto playerPrivate = const_cast<PlayerPrivate *>(self);
    if (playerPrivate == nullptr) {
        g_print("playerPrivate is null");
        return;
    }
    if (playerPrivate->mRepeat) {
        HiLog::Info(LABEL, "Repeat playback");
        gst_player_play((GstPlayer *)player);
    } else {
        g_main_loop_quit(playerPrivate->mLoop);
    }

    if (playerPrivate->mPlayerCallback == nullptr) {
        g_print("playerPrivate->mPlayerCallback is null");
        return;
    }
    playerPrivate->mPlayerCallback->OnEndOfStream(playerPrivate->shared_from_this());
}

void PlayerPrivate::OnMessageCb(const Player::State newState, const PlayerPrivate *self)
{
    HiLog::Info(LABEL, "On Message callback");
    auto playerPrivate = const_cast<PlayerPrivate *>(self);
    if (playerPrivate == nullptr) {
        g_print("playerPrivate is null");
        return;
    }
    // INFO: mState is the previous state and newState is the current state of GstPlayer
    int32_t infoType = MessageType::PLAYER_INFO_UNKNOWN;
    if (newState == State::PLAYER_STATE_BUFFERING) {
        infoType = MessageType::PLAYER_INFO_BUFFERING_START;
    } else if (newState == PLAYER_STATE_PLAYING) {
        infoType = MessageType::PLAYER_INFO_VIDEO_RENDERING_START;
    } else if (playerPrivate->mState == State::PLAYER_STATE_BUFFERING) {
        infoType = MessageType::PLAYER_INFO_BUFFERING_END;
    } else {
        HiLog::Info(LABEL, "Message Other state");
    }

    playerPrivate->mPlayerCallback->OnMessage(infoType, -1, playerPrivate->shared_from_this());
}

void PlayerPrivate::StateChangedCb(const GstPlayer *player, GstPlayerState state,
                                   const PlayerPrivate *self)
{
    HiLog::Info(LABEL, "State changed: %s", gst_player_state_get_name(state));
    auto playerPrivate = const_cast<PlayerPrivate *>(self);
    if (playerPrivate == nullptr) {
        g_print("playerPrivate is null");
        return;
    }
    State newState = (State)state;
    if (playerPrivate->mState != newState) {
        OnMessageCb(newState, playerPrivate);
    }
    playerPrivate->mState = newState;
    playerPrivate->mPlayerCallback->OnStateChanged(playerPrivate->mState,
                                                   playerPrivate->shared_from_this());
}

void PlayerPrivate::ErrorCb(const GstPlayer *player, const GError *err, const PlayerPrivate *self)
{
    HiLog::Info(LABEL, "Received error signal from pipeline.");
    // If looping is enabled, then disable it else will keep looping forever
    auto playerPrivate = const_cast<PlayerPrivate *>(self);
    if (playerPrivate == nullptr) {
        g_print("playerPrivate is null");
        return;
    }
    playerPrivate->mRepeat = FALSE;
    g_main_loop_quit(playerPrivate->mLoop);

    PlayerPrivate::ErrorInfo errorInfo = playerPrivate->GetErrorInfo(err->domain, err->code);
    playerPrivate->mPlayerCallback->OnError(errorInfo.errorType, errorInfo.errorCode,
                                            playerPrivate->shared_from_this());
}

void PlayerPrivate::MediaInfoUpdatedCb(const GstPlayer *player, const GstPlayerMediaInfo *info,
                                       const PlayerPrivate *self)
{
    auto playerPrivate = const_cast<PlayerPrivate *>(self);
    if (playerPrivate == nullptr) {
        g_print("playerPrivate is null");
        return;
    }
    playerPrivate->mPlayerCallback->OnMediaTracksChanged(playerPrivate->shared_from_this());
}

void PlayerPrivate::SeekDoneCb(const GstPlayer *player, guint64 position,
                               const PlayerPrivate *self)
{
    auto playerPrivate = const_cast<PlayerPrivate *>(self);
    if (playerPrivate == nullptr) {
        g_print("playerPrivate is null");
        return;
    }
    playerPrivate->mPlayerCallback->OnSeekDone(playerPrivate->shared_from_this());
}

void PlayerPrivate::PositionUpdatedCb(const GstPlayer *player, guint64 position,
                                      const PlayerPrivate *self)
{
    auto playerPrivate = const_cast<PlayerPrivate *>(self);
    if (playerPrivate == nullptr) {
        g_print("playerPrivate is null");
        return;
    }
    playerPrivate->mPlayerCallback->OnPositionUpdated(playerPrivate->shared_from_this(), position);
}

GstElement *PlayerPrivate::CreateSink(GstCaps *caps, DataAvailableFunc callback,
                                      gpointer user_data)
{
    HiLog::Info(LABEL, "CreateSink in.");
    auto sink = gst_element_factory_make("appsink", nullptr);
    GstAppSinkCallbacks sinkCallbacks = {nullptr, nullptr, callback};
    gst_app_sink_set_callbacks(GST_APP_SINK(sink), &sinkCallbacks, user_data, nullptr);
    gst_app_sink_set_caps(GST_APP_SINK(sink), caps);
    g_object_set(sink, "sync", false, nullptr);

    GstPad *pad = gst_element_get_static_pad(sink, "sink");
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM, SinkPadProbeCb, user_data,
                      nullptr);

    return sink;
}

GstElement *PlayerPrivate::CreateVideoSink(GstPlayerVideoRenderer *renderer, GstPlayer *player)
{
    HiLog::Info(LABEL, "CreateVideoSink in.");
    gint rate = 44100;
    gint channels = 2;
    auto self = ((PlayerVideoRenderer *)renderer)->player_;
    GstElement *playbin = gst_player_get_pipeline(player);
    if (self->mAudioCaps) {
        self->mAudioCaps = gst_caps_new_simple("audio/x-raw",
                                               "format", G_TYPE_STRING, "S16LE",
                                               "rate", G_TYPE_INT, rate,
                                               "channels", G_TYPE_INT, channels, NULL);
        self->mAudioSink = CreateSink(self->mAudioCaps, AudioDataAvailableCb, self);
        g_object_set(playbin, "audio-sink", self->mAudioSink, nullptr);
    }

    if (self->mVideoCaps) {
        self->mVideoCaps = gst_caps_new_simple("video/x-raw",
                                               "format", G_TYPE_STRING, "RGBA", NULL);
        self->mVideoSink = CreateSink(self->mVideoCaps, VideoDataAvailableCb, self);
        return self->mVideoSink;
    }

    return nullptr;
}

GstFlowReturn PlayerPrivate::VideoDataAvailableCb(GstAppSink *appsink, gpointer user_data)
{
    HiLog::Info(LABEL, "Video data available in.");
    auto self = reinterpret_cast<PlayerPrivate *>(user_data);
    int32_t ret = self->PullVideoBuffer();
    if (ret != PLAYER_SUCCESS) {
        return GST_FLOW_ERROR;
    }

    return GST_FLOW_OK;
}

GstFlowReturn PlayerPrivate::AudioDataAvailableCb(GstAppSink *appsink, gpointer user_data)
{
    auto self = reinterpret_cast<PlayerPrivate *>(user_data);
    GstAppSink *audioAppSink = GST_APP_SINK(self->mAudioSink);
    GstSample *sample = gst_app_sink_pull_sample(audioAppSink);
    if (!sample) {
        return GST_FLOW_OK;
    }

    if (!g_audioRendererSinkInitialized) {
        HiLog::Info(LABEL, "Audio renderer init");
        auto caps = gst_sample_get_caps(sample);

        gint rate;
        gint channels;
        std::string format;
        const GstStructure *str = gst_caps_get_structure(caps, 0);
        gst_structure_get_int(str, "rate", &rate);
        format = gst_structure_get_string(str, "format");
        gst_structure_get_int(str, "channels", &channels);
        HiLog::Info(LABEL, "Audio Sample rate %d Channels %d", rate, channels);
        HiLog::Info(LABEL, "Audio Format %s", format.c_str());

        /* Call AudiorendererSink Prepare */
        if (!self->mAudioRendererSink_->Prepare(*caps)) {
            HiLog::Info(LABEL, "Audio renderer prepare failed");
            return GST_FLOW_OK;
        }

        /* Pass the AudioRendererSink instance to Audio Manager */
        self->mVolumeControl_->SetRenderer(self->mAudioRendererSink_);

        /* As Audio Caps are unavailable before playback from gst pipeline,
         * We Set the volume here at the first Audio frame
         */
        int ret = self->mVolumeControl_->SetCurrentVolume();
        if (ret) {
            HiLog::Info(LABEL, "Setting Volume from First Audio frame failed!");
        }

        g_audioRendererSinkInitialized = TRUE;
    }

    auto buf = gst_sample_get_buffer(sample);
    uint size = gst_buffer_get_size(buf);

    char *audioOut = new (std::nothrow) char[size];
    if (audioOut) {
        gst_buffer_extract(buf, 0, audioOut, size);
        self->mAudioRendererSink_->Write(audioOut, size);
        delete[] audioOut;
    } else {
        return GST_FLOW_ERROR;
    }

    return GST_FLOW_OK;
}

GstPadProbeReturn PlayerPrivate::SinkPadProbeCb(GstPad *pad, GstPadProbeInfo *info,
                                                gpointer user_data)
{
    HiLog::Info(LABEL, "Sink pad probe.");
    GstQuery *query = GST_PAD_PROBE_INFO_QUERY(info);
    if (GST_QUERY_TYPE(query) == GST_QUERY_ALLOCATION) {
        GstCaps *caps = nullptr;
        gboolean needPool;
        gst_query_parse_allocation(query, &caps, &needPool);

        auto s = gst_caps_get_structure(caps, 0);
        auto mediaType = gst_structure_get_name(s);
        bool isVideo = g_str_has_prefix(mediaType, "video/");
        if (isVideo) {
            gst_query_add_allocation_meta(query, GST_VIDEO_META_API_TYPE, NULL);
        }
    }
    return GST_PAD_PROBE_OK;
}

int32_t PlayerPrivate::SetVideoSurface(sptr<Surface> surface)
{
    HiLog::Info(LABEL, "SetVideoSurface in");
    CHK_NULL_RETURN(surface);
    mProducerSurface = surface;

    return PLAYER_SUCCESS;
}

int32_t PlayerPrivate::UpdateSurfaceBuffer(const GstBuffer &buffer) const
{
    auto buf = const_cast<GstBuffer *>(&buffer);
    GstVideoMeta *videoMeta = gst_buffer_get_video_meta(buf);

    int size = gst_buffer_get_size(buf);
    if (size <= 0) {
        return PLAYER_BUFFER_EXTRACT_ERR;
    }

    sptr<SurfaceBuffer> surfaceBuffer;
    int32_t releaseFence;
    BufferRequestConfig requestConfig;
    requestConfig.width = videoMeta->width;
    requestConfig.height = videoMeta->height;
    requestConfig.strideAlignment
                        = std::stoi(mProducerSurface->GetUserData(SURFACE_STRIDE_ALIGNMENT));
    requestConfig.format = std::stoi(mProducerSurface->GetUserData(SURFACE_FORMAT));
    requestConfig.usage = HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA;
    requestConfig.timeout = 0;

    SurfaceError ret = mProducerSurface->RequestBuffer(surfaceBuffer, releaseFence, requestConfig);
    if (ret != SURFACE_ERROR_OK) {
        return PLAYER_REQUEST_BUFFER_ERR;
    }

    gst_buffer_extract(buf, 0, surfaceBuffer->GetVirAddr(), size);

    BufferFlushConfig flushConfig;
    flushConfig.damage.x = 0;
    flushConfig.damage.y = 0;
    flushConfig.damage.w = videoMeta->width;
    flushConfig.damage.h =  videoMeta->height;

    mProducerSurface->FlushBuffer(surfaceBuffer, -1, flushConfig);

    return PLAYER_SUCCESS;
}

int32_t PlayerPrivate::PullVideoBuffer() const
{
    HiLog::Info(LABEL, "Pull buffer");

    if ((mVideoSink == nullptr) || (mProducerSurface == nullptr)) {
        return PLAYER_INVALID_ARGUMENT_ERR;
    }

    GstAppSink *appsink = GST_APP_SINK(mVideoSink);
    GstSample *sample = gst_app_sink_pull_sample(appsink);
    if (!sample) {
        return PLAYER_SAMPLE_EXTRACT_ERR;
    }

    auto buf = gst_sample_get_buffer(sample);
    if (!buf) {
        gst_sample_unref(sample);
        return PLAYER_BUFFER_EXTRACT_ERR;
    }

    int32_t ret = UpdateSurfaceBuffer(*buf);
    if (ret != PLAYER_SUCCESS) {
        gst_sample_unref(sample);
        return ret;
    }

    gst_sample_unref(sample);

    return PLAYER_SUCCESS;
}

void PlayerPrivate::UpdateCoreErrorInfo(PlayerPrivate::ErrorInfo &errorInfo, gint code) const
{
    int32_t errorType = PLAYER_ERROR;
    int32_t errorCode;
    switch (code) {
        case GST_CORE_ERROR_STATE_CHANGE: {
            errorCode = PlayerError::PLAYER_ERROR_STATE_CHANGE;
            break;
        }
        case GST_CORE_ERROR_MISSING_PLUGIN:
        case GST_CORE_ERROR_NEGOTIATION: {
            errorCode = PlayerError::PLAYER_FORMAT_NOT_SUPPORTED_ERROR;
            break;
        }
        case GST_CORE_ERROR_SEEK: {
            errorCode = PlayerError::PLAYER_ERROR_SEEK;
            break;
        }
        default: {
            errorCode = PlayerError::PLAYER_INTERNAL_ERROR;
            break;
        }
    }
    errorInfo.errorType = errorType;
    errorInfo.errorCode = errorCode;
}

PlayerPrivate::ErrorInfo PlayerPrivate::GetErrorInfo(GQuark domain, gint code) const
{
    PlayerPrivate::ErrorInfo errorInfo;
    if (domain == GST_CORE_ERROR) {
        UpdateCoreErrorInfo(errorInfo, code);
    } else if (domain == GST_LIBRARY_ERROR) {
        errorInfo.errorType = PLAYER_ERROR;
        errorInfo.errorCode = PLAYER_INTERNAL_ERROR;
    } else if (domain == GST_RESOURCE_ERROR) {
        errorInfo.errorType = PLAYER_IO_ERROR;
        errorInfo.errorCode = code;
    } else if (domain == GST_STREAM_ERROR) {
        errorInfo.errorType = PLAYER_STREAM_ERROR;
        errorInfo.errorCode = code;
    } else {
        errorInfo.errorType = PLAYER_ERROR;
        errorInfo.errorCode = PLAYER_INTERNAL_ERROR;
    }

    return errorInfo;
}
}  // namespace Media
}  // namespace OHOS
