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
    constexpr uint32_t STUB_STREAM_SIZE = 612;
    constexpr uint32_t FRAME_RATE = 30000;
    constexpr uint32_t CODEC_BUFFER_WIDTH = 1024;
    constexpr uint32_t CODEC_BUFFER_HEIGHT = 25;
    constexpr uint32_t STRIDE_ALIGN = 8;
    constexpr uint32_t FIRST_FRAME_DURATION = 40000000;
    constexpr uint32_t FRAME_DURATION = 33425000;
    constexpr uint32_t RECORDER_TIME = 8;
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
    31197, 785, 4084, 3532, 3468, 3449, 964, 3345, 3725, 3374, 3626, 1098, 3330, 3305, 3283, 6903,
    1388, 3594, 3791, 3671, 3678, 1350, 3466, 3475, 3591, 3551, 1092, 3318, 3207, 3340, 6454, 1420, 3506,
    3737, 3769, 3875, 1166, 3604, 3808, 3678, 3613, 1163, 3419, 4055, 3839, 6840, 1391, 3742, 4267, 4333,
    4253, 1280, 4139, 4480, 3799, 3854, 1026, 3388, 3292, 3562, 32673, 624, 4205, 3659, 3854, 3813, 835,
    3453, 3558, 3889, 3800, 986, 3364, 3790, 3379, 6625, 963, 3294, 3186, 3667, 3344, 1240, 3093, 3589,
    2611, 3353, 818, 3170, 3264, 3360, 4881, 1510, 2426, 2742, 2484, 2965, 764, 2250, 2159, 2130, 1529,
    345, 972, 662, 630, 35704, 1112, 2724, 2896, 3070, 3070, 1424, 2746, 3448, 3111, 3900, 1277, 2937, 3008,
    2708, 6146, 1463, 4032, 3275, 3035, 3222, 1125, 3337, 2851, 2632, 2664, 1626, 2309, 2547, 2582, 5632,
    1274, 2526, 2681, 3137, 2754, 1251, 2526, 3006, 2521, 2886, 814, 2414, 1951, 2356, 4652, 1421, 2469,
    2771, 2624, 3054, 796, 2463, 2205, 2783, 2803, 521, 4925, 6872, 8551, 31033, 9787, 10254, 13513, 14272,
    15447, 16037, 12133, 17105, 18514, 18871, 19697, 12643, 20666, 21506, 27645, 20742, 18800, 24027, 26226,
    26769, 28484, 11077, 26956, 28647, 30788, 33112, 9286, 29835, 32406, 35572, 30705, 2307, 26235, 28120,
    28891, 31952, 1072, 32607, 32968, 28632, 29051, 2010, 24916, 24573, 29684, 25905, 1089, 22359, 22607,
    21882, 24131, 1193, 22290, 24737, 26631, 28613, 1488, 24672, 24947, 67244, 22503, 1303, 23184, 25121,
    26596, 28764, 1449, 25110, 26222, 26624, 28688, 1551, 24429, 24756, 32551, 25001, 9917, 27482, 29057,
    27699, 26879, 1556, 24305, 26122, 26567, 27387, 1464, 23607, 25111, 30900, 27487, 1241, 25425, 24539,
    26847, 29093, 855, 25092, 24771, 21725, 23147, 1049, 21197, 20709, 25595, 22107, 989, 19552, 21422,
    20860, 860, 16846, 17157, 17698, 18440, 1031, 15376, 15480, 35884, 14296, 464, 14401, 12730, 12952,
    12166, 460, 9093, 6349, 7421, 4496, 3601, 325, 3147, 5315, 5610, 7805, 442, 7337, 7705, 7709, 8260,
    539, 7719, 7893, 8016, 8374, 545, 7617, 9014, 8663, 9565, 497, 8459, 8719, 8534, 9208, 501, 7384,
    8432, 10209, 12020, 933, 11971, 14345, 12834, 13185, 1420, 11401, 11618, 11226, 11852, 1343,
    10442, 10644, 10734, 10977, 1618, 9352, 35624, 9202, 10590, 1319, 9722, 9908, 9902, 10399, 1852, 9494,
    10412, 10100, 10701, 1189, 9732, 14164, 10204, 12334, 1099, 9956, 10790, 10226, 11455, 1363, 11482, 12969,
    12530, 13274, 1357, 9081, 14601, 11146, 13819, 1351, 12070, 12543, 11421, 12156, 1226, 10110, 10029, 9956,
    10632, 1016, 9001, 10776, 8299, 7520, 754, 36614, 9579, 9988, 9715, 1430, 9742, 10214, 9637, 10005, 7546,
    10298, 16356, 12402, 12457, 6946, 13289, 12179, 13869, 14993, 7355, 14856, 15892, 15530, 14996, 9611, 14569,
    18244, 15906, 16474, 5330, 15342, 15165, 15135, 17146, 9169, 13615, 15490, 16992, 15316, 7257, 16976, 17882,
    13253, 18520, 9795, 13041, 17877, 15932, 14396, 12038, 16912, 14416, 16271, 17908, 8880, 14257, 20485, 13466,
    12230, 11794, 29670, 8826, 12739, 10869, 5996, 23619, 19552, 19432, 19313, 20401, 8858, 18229, 19786, 21848,
    26781, 11326, 22042, 20603, 21452, 23263, 11366, 19500, 21040, 18793, 23337, 6474, 21074, 18743, 16815, 15872,
    7149, 12303, 11993, 11463, 12773, 5315, 64838, 25544, 29392, 32703, 1268, 30266, 26460, 26437, 28674, 2753,
    22593, 23888, 24000, 25587, 1698, 20536, 20897, 18575, 19355, 1726, 17855, 16729, 16687, 16931, 846, 13345, 12988,
    12724, 12625, 699, 10588, 11381, 13724, 18615, 791, 18171, 18174, 18821, 20477, 850, 16970, 19558, 20364, 23982,
    1061, 20855, 22747, 24841, 27388, 1278, 25831, 27721, 28749, 32541, 1417, 27845, 31165, 30490, 33918, 1237,
    62492, 30168, 33918, 35696, 852, 26051, 20160, 16978, 18900, 12969, 16858, 5129, 12635, 13797, 13117, 16921, 4610,
    12292, 11839, 10739, 31918, 4955, 16772, 18961, 20193, 23374, 7177, 18154, 19559, 19984, 24131, 7090, 20187,
    20948, 21224, 27894, 8023, 21147, 21784, 22388, 24928, 7955, 21405, 22666, 22475, 26376, 7368, 18743, 17444,
    9763, 8432, 8873, 9444, 655, 8454, 8651, 9108, 10486, 877, 9877, 10388
};

int32_t RecorderTest::GetStubFile()
{
    testFile_ = std::make_shared<std::ifstream>();
    TEST_CHECK_AND_RETURN_RET_LOG(testFile_ != nullptr, MSERR_INVALID_OPERATION, "create file failed");
    const std::string filePath = "/data/h264_1920_1080.es";
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
        count_ == 0 ? (duration_ = FIRST_FRAME_DURATION) : (duration_ = FRAME_DURATION);
        (void)buffer->ExtraSet("pts", pts_);
        pts_ += duration_;
        (void)producerSurface_->FlushBuffer(buffer, -1, g_flushConfig);
        count_++;
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
