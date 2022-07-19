/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef PLAYER_MOCK_H
#define PLAYER_MOCK_H

#include "player.h"
#include "media_data_source_test_noseek.h"
#include "media_data_source_test_seekable.h"
#include "test_params_config.h"
#include "window.h"

namespace OHOS {
namespace Media {
class PlayerSignal {
protected:
    PlayerStates state_ = PLAYER_IDLE;
    int32_t seekPosition_;
    bool seekDoneFlag_;
    bool speedDoneFlag_;
    PlayerSeekMode seekMode_ = PlayerSeekMode::SEEK_CLOSEST;
    std::mutex mutexCond_;
    std::condition_variable condVarPrepare_;
    std::condition_variable condVarPlay_;
    std::condition_variable condVarPause_;
    std::condition_variable condVarStop_;
    std::condition_variable condVarReset_;
    std::condition_variable condVarSeek_;
    std::condition_variable condVarSpeed_;
};

class PlayerCallbackTest : public PlayerCallback, public NoCopyable, public PlayerSignal {
public:
    ~PlayerCallbackTest() {}
    void OnError(PlayerErrorType errorType, int32_t errorCode) override {}
    void OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody) override;
    void SeekNotify(int32_t extra, const Format &infoBody);
    void Notify(PlayerStates currentState);
    void SetSeekDoneFlag(bool seekDoneFlag);
    void SetSpeedDoneFlag(bool speedDoneFlag);
    void SetSeekPosition(int32_t seekPosition);
    void SetState(PlayerStates state);
    int32_t PrepareSync();
    int32_t PlaySync();
    int32_t PauseSync();
    int32_t StopSync();
    int32_t ResetSync();
    int32_t SeekSync();
    int32_t SpeedSync();
};

class PlayerMock : public NoCopyable {
public:
    explicit PlayerMock(std::shared_ptr<PlayerCallbackTest> &callback);
    virtual ~PlayerMock();
    bool CreatePlayer();
    int32_t SetSource(const std::string url);
    int32_t SetDataSrc(const std::string &path, int32_t size, bool seekable);
    int32_t SetSource(const std::string &path, int64_t offset, int64_t size);
    int32_t Prepare();
    int32_t PrepareAsync();
    int32_t Play();
    int32_t Pause();
    int32_t Stop();
    int32_t Reset();
    int32_t Release();
    int32_t Seek(int32_t mseconds, PlayerSeekMode mode);
    int32_t SetVolume(float leftVolume, float rightVolume);
    int32_t SetLooping(bool loop);
    int32_t GetCurrentTime(int32_t &currentTime);
    int32_t GetVideoTrackInfo(std::vector<Format> &videoTrack);
    int32_t GetAudioTrackInfo(std::vector<Format> &audioTrack);
    int32_t GetVideoWidth();
    int32_t GetVideoHeight();
    int32_t GetDuration(int32_t &duration);
    int32_t SetPlaybackSpeed(PlaybackRateMode mode);
    int32_t GetPlaybackSpeed(PlaybackRateMode &mode);
    int32_t SelectBitRate(uint32_t bitRate);
    bool IsPlaying();
    bool IsLooping();
    int32_t SetParameter(const Format &param);
    int32_t SetPlayerCallback(const std::shared_ptr<PlayerCallback> &callback);
    int32_t SetVideoSurface(sptr<Surface> surface);
    sptr<Surface> GetVideoSurface();
private:
    void SeekPrepare(int32_t &mseconds, PlayerSeekMode &mode);
    std::shared_ptr<Player> player_ = nullptr;
    std::shared_ptr<MediaDataSourceTest> dataSrc_ = nullptr;
    std::shared_ptr<PlayerCallbackTest> callback_ = nullptr;
    sptr<Rosen::Window> previewWindow_ = nullptr;
    int32_t height_ = 1080;
    int32_t width_ = 1920;
    std::mutex mutex_;
};
} // namespace Media
} // namespace OHOS
#endif