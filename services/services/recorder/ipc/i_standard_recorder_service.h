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

#ifndef I_STANDARD_RECORDER_SERVICE_H
#define I_STANDARD_RECORDER_SERVICE_H

#include "ipc_types.h"
#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"
#include "recorder.h"

namespace OHOS {
namespace Media {
class IStandardRecorderService : public IRemoteBroker {
public:
    virtual ~IStandardRecorderService() = default;
    virtual int32_t SetListenerObject(const sptr<IRemoteObject> &object) = 0;
    virtual int32_t SetVideoSource(VideoSourceType source, int32_t &sourceId) = 0;
    virtual int32_t SetVideoEncoder(int32_t sourceId, VideoCodecFormat encoder) = 0;
    virtual int32_t SetVideoSize(int32_t sourceId, int32_t width, int32_t height) = 0;
    virtual int32_t SetVideoFrameRate(int32_t sourceId, int32_t frameRate) = 0;
    virtual int32_t SetVideoEncodingBitRate(int32_t sourceId, int32_t rate) = 0;
    virtual int32_t SetCaptureRate(int32_t sourceId, double fps) = 0;
    virtual sptr<OHOS::Surface> GetSurface(int32_t sourceId) = 0;
    virtual int32_t SetAudioSource(AudioSourceType source, int32_t &sourceId) = 0;
    virtual int32_t SetAudioEncoder(int32_t sourceId, AudioCodecFormat encoder) = 0;
    virtual int32_t SetAudioSampleRate(int32_t sourceId, int32_t rate) = 0;
    virtual int32_t SetAudioChannels(int32_t sourceId, int32_t num) = 0;
    virtual int32_t SetAudioEncodingBitRate(int32_t sourceId, int32_t bitRate) = 0;
    virtual int32_t SetDataSource(DataSourceType dataType, int32_t &sourceId) = 0;
    virtual int32_t SetMaxDuration(int32_t duration) = 0;
    virtual int32_t SetOutputFormat(OutputFormatType format) = 0;
    virtual int32_t SetOutputPath(const std::string &path) = 0;
    virtual int32_t SetOutputFile(int32_t fd) = 0;
    virtual int32_t SetNextOutputFile(int32_t fd) = 0;
    virtual int32_t SetMaxFileSize(int64_t size) = 0;
    virtual int32_t Prepare() = 0;
    virtual int32_t Start() = 0;
    virtual int32_t Pause() = 0;
    virtual int32_t Resume() = 0;
    virtual int32_t Stop(bool block) = 0;
    virtual int32_t Reset() = 0;
    virtual int32_t Release() = 0;
    virtual int32_t SetFileSplitDuration(FileSplitType type, int64_t timestamp, uint32_t duration) = 0;
    virtual int32_t DestroyStub() = 0;
    /**
     * IPC code ID
     */
    enum RecorderServiceMsg {
        SET_LISTENER_OBJ = 0,
        SET_VIDEO_SOURCE,
        SET_VIDEO_ENCODER,
        SET_VIDEO_SIZE,
        SET_VIDEO_FARAME_RATE,
        SET_VIDEO_ENCODING_BIT_RATE,
        SET_CAPTURE_RATE,
        GET_SURFACE,
        SET_AUDIO_SOURCE,
        SET_AUDIO_ENCODER,
        SET_AUDIO_SAMPLE_RATE,
        SET_AUDIO_CHANNELS,
        SET_AUDIO_ENCODING_BIT_RATE,
        SET_DATA_SOURCE,
        SET_MAX_DURATION,
        SET_OUTPUT_FORMAT,
        SET_OUTPUT_PATH,
        SET_OUTPUT_FILE,
        SET_NEXT_OUTPUT_FILE,
        SET_MAX_FILE_SIZE,
        PREPARE,
        START,
        PAUSE,
        RESUME,
        STOP,
        RESET,
        RELEASE,
        SET_FILE_SPLIT_DURATION,
        DESTROY,
    };

    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardRecorderService");
};
} // OHOS
} // namespace OHOS
#endif // I_STANDARD_RECORDER_SERVICE_H
