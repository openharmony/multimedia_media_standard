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

namespace OHOS {
namespace Media {
class GstPlayerCtrl {
public:
    explicit GstPlayerCtrl(GstPlayer *gstPlayer);
    ~GstPlayerCtrl();
    DISALLOW_COPY_AND_MOVE(GstPlayerCtrl);
    int32_t SetUri(const std::string &uri);
    int32_t SetSource(const std::shared_ptr<GstAppsrcWarp> &appsrcWarp);
    int32_t SetCallbacks(const std::weak_ptr<IPlayerEngineObs> &obs);
    void SetVideoTrack(bool enable);
    void Pause(bool syncExecuted = false);
    void Play();
    int32_t Seek(uint64_t position, const PlayerSeekMode mode);
    void Stop();
    int32_t SetLoop(bool loop);
    void SetVolume(const float &leftVolume, const float &rightVolume);
    uint64_t GetPosition();
    uint64_t GetDuration();
    int32_t SetRate(double rate);
    double GetRate();
    PlayerStates GetState() const;
    void SetRingBufferMaxSize(uint64_t size);
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
    static void OnPositionUpdatedCb(const GstPlayer *player, guint64 position, const GstPlayerCtrl *playerGst);
    static void OnVolumeChangeCb(const GObject *combiner, const GParamSpec *pspec, const GstPlayerCtrl *playerGst);
    static void OnSourceSetupCb(const GstPlayer *player, GstElement *src, const GstPlayerCtrl *playerGst);

private:
    PlayerStates ProcessStoppedState();
    PlayerStates ProcessPausedState();
    int32_t ChangeSeekModeToGstFlag(const PlayerSeekMode mode) const;
    void ProcessStateChanged(const GstPlayer *cbPlayer, GstPlayerState state);
    void ProcessSeekDone(const GstPlayer *cbPlayer, uint64_t position);
    void ProcessPositionUpdated(const GstPlayer *cbPlayer, uint64_t position) const;
    void ProcessEndOfStream(const GstPlayer *cbPlayer);
    void OnStateChanged(PlayerStates state);
    void OnVolumeChange() const;
    void OnSeekDone();
    void OnEndOfStream();
    void OnMessage(int32_t extra) const;
    void InitDuration();
    void PlaySync();
    void SeekSync(uint64_t position, const PlayerSeekMode mode);
    void SetRateSync(double rate);
    void PauseSync();
    void OnNotify(PlayerStates state);
    void GetAudioSink();
    void HandleStopNotify();
    void HandlePlayBackNotify();
    std::mutex mutex_;
    std::condition_variable condVarPlaySync_;
    std::condition_variable condVarPauseSync_;
    std::condition_variable condVarStopSync_;
    std::condition_variable condVarSeekSync_;
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
    bool seekDoneNeedCb_ = false;
    bool endOfStreamCb_ = false;
    std::vector<gulong> signalIds_;
    gulong signalIdVolume_ = 0;
    GstElement *audioSink_ = nullptr;
    float volume_; // inited at the constructor
    std::shared_ptr<GstAppsrcWarp> appsrcWarp_ = nullptr;
    std::shared_ptr<ITaskHandler> seekTask_ = nullptr;
    std::shared_ptr<ITaskHandler> rateTask_ = nullptr;
    double rate_; // inited at the constructor
};
} // Media
} // OHOS
#endif // GST_PLAYER_CTRL_H
