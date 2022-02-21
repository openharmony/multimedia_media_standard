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

#ifndef RECORDER_DEMO_H
#define RECORDER_DEMO_H

#include <atomic>
#include <thread>
#include <string>
#include "recorder.h"
#include "surface.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
const std::string SAVE_PATH = "/data/recorder";
struct VideoRecorderConfig {
    int32_t audioSourceId = 0;
    int32_t videoSourceId = 0;
    int32_t audioEncodingBitRate = 48000;
    int32_t channelCount = 2;
    int32_t duration = 60;
    int32_t width = 1280;
    int32_t height = 720;
    int32_t frameRate = 30;
    int32_t videoEncodingBitRate = 48000;
    int32_t sampleRate = 48000;
    double captureFps = 30;
    std::string outPath = SAVE_PATH;
    AudioCodecFormat audioFormat = AAC_LC;
    AudioSourceType aSource = AUDIO_MIC;
    OutputFormatType outPutFormat = FORMAT_MPEG_4;
    VideoSourceType vSource = VIDEO_SOURCE_SURFACE_ES;
    VideoCodecFormat videoFormat = H264;
};

struct AudioRecorderConfig {
    int32_t audioSourceId = 0;
    int32_t audioEncodingBitRate = 48000;
    int32_t channelCount = 2;
    int32_t duration = 60;
    std::string outPath = SAVE_PATH;
    int32_t sampleRate = 48000;
    AudioCodecFormat audioFormat = AAC_LC;
    AudioSourceType inputSource = AUDIO_MIC;
    OutputFormatType outPutFormat = FORMAT_M4A;
};

class RecorderDemo : public NoCopyable {
public:
    RecorderDemo() = default;
    virtual ~RecorderDemo() = default;

    void RunCase();
    void HDICreateBuffer();
    int32_t CameraServicesForVideo() const;
    int32_t CameraServicesForAudio() const;
    int32_t SetFormat(const std::string &type) const;
    int32_t GetStubFile();

private:
    int64_t pts_ = 0;
    int32_t isKeyFrame_ = 1;
    OHOS::sptr<OHOS::Surface> producerSurface_ = nullptr;
    std::shared_ptr<std::ifstream> file_ = nullptr;
    std::atomic<bool> isExit_{ false };
    std::shared_ptr<Recorder> recorder_ = nullptr;
    std::unique_ptr<std::thread> camereHDIThread_;
    uint32_t count_ = 0;
};

class RecorderCallbackDemo : public RecorderCallback, public NoCopyable {
public:
    RecorderCallbackDemo() = default;
    virtual ~RecorderCallbackDemo() = default;

    void OnError(RecorderErrorType errorType, int32_t errorCode) override;
    void OnInfo(int32_t type, int32_t extra) override;
};
} // Media
} // OHOS
#endif // RECORDER_DEMO_H
