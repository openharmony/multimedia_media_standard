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

namespace OHOS {
namespace Media {
class GstPlayerCtrl {
public:
    explicit GstPlayerCtrl(GstPlayer *gstPlayer);
    ~GstPlayerCtrl();

    int32_t SetUri(std::string uri);
    int32_t SetCallbacks(const std::weak_ptr<IPlayerEngineObs> &obs);
    void SetVideoTrack(bool enable);
    void Pause(bool cancelNotExecuted = false);
    void Play();
    void Seek(uint64_t position, const PlayerSeekMode mode);
    void Stop();
    void SetLoop(bool loop);
    void SetVolume(float leftVolume, float rightVolume);
    uint64_t GetPosition();
    uint64_t GetDuration();
    void SetRate(double rate);
    double GetRate();
    PlayerStates GetState();
    static void OnStateChangedCb(const GstPlayer *player, GstPlayerState state, const GstPlayerCtrl *self);
    static void OnEndOfStreamCb(const GstPlayer *player, const GstPlayerCtrl *self);
    static void StreamDecErrorParse(const gchar *name, int32_t &errorCode);
    static void StreamErrorParse(const gchar *name, const GError *err, int32_t &errorCode);
    static void ResourceErrorParse(const GError *err, int32_t &errorCode);
    static void MessageErrorProcess(const char *name, const GError *err,
        PlayerErrorType &errorType, int32_t &errorCode);
    static void ErrorProcess(const GstMessage *msg, PlayerErrorType &errorType, int32_t &errorCode);
    static void OnErrorCb(const GstPlayer *player, const GstMessage *msg, const GstPlayerCtrl *self);
    static void OnSeekDoneCb(const GstPlayer *player, guint64 position, const GstPlayerCtrl *self);
    static void OnPositionUpdatedCb(const GstPlayer *player, guint64 position, const GstPlayerCtrl *self);

private:
    PlayerStates ProcessStoppedState();
    PlayerStates ProcessPausedState();
    int32_t ChangeSeekModeToGstFlag(const PlayerSeekMode mode) const;
    void ProcessStateChanged(const GstPlayer *cbPlayer, GstPlayerState state);
    void ProcessSeekDone(const GstPlayer *cbPlayer, uint64_t position);
    void ProcessPositionUpdated(const GstPlayer *cbPlayer, uint64_t position);
    void ProcessEndOfStream(const GstPlayer *cbPlayer);
    void OnStateChanged(PlayerStates state);
    void OnVolumeChange();
    void OnSeekDone();
    void OnEndOfStream();
    void OnMessage(int32_t type, int32_t extra);
    void InitDuration();
    void PlaySync();
    void SeekSync(uint64_t position, const PlayerSeekMode mode);
    void MultipleSeek();
    void StopSync();
    void PauseSync();
    void OnNotify(PlayerStates state);

private:
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
    bool seekInProgress_ = false;
    bool userStop_ = false;
    bool stopTimeFlag_ = false;
    bool errorFlag_ = false;
    uint64_t nextSeekPos_ = 0;
    PlayerSeekMode nextSeekMode_ = SEEK_PREVIOUS_SYNC;
    PlayerStates currentState_ = PLAYER_IDLE;
    uint64_t sourceDuration_ = 0;
    uint64_t seekDonePosition_ = 0;
    bool seekDoneNeedCb_ = false;
    bool endOfStreamCb_ = false;
    std::vector<gulong> signalIds_;
};
} // Media
} // OHOS
#endif // GST_PLAYER_CTRL_H
