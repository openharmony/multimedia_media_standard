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
#include "i_playbin_ctrler.h"
#include "gst_appsrc_wrap.h"
#include "player_track_parse.h"

namespace OHOS {
namespace Media {
class CodecChangedDetector {
public:
    CodecChangedDetector() = default;
    ~CodecChangedDetector() = default;
    void DetectCodecSetup(const std::string &metaStr, GstElement *src, GstElement *videoSink);
    void DetectCodecUnSetup(GstElement *src, GstElement *videoSink);

private:
    void SetupCodecCb(const std::string &metaStr, GstElement *src, GstElement *videoSink);

    bool isHardwareDec_ = false;
    std::list<bool> codecTypeList_;
};

class PlayerEngineGstImpl : public IPlayerEngine, public NoCopyable {
public:
    explicit PlayerEngineGstImpl(int32_t uid = 0, int32_t pid = 0);
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
    int32_t SelectBitRate(uint32_t bitRate) override;
    int32_t SetVideoScaleType(VideoScaleType videoScaleType) override;
    int32_t SetAudioRendererInfo(const int32_t contentType, const int32_t streamUsage,
        const int32_t rendererFlag) override;
    int32_t SetAudioInterruptMode(const int32_t interruptMode) override;

private:
    void OnNotifyMessage(const PlayBinMessage &msg);
    void OnNotifyElemSetup(GstElement &elem);
    void OnNotifyElemUnSetup(GstElement &elem);
    double ChangeModeToSpeed(const PlaybackRateMode &mode) const;
    PlaybackRateMode ChangeSpeedToMode(double rate) const;
    int32_t PlayBinCtrlerInit();
    int32_t PlayBinCtrlerPrepare();
    void PlayBinCtrlerDeInit();
    int32_t GetRealPath(const std::string &url, std::string &realUrlPath) const;
    bool IsFileUrl(const std::string &url) const;
    void HandleErrorMessage(const PlayBinMessage &msg);
    void HandleInfoMessage(const PlayBinMessage &msg);
    void HandleSeekDoneMessage(const PlayBinMessage &msg);
    void HandleSubTypeMessage(const PlayBinMessage &msg);
    void HandleBufferingStart();
    void HandleBufferingEnd();
    void HandleBufferingTime(const PlayBinMessage &msg);
    void HandleBufferingPercent(const PlayBinMessage &msg);
    void HandleBufferingUsedMqNum(const PlayBinMessage &msg);
    void HandleVideoRenderingStart();
    void HandleVideoSizeChanged(const PlayBinMessage &msg);
    void HandleBitRateCollect(const PlayBinMessage &msg);
    void HandleVolumeChangedMessage(const PlayBinMessage &msg);
    void HandleInterruptMessage(const PlayBinMessage &msg);
    void HandlePositionUpdateMessage(const PlayBinMessage &msg);

    std::mutex mutex_;
    std::mutex trackParseMutex_;
    std::shared_ptr<IPlayBinCtrler> playBinCtrler_ = nullptr;
    std::shared_ptr<PlayBinSinkProvider> sinkProvider_;
    std::weak_ptr<IPlayerEngineObs> obs_;
    sptr<Surface> producerSurface_ = nullptr;
    std::string url_ = "";
    std::shared_ptr<GstAppsrcWrap> appsrcWrap_ = nullptr;
    std::shared_ptr<PlayerTrackParse> trackParse_ = nullptr;
    std::shared_ptr<CodecChangedDetector> codecChangedDetector_ = nullptr;
    int32_t videoWidth_ = 0;
    int32_t videoHeight_ = 0;
    int32_t percent_ = 0;
    uint32_t mqNumUsedBuffering_ = 0;
    uint64_t bufferingTime_ = 0;
    int32_t appuid_ = 0;
    int32_t apppid_ = 0;
    std::map<uint32_t, uint64_t> mqBufferingTime_;
    std::unordered_map<GstElement *, gulong> signalIds_;
    VideoScaleType videoScaleType_ = VIDEO_SCALE_TYPE_FIT;
    int32_t contentType_ = 0;
    int32_t streamUsage_ = 0;
    int32_t rendererFlag_ = 0;
};
} // namespace Media
} // namespace OHOS
#endif // PLAYER_ENGINE_GST_IMPL
