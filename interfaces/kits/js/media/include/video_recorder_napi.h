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

#ifndef VIDEO_RECORDER_NAPI_H_
#define VIDEO_RECORDER_NAPI_H_

#include "recorder.h"
#include "av_common.h"
#include "media_errors.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "common_napi.h"
#include "recorder_napi_utils.h"

namespace OHOS {
namespace Media {
namespace VideoRecorderState {
const std::string STATE_IDLE = "idle";
const std::string STATE_PREPARED = "prepared";
const std::string STATE_PLAYING = "playing";
const std::string STATE_PAUSED = "paused";
const std::string STATE_STOPPED = "stopped";
const std::string STATE_ERROR = "error";
};

struct VideoRecorderAsyncContext;

constexpr int32_t DEFAULT_AUDIO_BIT_RATE = 48000;
constexpr int32_t DEFAULT_AUDIO_CHANNELS = 2;
constexpr int32_t DEFAULT_AUDIO_SAMPLE_RATE = 48000;
constexpr int32_t DEFAULT_DURATION = 5;
constexpr int32_t DEFAULT_VIDEO_BIT_RATE = 48000;
constexpr int32_t DEFAULT_FRAME_HEIGHT = -1;
constexpr int32_t DEFAULT_FRAME_WIDTH = -1;
constexpr int32_t DEFAULT_FRAME_RATE = 30;

class VideoRecorderNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);

private:
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static void Destructor(napi_env env, void *nativeObject, void *finalize);
    static napi_value CreateVideoRecorder(napi_env env, napi_callback_info info);
    static napi_value Prepare(napi_env env, napi_callback_info info);
    static napi_value GetInputSurface(napi_env env, napi_callback_info info);
    static napi_value Start(napi_env env, napi_callback_info info);
    static napi_value Pause(napi_env env, napi_callback_info info);
    static napi_value Resume(napi_env env, napi_callback_info info);
    static napi_value Stop(napi_env env, napi_callback_info info);
    static napi_value Release(napi_env env, napi_callback_info info);
    static napi_value Reset(napi_env env, napi_callback_info info);
    static napi_value On(napi_env env, napi_callback_info info);
    static napi_value GetState(napi_env env, napi_callback_info info);
    void ErrorCallback(MediaServiceExtErrCode errCode);
    VideoRecorderNapi();
    ~VideoRecorderNapi();

    struct VideoRecorderProfile {
        int32_t audioBitrate = DEFAULT_AUDIO_BIT_RATE;
        int32_t audioChannels = DEFAULT_AUDIO_CHANNELS;
        AudioCodecFormat audioCodecFormat = AudioCodecFormat::AUDIO_DEFAULT;
        int32_t auidoSampleRate = DEFAULT_AUDIO_SAMPLE_RATE;
        int32_t duration = DEFAULT_DURATION;
        OutputFormatType outputFormat = OutputFormatType::FORMAT_DEFAULT;
        int32_t videoBitrate = DEFAULT_VIDEO_BIT_RATE;
        VideoCodecFormat videoCodecFormat = VideoCodecFormat::VIDEO_DEFAULT;
        int32_t videoFrameWidth = DEFAULT_FRAME_HEIGHT;
        int32_t videoFrameHeight = DEFAULT_FRAME_WIDTH;
        int32_t videoFrameRate = DEFAULT_FRAME_RATE;
    };

    struct VideoRecorderProperties {
        AudioSourceType audioSourceType; // source type;
        VideoSourceType videoSourceType;
        VideoRecorderProfile profile;
        int32_t orientationHint = 0; // Optional
        Location location; // Optional
        std::string url;
    };

    int32_t GetVideoRecorderProperties(napi_env env, napi_value args, VideoRecorderProperties &properties);
    int32_t SetVideoRecorderProperties(std::unique_ptr<VideoRecorderAsyncContext> &ctx,
        const VideoRecorderProperties &properties);
    void GetConfig(napi_env env, napi_value args, std::unique_ptr<VideoRecorderAsyncContext> &ctx,
        VideoRecorderProperties &properties);
    int32_t SetUrl(const std::string &UrlPath);
    bool isSurfaceIdVaild(uint64_t surfaceID);

    static napi_ref constructor_;
    napi_env env_ = nullptr;
    napi_ref wrapper_ = nullptr;
    std::shared_ptr<Recorder> recorder_ = nullptr;
    std::shared_ptr<RecorderCallback> callbackNapi_ = nullptr;
    sptr<Surface> surface_;
    std::string currentStates_ = VideoRecorderState::STATE_IDLE;
    bool isPureVideo = false;
    int32_t videoSourceID;
    int32_t audioSourceID;
};

struct VideoRecorderAsyncContext : public MediaAsyncContext {
    explicit VideoRecorderAsyncContext(napi_env env) : MediaAsyncContext(env) {}
    ~VideoRecorderAsyncContext() = default;

    VideoRecorderNapi *napi = nullptr;
};
} // namespace Media
} // namespace OHOS
#endif /* VIDERECORDER_NAPI_H_ */
