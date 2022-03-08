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

#ifndef PLAYER_ENGINE_GST_IMPL
#define PLAYER_ENGINE_GST_IMPL

#include <mutex>
#include <cstdint>
#include <thread>
#include <map>

#include "i_player_engine.h"
#include "gst_player_build.h"
#include "gst_player_ctrl.h"
#include "gst_appsrc_warp.h"

namespace OHOS {
namespace Media {
class PlayerEngineGstImpl : public IPlayerEngine, public NoCopyable {
public:
    PlayerEngineGstImpl();
    ~PlayerEngineGstImpl();

    int32_t SetSource(const std::string &url) override;
    int32_t SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc) override;
    int32_t SetObs(const std::weak_ptr<IPlayerEngineObs> &obs) override;
    int32_t SetVideoSurface(sptr<Surface> surface) override;
    int32_t Prepare() override;
    int32_t PrepareAsync() override;
    int32_t Play() override;
    int32_t Pause() override;
    int32_t Stop() override;
    int32_t Reset() override;
    int32_t SetVolume(float leftVolume, float rightVolume) override;
    int32_t Seek(int32_t mSeconds, PlayerSeekMode mode) override;
    int32_t GetCurrentTime(int32_t &currentTime) override;
    int32_t GetVideoTrackInfo(std::vector<Format> &videoTrack) override;
    int32_t GetAudioTrackInfo(std::vector<Format> &audioTrack) override;
    int32_t GetVideoWidth() override;
    int32_t GetVideoHeight() override;
    int32_t GetDuration(int32_t &duration) override;
    int32_t SetPlaybackSpeed(PlaybackRateMode mode) override;
    int32_t GetPlaybackSpeed(PlaybackRateMode &mode) override;
    int32_t SetParameter(const Format &param) override;
    int32_t SetLooping(bool loop) override;

private:
    double ChangeModeToSpeed(const PlaybackRateMode &mode) const;
    PlaybackRateMode ChangeSpeedToMode(double rate) const;
    int32_t GstPlayerInit();
    int32_t GstPlayerPrepare() const;
    void PlayerLoop();
    void GstPlayerDeInit();
    int32_t GetRealPath(const std::string &url, std::string &realUrlPath) const;
    bool IsFileUrl(const std::string &url) const;
    std::mutex mutex_;
    std::mutex mutexSync_;
    std::unique_ptr<GstPlayerBuild> playerBuild_ = nullptr;
    std::shared_ptr<GstPlayerCtrl> playerCtrl_ = nullptr;
    std::shared_ptr<GstPlayerVideoRendererCtrl> rendererCtrl_ = nullptr;
    std::weak_ptr<IPlayerEngineObs> obs_;
    sptr<Surface> producerSurface_ = nullptr;
    std::string url_ = "";
    std::condition_variable condVarSync_;
    bool gstPlayerInit_ = false;
    std::unique_ptr<std::thread> playerThread_;
    std::shared_ptr<GstAppsrcWarp> appsrcWarp_ = nullptr;
};
} // namespace Media
} // namespace OHOS
#endif // PLAYER_ENGINE_GST_IMPL
