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

#ifndef RECORDER_SERVICE_STUB_H
#define RECORDER_SERVICE_STUB_H

#include <map>
#include "i_standard_recorder_service.h"
#include "i_standard_recorder_listener.h"
#include "media_death_recipient.h"
#include "recorder_server.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class RecorderServiceStub : public IRemoteStub<IStandardRecorderService> {
public:
    static sptr<RecorderServiceStub> Create();
    virtual ~RecorderServiceStub();
    DISALLOW_COPY_AND_MOVE(RecorderServiceStub);
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

    using RecorderStubFunc = int32_t(RecorderServiceStub::*)(MessageParcel &data, MessageParcel &reply);
    int32_t SetListenerObject(const sptr<IRemoteObject> &object) override;
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
    int32_t SetLocation(float latitude, float longitude) override;
    int32_t SetOrientationHint(int32_t rotation) override;
    int32_t Prepare() override;
    int32_t Start() override;
    int32_t Pause() override;
    int32_t Resume() override;
    int32_t Stop(bool block) override;
    int32_t Reset() override;
    int32_t Release() override;
    int32_t SetFileSplitDuration(FileSplitType type, int64_t timestamp, uint32_t duration) override;
    int32_t DestroyStub() override;

private:
    RecorderServiceStub();
    int32_t Init();
    int32_t SetListenerObject(MessageParcel &data, MessageParcel &reply);
    int32_t SetVideoSource(MessageParcel &data, MessageParcel &reply);
    int32_t SetVideoEncoder(MessageParcel &data, MessageParcel &reply);
    int32_t SetVideoSize(MessageParcel &data, MessageParcel &reply);
    int32_t SetVideoFrameRate(MessageParcel &data, MessageParcel &reply);
    int32_t SetVideoEncodingBitRate(MessageParcel &data, MessageParcel &reply);
    int32_t SetCaptureRate(MessageParcel &data, MessageParcel &reply);
    int32_t GetSurface(MessageParcel &data, MessageParcel &reply);
    int32_t SetAudioSource(MessageParcel &data, MessageParcel &reply);
    int32_t SetAudioEncoder(MessageParcel &data, MessageParcel &reply);
    int32_t SetAudioSampleRate(MessageParcel &data, MessageParcel &reply);
    int32_t SetAudioChannels(MessageParcel &data, MessageParcel &reply);
    int32_t SetAudioEncodingBitRate(MessageParcel &data, MessageParcel &reply);
    int32_t SetDataSource(MessageParcel &data, MessageParcel &reply);
    int32_t SetMaxDuration(MessageParcel &data, MessageParcel &reply);
    int32_t SetOutputFormat(MessageParcel &data, MessageParcel &reply);
    int32_t SetOutputPath(MessageParcel &data, MessageParcel &reply);
    int32_t SetOutputFile(MessageParcel &data, MessageParcel &reply);
    int32_t SetNextOutputFile(MessageParcel &data, MessageParcel &reply);
    int32_t SetMaxFileSize(MessageParcel &data, MessageParcel &reply);
    int32_t SetLocation(MessageParcel &data, MessageParcel &reply);
    int32_t SetOrientationHint(MessageParcel &data, MessageParcel &reply);
    int32_t Prepare(MessageParcel &data, MessageParcel &reply);
    int32_t Start(MessageParcel &data, MessageParcel &reply);
    int32_t Pause(MessageParcel &data, MessageParcel &reply);
    int32_t Resume(MessageParcel &data, MessageParcel &reply);
    int32_t Stop(MessageParcel &data, MessageParcel &reply);
    int32_t Reset(MessageParcel &data, MessageParcel &reply);
    int32_t Release(MessageParcel &data, MessageParcel &reply);
    int32_t SetFileSplitDuration(MessageParcel &data, MessageParcel &reply);
    int32_t DestroyStub(MessageParcel &data, MessageParcel &reply);

    std::shared_ptr<IRecorderService> recorderServer_ = nullptr;
    std::map<uint32_t, RecorderStubFunc> recFuncs_;
    std::mutex mutex_;
};
}
} // namespace OHOS
#endif // RECORDER_SERVICE_STUB_H
