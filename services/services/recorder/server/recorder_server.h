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

#ifndef RECORDER_SERVICE_SERVER_H
#define RECORDER_SERVICE_SERVER_H

#include "i_recorder_service.h"
#include "i_recorder_engine.h"
#include "time_monitor.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class RecorderServer : public IRecorderService, public IRecorderEngineObs {
public:
    static std::shared_ptr<IRecorderService> Create();
    RecorderServer();
    ~RecorderServer();
    DISALLOW_COPY_AND_MOVE(RecorderServer);

    enum RecStatus {
        REC_INITIALIZED = 0,
        REC_CONFIGURED,
        REC_PREPARED,
        REC_RECORDING,
        REC_PAUSED,
        REC_ERROR,
    };

    // IRecorderService override
    int32_t SetVideoSource(VideoSourceType source, int32_t &sourceId) override;
    int32_t SetVideoEncoder(int32_t sourceId, VideoCodecFormat encoder) override;
    int32_t SetVideoSize(int32_t sourceId, int32_t width, int32_t height) override;
    int32_t SetVideoFrameRate(int32_t sourceId, int32_t frameRate) override;
    int32_t SetVideoEncodingBitRate(int32_t sourceId, int32_t rate) override;
    int32_t SetCaptureRate(int32_t sourceId, double fps) override;
    sptr<OHOS::Surface> GetSurface(int32_t sourceId) override;
    int32_t SetAudioSource(AudioSourceType source, int32_t &sourceId) override;
    int32_t SetAudioEncoder(int32_t sourceId, AudioCodecFormat encoder) override;
    int32_t SetAudioSampleRate(int32_t sourceId, int32_t rate) override;
    int32_t SetAudioChannels(int32_t sourceId, int32_t num) override;
    int32_t SetAudioEncodingBitRate(int32_t sourceId, int32_t bitRate) override;
    int32_t SetDataSource(DataSourceType dataType, int32_t &sourceId) override;
    int32_t SetMaxDuration(int32_t duration) override;
    int32_t SetOutputFormat(OutputFormatType format) override;
    int32_t SetOutputPath(const std::string &path) override;
    int32_t SetOutputFile(int32_t fd) override;
    int32_t SetNextOutputFile(int32_t fd) override;
    int32_t SetMaxFileSize(int64_t size) override;
    int32_t SetRecorderCallback(const std::shared_ptr<RecorderCallback> &callback) override;
    int32_t Prepare() override;
    int32_t Start() override;
    int32_t Pause() override;
    int32_t Resume() override;
    int32_t Stop(bool block) override;
    int32_t Reset() override;
    int32_t Release() override;
    int32_t SetFileSplitDuration(FileSplitType type, int64_t timestamp, uint32_t duration) override;
    int32_t SetParameter(int32_t sourceId, const Format &format) override;

    // IRecorderEngineObs override
    void OnError(ErrorType errorType, int32_t errorCode) override;
    void OnInfo(InfoType type, int32_t extra) override;

private:
    int32_t Init();

private:
    std::unique_ptr<IRecorderEngine> recorderEngine_ = nullptr;
    std::shared_ptr<RecorderCallback> recorderCb_ = nullptr;
    RecStatus status_ = REC_INITIALIZED;
    std::mutex mutex_;
    std::mutex cbMutex_;
    TimeMonitor startTimeMonitor_;
    TimeMonitor stopTimeMonitor_;
};
} // namespace Media
} // namespace OHOS
#endif // RECORDER_SERVICE_SERVER_H
