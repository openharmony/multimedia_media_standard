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

#ifndef VIDEO_CALLBACK_NAPI_H_
#define VIDEO_CALLBACK_NAPI_H_

#include <queue>
#include "player_callback_napi.h"
#include "video_player_napi.h"

namespace OHOS {
namespace Media {
enum class AsyncWorkType : int32_t {
    ASYNC_WORK_PREPARE = 0,
    ASYNC_WORK_PLAY,
    ASYNC_WORK_PAUSE,
    ASYNC_WORK_STOP,
    ASYNC_WORK_RESET,
    ASYNC_WORK_SEEK,
    ASYNC_WORK_SPEED,
    ASYNC_WORK_VOLUME,
    ASYNC_WORK_INVALID,
};

struct VideoPlayerAsyncContext : public MediaAsyncContext {
    explicit VideoPlayerAsyncContext(napi_env env) : MediaAsyncContext(env) {}
    virtual ~VideoPlayerAsyncContext() = default;
    static void AsyncCallback(napi_env env, VideoPlayerAsyncContext *&asyncContext);
    VideoPlayerNapi *playerNapi = nullptr;
    VideoPlayerNapi *jsPlayer = nullptr;
    AsyncWorkType asyncWorkType = AsyncWorkType::ASYNC_WORK_INVALID;
    double volume = 1.0f; // default volume level
    int32_t seekPosition = 0;
    int32_t seekMode = SEEK_PREVIOUS_SYNC;
    int32_t speedMode = SPEED_FORWARD_1_00_X;
    std::string surface = "none";
};

class VideoCallbackNapi : public PlayerCallbackNapi {
public:
    explicit VideoCallbackNapi(napi_env env);
    virtual ~VideoCallbackNapi();

    void SaveCallbackReference(const std::string &callbackName, napi_value callback) override;
    void OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody) override;
    PlayerStates GetCurrentState() const override;
    int32_t GetVideoWidth() const { return width_; }
    int32_t GetVideoHeight() const { return height_; }
    void QueueAsyncWork(VideoPlayerAsyncContext *context);
    void ClearAsyncWork();

private:
    void OnStartRenderFrameCb() const;
    void OnPlaybackCompleteCb() const;
    void OnVideoSizeChangedCb(const Format &infoBody);
    void OnStateChangeCb(PlayerStates state);
    void OnSeekDoneCb(int32_t positon);
    void OnSpeedDoneCb(int32_t speedMode);
    void OnVolumeDoneCb();
    void DequeueAsyncWork();
    static void ContextCallback(napi_env env, napi_status status, void *data);

    std::mutex mutex_;
    napi_env env_ = nullptr;
    PlayerStates currentState_ = PLAYER_IDLE;
    std::queue<VideoPlayerAsyncContext *> contextStateQue_;
    std::queue<VideoPlayerAsyncContext *> contextSeekQue_;
    std::queue<VideoPlayerAsyncContext *> contextSpeedQue_;
    std::queue<VideoPlayerAsyncContext *> contextVolumeQue_;
    std::shared_ptr<AutoRef> startRenderFrameCallback_ = nullptr;
    std::shared_ptr<AutoRef> videoSizeChangedCallback_ = nullptr;
    std::shared_ptr<AutoRef> playbackCompletedCallback_ = nullptr;
    int32_t width_ = 0;
    int32_t height_ = 0;
};
}
}
#endif // VIDEO_CALLBACK_NAPI_H_