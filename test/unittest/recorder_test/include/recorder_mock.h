/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef RECORDER_MOCK_H
#define RECORDER_MOCK_H

#include <atomic>
#include <thread>
#include <string>
#include "gtest/gtest.h"
#include "media_errors.h"
#include "test_params_config.h"
#include "unittest_log.h"
#include "recorder.h"
#include "surface.h"
#include "securec.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class RecorderMock {
public:
    RecorderMock() = default;
    ~RecorderMock() = default;
    bool CreateRecorder();
    int32_t SetVideoSource(VideoSourceType source, int32_t &sourceId);
    int32_t SetAudioSource(AudioSourceType source, int32_t &sourceId);
    int32_t SetDataSource(DataSourceType dataType, int32_t &sourceId);
    int32_t SetOutputFormat(OutputFormatType format);
    int32_t SetVideoEncoder(int32_t sourceId, VideoCodecFormat encoder);
    int32_t SetVideoSize(int32_t sourceId, int32_t width, int32_t height);
    int32_t SetVideoFrameRate(int32_t sourceId, int32_t frameRate);
    int32_t SetVideoEncodingBitRate(int32_t sourceId, int32_t rate);
    int32_t SetCaptureRate(int32_t sourceId, double fps);
    OHOS::sptr<OHOS::Surface> GetSurface(int32_t sourceId);
    int32_t SetAudioEncoder(int32_t sourceId, AudioCodecFormat encoder);
    int32_t SetAudioSampleRate(int32_t sourceId, int32_t rate);
    int32_t SetAudioChannels(int32_t sourceId, int32_t num);
    int32_t SetAudioEncodingBitRate(int32_t sourceId, int32_t bitRate);
    int32_t SetMaxDuration(int32_t duration);
    int32_t SetMaxFileSize(int64_t size);
    int32_t SetOutputFile(int32_t fd);
    int32_t SetNextOutputFile(int32_t fd);
    void SetLocation(float latitude, float longitude);
    void SetOrientationHint(int32_t rotation);
    int32_t SetRecorderCallback(const std::shared_ptr<RecorderCallback> &callback);
    int32_t Prepare();
    int32_t Start();
    int32_t Pause();
    int32_t Resume();
    int32_t Stop(bool block);
    int32_t Reset();
    int32_t Release();
    int32_t SetFileSplitDuration(FileSplitType type, int64_t timestamp, uint32_t duration);
    int32_t SetParameter(int32_t sourceId, const Format &format);
    int32_t RequesetBuffer(const std::string & recorderType, RecorderTestParam::VideoRecorderConfig &recorderConfig);
    void StopBuffer(const std::string &recorderType);
    void HDICreateESBuffer();
    void HDICreateYUVBuffer();
    int32_t CameraServicesForVideo(RecorderTestParam::VideoRecorderConfig &recorderConfig) const;
    int32_t CameraServicesForAudio(RecorderTestParam::VideoRecorderConfig &recorderConfig) const;
    int32_t SetFormat(const std::string &type, RecorderTestParam::VideoRecorderConfig &recorderConfig) const;
    int32_t GetStubFile();
    void GetFileFd();
    uint64_t GetPts();
private:
    std::shared_ptr<Recorder> recorder_ = nullptr;
    OHOS::sptr<OHOS::Surface> producerSurface_ = nullptr;
    std::shared_ptr<std::ifstream> file_ = nullptr;
    std::unique_ptr<std::thread> camereHDIThread_;
    std::atomic<bool> isExit_ { false };
    std::atomic<bool> isStart_ { true };
    int64_t pts_ = 0;
    int32_t isKeyFrame_ = 1;
    uint32_t count_ = 0;
    unsigned char color_ = 0xFF;
};

class RecorderCallbackTest : public RecorderCallback, public NoCopyable {
public:
    RecorderCallbackTest() = default;
    virtual ~RecorderCallbackTest() = default;

    void OnError(RecorderErrorType errorType, int32_t errorCode) override;
    void OnInfo(int32_t type, int32_t extra) override;
    int32_t GetErrorCode();
private:
    int32_t errorCode_ = 0;
};
}
}
#endif
