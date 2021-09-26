/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License\n");
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

#include "recorder_test.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "display_type.h"
#include "securec.h"
#include "test_log.h"
#include "media_errors.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
namespace {
    constexpr uint32_t STUB_STREAM_SIZE = 451;
    constexpr uint32_t FRAME_RATE = 30000;
    constexpr uint32_t CODEC_BUFFER_WIDTH = 1024;
    constexpr uint32_t CODEC_BUFFER_HEIGHT = 25;
    constexpr uint32_t STRIDE_ALIGN = 8;
    constexpr uint32_t FRAME_DURATION = 40000000;
    constexpr uint32_t RECORDER_TIME = 5;
    const string PURE_VIDEO = "1";
    const string PURE_AUDIO = "2";
    const string AUDIO_VIDEO = "3";
}

void RecorderCallbackTest::OnError(RecorderErrorType errorType, int32_t errorCode)
{
    cout << "Error received, errorType:" << errorType << " errorCode:" << errorCode << endl;
}

void RecorderCallbackTest::OnInfo(int32_t type, int32_t extra)
{
    cout << "Info received, Infotype:" << type << " Infocode:" << extra << endl;
}

// config for video to request buffer from surface
static VideoRecorderConfig g_videoRecorderConfig;

// config for audio to request buffer from surface
static AudioRecorderConfig g_audioRecorderConfig;

// config for surface buffer flush to the queue
static OHOS::BufferFlushConfig g_flushConfig = {
    .damage = {
        .x = 0,
        .y = 0,
        .w = CODEC_BUFFER_WIDTH,
        .h = CODEC_BUFFER_HEIGHT
    },
    .timestamp = 0
};

// config for surface buffer request from the queue
static OHOS::BufferRequestConfig g_requestConfig = {
    .width = CODEC_BUFFER_WIDTH,
    .height = CODEC_BUFFER_HEIGHT,
    .strideAlignment = STRIDE_ALIGN,
    .format = PIXEL_FMT_RGBA_8888,
    .usage = HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA,
    .timeout = 0
};

// this array contains each buffer size of the stub stream
const uint32_t HIGH_VIDEO_FRAME_SIZE[STUB_STREAM_SIZE] = {
    12145, 6247, 9203, 4810, 7751, 3429, 2285, 2906, 4461, 4392, 4620, 4251, 3973, 3652, 4469, 5208, 4661,
    4712, 4139, 3199, 4299, 4429, 4726, 4612, 4428, 3709, 2715, 3006, 4048, 3939, 9648, 2796, 3900, 4119,
    3732, 3721, 3145, 2753, 3526, 3801, 4413, 4175, 4579, 4439, 3619, 3697, 4021, 3178, 3420, 3721, 3726,
    4671, 3823, 4565, 4667, 4040, 4174, 3884, 3398, 2933, 12095, 1126, 1514, 2308, 925, 3799, 2556, 1904,
    3721, 2380, 2152, 4190, 3415, 3248, 5801, 3772, 3070, 4871, 4584, 5653, 4888, 6578, 7284, 6345, 5787,
    4957, 5071, 4985, 4158, 3549, 15231, 2242, 1281, 2843, 1693, 3056, 3372, 4148, 3848, 5967, 3637, 3006,
    3347, 4323, 2136, 1865, 2548, 1899, 3360, 2289, 3669, 2452, 4872, 4113, 3268, 4847, 3861, 3157, 5288,
    4090, 20183, 5006, 2972, 4122, 3377, 3608, 3627, 3387, 3594, 3276, 5284, 4684, 4237, 3983, 3601, 3459,
    2489, 2609, 3443, 3135, 3489, 3464, 3888, 3893, 4359, 4363, 4263, 4480, 3545, 4654, 24011, 2565, 2118,
    4948, 2468, 2889, 2671, 4277, 2402, 3828, 3580, 3123, 4562, 3065, 5313, 2884, 3170, 2653, 3530, 3093,
    3002, 3199, 4546, 3232, 3347, 2991, 4202, 3994, 3740, 3771, 30196, 2683, 4039, 3324, 4274, 3866, 3910,
    3214, 3016, 3079, 4113, 2674, 4028, 3957, 4141, 3585, 5638, 4704, 3764, 3483, 3457, 3365, 3520, 3619,
    3664, 3490, 3979, 2686, 3735, 3601, 29065, 4288, 4371, 3420, 4585, 3705, 4230, 3887, 3310, 2815, 3030,
    2440, 2133, 2255, 2506, 2080, 2833, 2694, 3390, 3140, 3265, 3067, 3000, 3754, 3743, 3662, 3439, 3698,
    3420, 3340, 36130, 2170, 2589, 2361, 3101, 3011, 2942, 2832, 3550, 3614, 3620, 3443, 3363, 3518, 2684,
    2650, 2680, 3285, 3121, 3471, 3343, 3250, 3207, 3269, 3169, 3147, 3247, 3367, 3791, 3039, 43578,
    1813, 2615, 2529, 2501, 2491, 2466, 2436, 2966, 2805, 2777, 3260, 3270, 3416, 2819, 3154, 3273, 3334,
    3280, 3201, 3192, 3429, 3128, 3150, 3073, 3182, 3084, 3238, 3049, 3137, 49379, 2086, 2785, 2795, 2900,
    2768, 2770, 2794, 2780, 2872, 2896, 3538, 3473, 6350, 4433, 4241, 3684, 5019, 4919, 3985, 3135, 3637,
    3819, 3976, 4129, 4270, 2629, 3921, 3068, 3730, 33887, 3123, 4353, 4059, 5657, 5496, 6541, 5528, 5715,
    4582, 4379, 3810, 2871, 2204, 3205, 4453, 4865, 4691, 4724, 3618, 3361, 2881, 2864, 2067, 1682, 1415,
    1464, 908, 588, 1492, 20448, 1248, 2027, 2262, 3769, 4053, 4676, 5594, 5713, 6662, 5951, 5873, 4495,
    3375, 4793, 5842, 5924, 5875, 4928, 4075, 3795, 3342, 2234, 2668, 1435, 1630, 1064, 1842, 216, 796,
    18987, 932, 1994, 2344, 3205, 3743, 4465, 5631, 6009, 7096, 6377, 6694, 5381, 4557, 2976, 3019, 3607,
    5581, 5026, 6058, 6325, 5990, 5147, 4521, 3488, 3509, 2779, 2785, 2902, 2759, 13536, 1871, 1485, 927,
    509, 864, 681, 1821, 2644, 2779, 3309, 3580, 3798, 2824, 2444, 1766, 3737, 2738, 2149, 3066, 4515, 2084,
    8322, 4885, 5643, 3339, 3379, 3299, 2804, 2362, 18116
};

int32_t RecorderTest::GetStubFile()
{
    testFile_ = std::make_shared<std::ifstream>();
    TEST_CHECK_AND_RETURN_RET_LOG(testFile_ != nullptr, MSERR_INVALID_OPERATION, "create file failed");
    const std::string filePath = "/data/h264_1280_720.h264";
    testFile_->open(filePath, std::ios::in | std::ios::binary);
    TEST_CHECK_AND_RETURN_RET_LOG(testFile_->is_open(), MSERR_INVALID_OPERATION, "open file failed");

    return MSERR_OK;
}

void RecorderTest::HDICreateBuffer()
{
    // camera hdi loop to requeset buffer
    const uint32_t *frameLenArray = HIGH_VIDEO_FRAME_SIZE;
    while (count_ < STUB_STREAM_SIZE) {
        TEST_CHECK_AND_BREAK_LOG(!isExit_.load(), "close camera hdi thread");
        usleep(FRAME_RATE);
        OHOS::sptr<OHOS::SurfaceBuffer> buffer;
        int32_t releaseFence;
        OHOS::SurfaceError ret = producerSurface_->RequestBuffer(buffer, releaseFence, g_requestConfig);
        TEST_CHECK_AND_CONTINUE_LOG(ret != OHOS::SURFACE_ERROR_NO_BUFFER, "surface loop full, no buffer now");
        TEST_CHECK_AND_BREAK_LOG(ret == SURFACE_ERROR_OK && buffer != nullptr, "RequestBuffer failed");

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
        (void)testFile_->read(tempBuffer, *frameLenArray);
        if (*frameLenArray > buffer->GetSize()) {
            free(tempBuffer);
            (void)producerSurface_->CancelBuffer(buffer);
            break;
        }
        errno_t mRet = memcpy_s(addr, *frameLenArray, tempBuffer, *frameLenArray);
        if (mRet != EOK) {
            (void)producerSurface_->CancelBuffer(buffer);
            free(tempBuffer);
            break;
        }
        (void)buffer->ExtraSet("dataSize", static_cast<int32_t>(*frameLenArray));
        (void)buffer->ExtraSet("timeStamp", pts_);
        (void)buffer->ExtraSet("isKeyFrame", isKeyFrame_);
        count_++;
        (count_ % 30) == 0 ? (isKeyFrame_ = 1) : (isKeyFrame_ = 0);
        pts_ += FRAME_DURATION;
        (void)producerSurface_->FlushBuffer(buffer, -1, g_flushConfig);
        frameLenArray++;
        free(tempBuffer);
    }
    cout << "exit camera hdi loop" << endl;
    if ((testFile_ != nullptr) && (testFile_->is_open())) {
        testFile_->close();
    }
}

int32_t RecorderTest::CameraServicesForVideo() const
{
    int32_t ret = recorder_->SetVideoEncoder(g_videoRecorderConfig.videoSourceId,
        g_videoRecorderConfig.videoFormat);
    TEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetVideoEncoder failed ");

    ret = recorder_->SetVideoSize(g_videoRecorderConfig.videoSourceId,
        g_videoRecorderConfig.width, g_videoRecorderConfig.height);
    TEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetVideoSize failed ");

    ret = recorder_->SetVideoFrameRate(g_videoRecorderConfig.videoSourceId, g_videoRecorderConfig.frameRate);
    TEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetVideoFrameRate failed ");

    ret = recorder_->SetVideoEncodingBitRate(g_videoRecorderConfig.videoSourceId,
        g_videoRecorderConfig.videoEncodingBitRate);
    TEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetVideoEncodingBitRate failed ");
    return MSERR_OK;
}

int32_t RecorderTest::CameraServicesForAudio() const
{
    int32_t ret = recorder_->SetAudioEncoder(g_videoRecorderConfig.audioSourceId, g_videoRecorderConfig.audioFormat);
    TEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetAudioEncoder failed ");

    ret = recorder_->SetAudioSampleRate(g_videoRecorderConfig.audioSourceId, g_videoRecorderConfig.sampleRate);
    TEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetAudioSampleRate failed ");

    ret = recorder_->SetAudioChannels(g_videoRecorderConfig.audioSourceId, g_videoRecorderConfig.channelCount);
    TEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetAudioChannels failed ");

    ret = recorder_->SetAudioEncodingBitRate(g_videoRecorderConfig.audioSourceId,
        g_videoRecorderConfig.audioEncodingBitRate);
    TEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetAudioEncodingBitRate failed ");

    return MSERR_OK;
}

int32_t RecorderTest::SetFormat(const std::string &recorderType) const
{
    int32_t ret;
    if (recorderType == PURE_VIDEO) {
        ret = recorder_->SetVideoSource(g_videoRecorderConfig.vSource, g_videoRecorderConfig.videoSourceId);
        TEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetVideoSource failed ");
        ret = recorder_->SetOutputFormat(g_videoRecorderConfig.outPutFormat);
        TEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetOutputFormat failed ");
        ret = CameraServicesForVideo();
        TEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "CameraServices failed ");
    } else if (recorderType == PURE_AUDIO) {
        ret = recorder_->SetAudioSource(g_videoRecorderConfig.aSource, g_videoRecorderConfig.audioSourceId);
        TEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetAudioSource failed ");
        ret = recorder_->SetOutputFormat(g_audioRecorderConfig.outPutFormat);
        TEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetOutputFormat failed ");
        ret = CameraServicesForAudio();
        TEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "CameraServicesForAudio failed ");
    } else if (recorderType == AUDIO_VIDEO) {
        ret = recorder_->SetVideoSource(g_videoRecorderConfig.vSource, g_videoRecorderConfig.videoSourceId);
        TEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetVideoSource failed ");
        ret = recorder_->SetAudioSource(g_videoRecorderConfig.aSource, g_videoRecorderConfig.audioSourceId);
        TEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetAudioSource failed ");
        ret = recorder_->SetOutputFormat(g_videoRecorderConfig.outPutFormat);
        TEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetOutputFormat failed ");
        ret = CameraServicesForVideo();
        TEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "CameraServicesForVideo failed ");
        ret = CameraServicesForAudio();
        TEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "CameraServicesForAudio failed ");
    }

    ret = recorder_->SetMaxDuration(g_videoRecorderConfig.duration);
    TEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetMaxDuration failed ");
    ret = recorder_->SetOutputPath(g_videoRecorderConfig.outPath);
    TEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetOutputPath failed ");
    std::shared_ptr<RecorderCallbackTest> testCallback = std::make_shared<RecorderCallbackTest>();
    ret = recorder_->SetRecorderCallback(testCallback);
    TEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetRecorderCallback failed ");

    cout << "set format finished" << endl;
    return MSERR_OK;
}

void RecorderTest::TestCase()
{
    recorder_ = OHOS::Media::RecorderFactory::CreateRecorder();
    if (recorder_ == nullptr) {
        cout << "recorder_ is null" << endl;
        return;
    }

    string recorderType;
    cout << "recorder pure video audio or audio/video " << endl;
    cout << "pure video enter  :  1" << endl;
    cout << "pure audio enter  :  2" << endl;
    cout << "audio/video enter :  3" << endl;
    (void)getline(cin, recorderType);

    int32_t ret = SetFormat(recorderType);
    TEST_CHECK_AND_RETURN_LOG(ret == MSERR_OK, "SetFormat failed ");

    ret = recorder_->Prepare();
    TEST_CHECK_AND_RETURN_LOG(ret == MSERR_OK, "Prepare failed ");
    cout << "Prepare finished" << endl;

    if (recorderType != PURE_AUDIO) {
        producerSurface_ = recorder_->GetSurface(g_videoRecorderConfig.videoSourceId);
        TEST_CHECK_AND_RETURN_LOG(producerSurface_ != nullptr, "GetSurface failed ");

        ret = GetStubFile();
        TEST_CHECK_AND_RETURN_LOG(ret == MSERR_OK, "GetStubFile failed ");

        camereHDIThread_.reset(new(std::nothrow) std::thread(&RecorderTest::HDICreateBuffer, this));
    }

    ret = recorder_->Start();
    TEST_CHECK_AND_RETURN_LOG(ret == MSERR_OK, "Start failed ");
    cout << "start recordering" << endl;
    sleep(RECORDER_TIME);

    isExit_.store(true);
    ret = recorder_->Stop(false);
    TEST_CHECK_AND_RETURN_LOG(ret == MSERR_OK, "Stop failed ");
    cout << "stop recordering" << endl;
    if (recorderType != PURE_AUDIO && camereHDIThread_ != nullptr) {
        camereHDIThread_->join();
    }
    ret = recorder_->Reset();
    TEST_CHECK_AND_RETURN_LOG(ret == MSERR_OK, "Reset failed ");
    ret = recorder_->Release();
    TEST_CHECK_AND_RETURN_LOG(ret == MSERR_OK, "Release failed ");
}
