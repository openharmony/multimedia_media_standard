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

#ifndef GST_PLAYER_CTRL_H
#define GST_PLAYER_CTRL_H

#include <memory>
#include <string>
#include <vector>
#include <gst/gst.h>
#include <gst/player/player.h>
#include "i_player_engine.h"
#include "task_queue.h"
#include "gst_appsrc_warp.h"
#include "gst_player_track_parse.h"

namespace OHOS {
namespace Media {
class GstPlayerCtrl : public NoCopyable {
public:
    explicit GstPlayerCtrl(GstPlayer *gstPlayer);
    ~GstPlayerCtrl();

    int32_t SetUrl(const std::string &url);
    int32_t SetSource(const std::shared_ptr<GstAppsrcWarp> &appsrcWarp);
    int32_t SetCallbacks(const std::weak_ptr<IPlayerEngineObs> &obs);
    void SetVideoTrack(bool enable);
    void Pause();
    void Play();
    void Prepare();
    void PrepareAsync();
    int32_t Seek(uint64_t position, const PlayerSeekMode mode);
    void Stop();
    int32_t SetLoop(bool loop);
    void SetVolume(const float &leftVolume, const float &rightVolume);
    int32_t SetParameter(const Format &param);
    uint64_t GetPosition();
    uint64_t GetDuration();
    int32_t GetVideoTrackInfo(std::vector<Format> &videoTrack);
    int32_t GetAudioTrackInfo(std::vector<Format> &audioTrack);
    int32_t GetVideoWidth();
    int32_t GetVideoHeight();
    int32_t SetRate(double rate);
    double GetRate();
    PlayerStates GetState() const;
    void SetRingBufferMaxSize(uint64_t size);
    void SetBufferingInfo();
    void SetHttpTimeOut();
    static void OnStateChangedCb(const GstPlayer *player, GstPlayerState state, GstPlayerCtrl *playerGst);
    static void OnEndOfStreamCb(const GstPlayer *player, GstPlayerCtrl *playerGst);
    static void StreamDecErrorParse(const gchar *name, int32_t &errorCode);
    static void StreamErrorParse(const gchar *name, const GError *err, int32_t &errorCode);
    static void ResourceErrorParse(const GError *err, int32_t &errorCode);
    static void MessageErrorProcess(const char *name, const GError *err,
        PlayerErrorType &errorType, int32_t &errorCode);
    static void ErrorProcess(const GstMessage *msg, PlayerErrorType &errorType, int32_t &errorCode);
    static void OnErrorCb(const GstPlayer *player, const GstMessage *msg, GstPlayerCtrl *playerGst);
    static void OnSeekDoneCb(const GstPlayer *player, guint64 position, GstPlayerCtrl *playerGst);
    static void OnPositionUpdatedCb(const GstPlayer *player, guint64 position, GstPlayerCtrl *playerGst);
    static void OnVolumeChangeCb(const GObject *combiner, const GParamSpec *pspec, const GstPlayerCtrl *playerGst);
    static void OnSourceSetupCb(const GstPlayer *player, GstElement *src, GstPlayerCtrl *playerGst);
    static void OnElementSetupCb(const GstPlayer *player, GstElement *src, GstPlayerCtrl *playerGst);
    static void OnResolutionChanegdCb(const GstPlayer *player,
        int32_t width, int32_t height, GstPlayerCtrl *playerGst);
    static void OnCachedPercentCb(const GstPlayer *player, guint percent, GstPlayerCtrl *playerGst);
    static void OnBufferingTimeCb(const GstPlayer *player, guint64 bufferingTime, guint mqNumId,
        GstPlayerCtrl *playerGst);
    static void OnMqNumUseBufferingCb(const GstPlayer *player, guint mqNumUseBuffering, GstPlayerCtrl *playerGst);
    static GValueArray* OnAutoplugSortCb(const GstElement *uriDecoder, GstPad *pad, GstCaps *caps,
                                         GValueArray *factories, GstPlayerCtrl *playerGst);
private:
    PlayerStates ProcessStoppedState();
    PlayerStates ProcessPausedState();
    int32_t ChangeSeekModeToGstFlag(const PlayerSeekMode mode) const;
    void ProcessStateChanged(const GstPlayer *cbPlayer, GstPlayerState state);
    void ProcessSeekDone(const GstPlayer *cbPlayer, uint64_t position);
    void ProcessPositionUpdated(const GstPlayer *cbPlayer, uint64_t position);
    void ProcessEndOfStream(const GstPlayer *cbPlayer);
    void OnStateChanged(PlayerStates state);
    void OnVolumeChange() const;
    void OnSeekDone();
    void OnEndOfStream();
    void OnMessage(int32_t extra) const;
    void OnBufferingUpdate(const std::string Message) const;
    void OnResolutionChange(int32_t width, int32_t height);
    void InitDuration();
    void PlaySync();
    void SeekSync(uint64_t position, const PlayerSeekMode mode);
    void SetRateSync(double rate);
    void PauseSync();
    void OnNotify(PlayerStates state);
    void GetAudioSink();
    void GetVideoSink();
    void HandleStopNotify();
    void HandlePlayBackNotify();
    uint64_t GetPositionInner();
    void ProcessCachedPercent(const GstPlayer *cbPlayer, int32_t percent);
    void ProcessBufferingTime(const GstPlayer *cbPlayer, guint64 bufferingTime, guint mqNumId);
    void ProcessMqNumUseBuffering(const GstPlayer *cbPlayer, uint32_t mqNumUseBuffering);
    void RemoveGstPlaySinkVideoConvertPlugin();
    bool IsLiveMode() const;
    bool SetAudioRendererInfo(const Format &param);
    std::mutex mutex_;
    std::condition_variable condVarPlaySync_;
    std::condition_variable condVarPauseSync_;
    std::condition_variable condVarStopSync_;
    std::condition_variable condVarSeekSync_;
    std::condition_variable condVarPreparingSync_;
    GstPlayer *gstPlayer_ = nullptr;
    TaskQueue taskQue_;
    std::weak_ptr<IPlayerEngineObs> obs_;
    bool enableLooping_ = false;
    bool bufferingStart_ = false;
    bool nextSeekFlag_ = false;
    bool userStop_ = false;
    bool locatedInEos_ = false;
    bool stopTimeFlag_ = false;
    bool errorFlag_ = false;
    PlayerStates currentState_ = PLAYER_IDLE;
    uint64_t sourceDuration_ = 0;
    uint64_t seekDonePosition_ = 0;
    uint64_t bufferingTime_ = 0;
    int32_t percent_ = 0;
    uint32_t mqNumUseBuffering_ = 0;
    bool seekDoneNeedCb_ = false;
    bool endOfStreamCb_ = false;
    bool preparing_ = false;
    std::vector<gulong> signalIds_;
    gulong signalIdVolume_ = 0;
    GstElement *audioSink_ = nullptr;
    float volume_; // inited at the constructor
    std::shared_ptr<GstAppsrcWarp> appsrcWarp_ = nullptr;
    std::shared_ptr<ITaskHandler> seekTask_ = nullptr;
    std::shared_ptr<ITaskHandler> rateTask_ = nullptr;
    double rate_; // inited at the constructor
    uint64_t lastTime_ = 0;
    bool speeding_ = false;
    bool isExit_ = true;
    bool seeking_ = false;
    std::map<guint, guint64> mqBufferingTime_;
    std::shared_ptr<GstPlayerTrackParse> trackParse_ = nullptr;
    int32_t videoWidth_ = 0;
    int32_t videoHeight_ = 0;
    bool isHardWare_ = false;
    GstElement *videoSink_ = nullptr;
    bool isPlaySinkFlagsSet_ = false;
    bool isNetWorkPlay_ = false;
};
} // namespace Media
} // namespace OHOS
#endif // GST_PLAYER_CTRL_H
