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

#include "recorder_mock.h"
#include <sync_fence.h>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include "display_type.h"

using namespace std;
using namespace OHOS::Media;
using namespace testing::ext;
using namespace OHOS::Media::RecorderTestParam;

// config for surface buffer flush to the queue
static OHOS::BufferFlushConfig g_esFlushConfig = {
    .damage = {
        .x = 0,
        .y = 0,
        .w = CODEC_BUFFER_WIDTH,
        .h = CODEC_BUFFER_HEIGHT
    },
    .timestamp = 0
};

// config for surface buffer request from the queue
static OHOS::BufferRequestConfig g_esRequestConfig = {
    .width = CODEC_BUFFER_WIDTH,
    .height = CODEC_BUFFER_HEIGHT,
    .strideAlignment = STRIDE_ALIGN,
    .format = PIXEL_FMT_RGBA_8888,
    .usage = HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA,
    .timeout = 0
};

// config for surface buffer flush to the queue
static OHOS::BufferFlushConfig g_yuvFlushConfig = {
    .damage = {
        .x = 0,
        .y = 0,
        .w = YUV_BUFFER_WIDTH,
        .h = YUV_BUFFER_HEIGHT
    },
    .timestamp = 0
};

// config for surface buffer request from the queue
static OHOS::BufferRequestConfig g_yuvRequestConfig = {
    .width = YUV_BUFFER_WIDTH,
    .height = YUV_BUFFER_HEIGHT,
    .strideAlignment = STRIDE_ALIGN,
    .format = PIXEL_FMT_YCRCB_420_SP,
    .usage = HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA,
    .timeout = 0
};
void RecorderCallbackTest::OnError(RecorderErrorType errorType, int32_t errorCode)
{
    cout << "Error received, errorType:" << errorType << " errorCode:" << errorCode << endl;
}

void RecorderCallbackTest::OnInfo(int32_t type, int32_t extra)
{
    cout << "Info received, Infotype:" << type << " Infocode:" << extra << endl;
}

int32_t RecorderCallbackTest::GetErrorCode()
{
    return errorCode_;
};

bool RecorderMock::CreateRecorder()
{
    recorder_ = RecorderFactory::CreateRecorder();
    return recorder_ != nullptr;
}

int32_t RecorderMock::SetVideoSource(VideoSourceType source, int32_t &sourceId)
{
    return recorder_->SetVideoSource(source, sourceId);
}

int32_t RecorderMock::SetAudioSource(AudioSourceType source, int32_t &sourceId)
{
    return recorder_->SetAudioSource(source, sourceId);
}

int32_t RecorderMock::SetDataSource(DataSourceType dataType, int32_t &sourceId)
{
    return recorder_->SetDataSource(dataType, sourceId);
}

int32_t RecorderMock::SetOutputFormat(OutputFormatType format)
{
    return recorder_->SetOutputFormat(format);
}

int32_t RecorderMock::SetVideoEncoder(int32_t sourceId, VideoCodecFormat encoder)
{
    return recorder_->SetVideoEncoder(sourceId, encoder);
}

int32_t RecorderMock::SetVideoSize(int32_t sourceId, int32_t width, int32_t height)
{
    return recorder_->SetVideoSize(sourceId, width, height);
}

int32_t RecorderMock::SetVideoFrameRate(int32_t sourceId, int32_t frameRate)
{
    return recorder_->SetVideoFrameRate(sourceId, frameRate);
}

int32_t RecorderMock::SetVideoEncodingBitRate(int32_t sourceId, int32_t rate)
{
    return recorder_->SetVideoEncodingBitRate(sourceId, rate);
}

int32_t RecorderMock::SetCaptureRate(int32_t sourceId, double fps)
{
    return recorder_->SetCaptureRate(sourceId, fps);
}

OHOS::sptr<OHOS::Surface> RecorderMock::GetSurface(int32_t sourceId)
{
    return recorder_->GetSurface(sourceId);
}

int32_t RecorderMock::SetAudioEncoder(int32_t sourceId, AudioCodecFormat encoder)
{
    return recorder_->SetAudioEncoder(sourceId, encoder);
}

int32_t RecorderMock::SetAudioSampleRate(int32_t sourceId, int32_t rate)
{
    return recorder_->SetAudioSampleRate(sourceId, rate);
}

int32_t RecorderMock::SetAudioChannels(int32_t sourceId, int32_t num)
{
    return recorder_->SetAudioChannels(sourceId, num);
}

int32_t RecorderMock::SetAudioEncodingBitRate(int32_t sourceId, int32_t bitRate)
{
    return recorder_->SetAudioEncodingBitRate(sourceId, bitRate);
}

int32_t RecorderMock::SetMaxDuration(int32_t duration)
{
    return recorder_->SetMaxDuration(duration);
}

int32_t RecorderMock::SetMaxFileSize(int64_t size)
{
    return recorder_->SetMaxFileSize(size);
}

int32_t RecorderMock::SetOutputFile(int32_t fd)
{
    return recorder_->SetOutputFile(fd);
}

int32_t RecorderMock::SetNextOutputFile(int32_t fd)
{
    return recorder_->SetNextOutputFile(fd);
}

void RecorderMock::SetLocation(float latitude, float longitude)
{
    return recorder_->SetLocation(latitude, longitude);
}

void RecorderMock::SetOrientationHint(int32_t rotation)
{
    return recorder_->SetOrientationHint(rotation);
}

int32_t RecorderMock::SetRecorderCallback(const std::shared_ptr<RecorderCallback> &callback)
{
    return recorder_->SetRecorderCallback(callback);
}

int32_t RecorderMock::Prepare()
{
    return recorder_->Prepare();
}

int32_t RecorderMock::Start()
{
    return recorder_->Start();
}

int32_t RecorderMock::Pause()
{
    if (isStart_.load()) {
        isStart_.store(false);
    }
    return recorder_->Pause();
}

int32_t RecorderMock::Resume()
{
    if (!isStart_.load()) {
        isStart_.store(true);
    }
    return recorder_->Resume();
}

int32_t RecorderMock::Stop(bool block)
{
    if (!isExit_.load()) {
        isExit_.store(true);
    }
    return recorder_->Stop(block);
}

int32_t RecorderMock::Reset()
{
    if (!isExit_.load()) {
        isExit_.store(true);
    }
    return recorder_->Reset();
}

int32_t RecorderMock::Release()
{
    if (!isExit_.load()) {
        isExit_.store(true);
    }
    return recorder_->Release();
}

int32_t RecorderMock::SetFileSplitDuration(FileSplitType type, int64_t timestamp, uint32_t duration)
{
    return recorder_->SetFileSplitDuration(type, timestamp, duration);
}

int32_t RecorderMock::SetParameter(int32_t sourceId, const Format &format)
{
    return recorder_->SetParameter(sourceId, format);
}

int32_t RecorderMock::RequesetBuffer(const std::string &recorderType, VideoRecorderConfig &recorderConfig)
{
    if (recorderType != PURE_AUDIO) {
        producerSurface_ = recorder_->GetSurface(recorderConfig.videoSourceId);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(producerSurface_ != nullptr, MSERR_INVALID_OPERATION, "GetSurface failed ");

        if (recorderConfig.vSource == VIDEO_SOURCE_SURFACE_ES) {
            cout << "es source stream, get from file" << endl;
            int32_t ret = GetStubFile();
            UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "GetStubFile failed ");
            camereHDIThread_.reset(new(std::nothrow) std::thread(&RecorderMock::HDICreateESBuffer, this));
        } else {
            camereHDIThread_.reset(new(std::nothrow) std::thread(&RecorderMock::HDICreateYUVBuffer, this));
        }
    }
    return MSERR_OK;
}

void RecorderMock::StopBuffer(const std::string &recorderType)
{
    if (recorderType != PURE_AUDIO && camereHDIThread_ != nullptr) {
        camereHDIThread_->join();
    }
}

int32_t RecorderMock::GetStubFile()
{
    file_ = std::make_shared<std::ifstream>();
    if (file_ == nullptr) {
        return -1;
    }
    UNITTEST_CHECK_AND_RETURN_RET_LOG(file_ != nullptr, MSERR_INVALID_OPERATION, "create file failed");
    const std::string filePath = "/data/test/h264_1280_720.h264";
    file_->open(filePath, std::ios::in | std::ios::binary);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(file_->is_open(), MSERR_INVALID_OPERATION, "open file failed");
    if (!(file_->is_open())) {
        return -1;
    }
    return MSERR_OK;
}

uint64_t RecorderMock::GetPts()
{
    struct timespec timestamp = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &timestamp);
    uint64_t time = (uint64_t)timestamp.tv_sec * SEC_TO_NS + (uint64_t)timestamp.tv_nsec;
    return time;
}

void RecorderMock::HDICreateESBuffer()
{
    // camera hdi loop to requeset buffer
    const uint32_t *frameLenArray = HIGH_VIDEO_FRAME_SIZE;
    while (count_ < STUB_STREAM_SIZE) {
        UNITTEST_CHECK_AND_BREAK_LOG(!isExit_.load(), "close camera hdi thread");
        usleep(FRAME_RATE);
        OHOS::sptr<OHOS::SurfaceBuffer> buffer;
        int32_t releaseFence;
        OHOS::SurfaceError ret = producerSurface_->RequestBuffer(buffer, releaseFence, g_esRequestConfig);
        UNITTEST_CHECK_AND_CONTINUE_LOG(ret != OHOS::SURFACE_ERROR_NO_BUFFER, "surface loop full, no buffer now");
        UNITTEST_CHECK_AND_BREAK_LOG(ret == SURFACE_ERROR_OK && buffer != nullptr, "RequestBuffer failed");

        sptr<SyncFence> tempFence = new SyncFence(releaseFence);
        tempFence->Wait(100); // 100ms

        auto addr = static_cast<uint8_t *>(buffer->GetVirAddr());
        if (addr == nullptr) {
            cout << "GetVirAddr failed" << endl;
            (void)producerSurface_->CancelBuffer(buffer);
            break;
        }
        char *tempBuffer = static_cast<char *>(malloc(sizeof(char) * (*frameLenArray) + 1));
        if (tempBuffer == nullptr) {
            (void)producerSurface_->CancelBuffer(buffer);
            break;
        }
        (void)file_->read(tempBuffer, *frameLenArray);
        if (*frameLenArray > buffer->GetSize()) {
            free(tempBuffer);
            (void)producerSurface_->CancelBuffer(buffer);
            break;
        }
        (void)memcpy_s(addr, *frameLenArray, tempBuffer, *frameLenArray);

        if (isStart_.load()) {
            pts_= (int64_t)(GetPts());
            isStart_.store(false);
        }

        (void)buffer->GetExtraData()->ExtraSet("dataSize", static_cast<int32_t>(*frameLenArray));
        (void)buffer->GetExtraData()->ExtraSet("timeStamp", pts_);
        (void)buffer->GetExtraData()->ExtraSet("isKeyFrame", isKeyFrame_);
        count_++;
        (count_ % 30) == 0 ? (isKeyFrame_ = 1) : (isKeyFrame_ = 0); // keyframe every 30fps
        pts_ += FRAME_DURATION;
        (void)producerSurface_->FlushBuffer(buffer, -1, g_esFlushConfig);
        frameLenArray++;
        free(tempBuffer);
    }
    cout << "exit camera hdi loop" << endl;
    if ((file_ != nullptr) && (file_->is_open())) {
        file_->close();
    }
}


void RecorderMock::HDICreateYUVBuffer()
{
    // camera hdi loop to requeset buffer
    while (count_ < STUB_STREAM_SIZE) {
        UNITTEST_CHECK_AND_BREAK_LOG(!isExit_.load(), "close camera hdi thread");
        usleep(FRAME_RATE);
        OHOS::sptr<OHOS::SurfaceBuffer> buffer;
        int32_t releaseFence;
        OHOS::SurfaceError ret = producerSurface_->RequestBuffer(buffer, releaseFence, g_yuvRequestConfig);
        UNITTEST_CHECK_AND_CONTINUE_LOG(ret != OHOS::SURFACE_ERROR_NO_BUFFER, "surface loop full, no buffer now");
        UNITTEST_CHECK_AND_BREAK_LOG(ret == SURFACE_ERROR_OK && buffer != nullptr, "RequestBuffer failed");

        sptr<SyncFence> tempFence = new SyncFence(releaseFence);
        tempFence->Wait(100); // 100ms

        char *tempBuffer = (char *)(buffer->GetVirAddr());
        (void)memset_s(tempBuffer, YUV_BUFFER_SIZE, color_, YUV_BUFFER_SIZE);

        (void)srand((int)time(0));
        for (uint32_t i = 0; i < YUV_BUFFER_SIZE - 1; i += (YUV_BUFFER_SIZE - 1)) {  // 100 is the steps between noise
            if (i >= YUV_BUFFER_SIZE - 1) {
                break;
            }
            tempBuffer[i] = (unsigned char)(rand() % 255); // 255 is the size of yuv, add noise
        }

        color_ = color_ - 3; // 3 is the step of the pic change

        if (color_ <= 0) {
            color_ = 0xFF;
        }

        // get time
        pts_= (int64_t)(GetPts());
        (void)buffer->GetExtraData()->ExtraSet("dataSize", static_cast<int32_t>(YUV_BUFFER_SIZE));
        (void)buffer->GetExtraData()->ExtraSet("timeStamp", pts_);
        (void)buffer->GetExtraData()->ExtraSet("isKeyFrame", isKeyFrame_);
        count_++;
        (count_ % 30) == 0 ? (isKeyFrame_ = 1) : (isKeyFrame_ = 0); // keyframe every 30fps
        (void)producerSurface_->FlushBuffer(buffer, -1, g_yuvFlushConfig);
    }
    cout << "exit camera hdi loop" << endl;
}

int32_t RecorderMock::CameraServicesForVideo(VideoRecorderConfig &recorderConfig) const
{
    int32_t ret = recorder_->SetVideoEncoder(recorderConfig.videoSourceId,
        recorderConfig.videoFormat);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetVideoEncoder failed ");

    ret = recorder_->SetVideoSize(recorderConfig.videoSourceId,
        recorderConfig.width, recorderConfig.height);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetVideoSize failed ");

    ret = recorder_->SetVideoFrameRate(recorderConfig.videoSourceId, recorderConfig.frameRate);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetVideoFrameRate failed ");

    ret = recorder_->SetVideoEncodingBitRate(recorderConfig.videoSourceId,
        recorderConfig.videoEncodingBitRate);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetVideoEncodingBitRate failed ");
    return MSERR_OK;
}

int32_t RecorderMock::CameraServicesForAudio(VideoRecorderConfig &recorderConfig) const
{
    int32_t ret = recorder_->SetAudioEncoder(recorderConfig.audioSourceId, recorderConfig.audioFormat);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetAudioEncoder failed ");

    ret = recorder_->SetAudioSampleRate(recorderConfig.audioSourceId, recorderConfig.sampleRate);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetAudioSampleRate failed ");

    ret = recorder_->SetAudioChannels(recorderConfig.audioSourceId, recorderConfig.channelCount);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetAudioChannels failed ");

    ret = recorder_->SetAudioEncodingBitRate(recorderConfig.audioSourceId,
        recorderConfig.audioEncodingBitRate);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetAudioEncodingBitRate failed ");

    return MSERR_OK;
}

int32_t RecorderMock::SetFormat(const std::string &recorderType, VideoRecorderConfig &recorderConfig) const
{
    int32_t ret = 0;
    if (recorderType == PURE_VIDEO) {
        ret = recorder_->SetVideoSource(recorderConfig.vSource, recorderConfig.videoSourceId);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetVideoSource failed ");
        ret = recorder_->SetOutputFormat(recorderConfig.outPutFormat);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetOutputFormat failed ");
        ret = CameraServicesForVideo(recorderConfig);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "CameraServices failed ");
    } else if (recorderType == PURE_AUDIO) {
        ret = recorder_->SetAudioSource(recorderConfig.aSource, recorderConfig.audioSourceId);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetAudioSource failed ");
        ret = recorder_->SetOutputFormat(recorderConfig.outPutFormat);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetOutputFormat failed ");
        ret = CameraServicesForAudio(recorderConfig);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "CameraServicesForAudio failed ");
    } else if (recorderType == AUDIO_VIDEO) {
        ret = recorder_->SetVideoSource(recorderConfig.vSource, recorderConfig.videoSourceId);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetVideoSource failed ");
        ret = recorder_->SetAudioSource(recorderConfig.aSource, recorderConfig.audioSourceId);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetAudioSource failed ");
        ret = recorder_->SetOutputFormat(recorderConfig.outPutFormat);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetOutputFormat failed ");
        ret = CameraServicesForVideo(recorderConfig);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "CameraServicesForVideo failed ");
        ret = CameraServicesForAudio(recorderConfig);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "CameraServicesForAudio failed ");
    }

    ret = recorder_->SetMaxDuration(recorderConfig.duration);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetMaxDuration failed ");

    ret = recorder_->SetOutputFile(recorderConfig.outputFd);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetOutputFile failed ");

    std::shared_ptr<RecorderCallbackTest> cb = std::make_shared<RecorderCallbackTest>();
    ret += recorder_->SetRecorderCallback(cb);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetRecorderCallback failed ");
    cout << "set format finished" << endl;
    return ret;
}