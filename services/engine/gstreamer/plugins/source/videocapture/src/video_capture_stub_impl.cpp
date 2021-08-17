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

#include "video_capture_stub_impl.h"
#include <map>
#include "param_wrapper.h"
#include "media_log.h"
#include "media_errors.h"
#include "scope_guard.h"
#include "securec.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoCaptureStubImpl"};
    constexpr int32_t DEFAULT_FRAME_DURATION = 33425000;
    constexpr uint32_t DEFAULT_SURFACE_WIDTH = 1920;
    constexpr uint32_t DEFAULT_SURFACE_HEIGHT = 1080;
    const std::string DEFAULT_FILE_PATH = "/data/h264_1920_1080.es";
    constexpr int32_t DEFAULT_SURFACE_QUEUE_SIZE = 3;
    constexpr int32_t DEFAULT_SURFACE_SIZE = 1024 * 1024;
}

namespace OHOS {
namespace Media {
const std::map<uint32_t, uint32_t> SURFACE_MAP = {
    { 1920, 1080 },
    { 1280, 720 },
    { 720, 480 },
};

// frame offset of video frame in h264.es
const int32_t HIGH_VIDEO_FRAME_SIZE[226] =
    { 31197, 785, 4084, 3532, 3468, 3449, 964, 3345, 3725, 3374, 3626, 1098, 3330, 3305, 3283, 6903,
      1388, 3594, 3791, 3671, 3678, 1350, 3466, 3475, 3591, 3551, 1092, 3318, 3207, 3340, 6454, 1420, 3506,
      3737, 3769, 3875, 1166, 3604, 3808, 3678, 3613, 1163, 3419, 4055, 3839, 6840, 1391, 3742, 4267, 4333,
      4253, 1280, 4139, 4480, 3799, 3854, 1026, 3388, 3292, 3562, 32673, 624, 4205, 3659, 3854, 3813, 835,
      3453, 3558, 3889, 3800, 986, 3364, 3790, 3379, 6625, 963, 3294, 3186, 3667, 3344, 1240, 3093, 3589,
      2611, 3353, 818, 3170, 3264, 3360, 4881, 1510, 2426, 2742, 2484, 2965, 764, 2250, 2159, 2130, 1529, 345,
      972, 662, 630, 35704, 1112, 2724, 2896, 3070, 3070, 1424, 2746, 3448, 3111, 3900, 1277, 2937, 3008,
      2708, 6146, 1463, 4032, 3275, 3035, 3222, 1125, 3337, 2851, 2632, 2664, 1626, 2309, 2547, 2582, 5632,
      1274, 2526, 2681, 3137, 2754, 1251, 2526, 3006, 2521, 2886, 814, 2414, 1951, 2356, 4652, 1421, 2469,
      2771, 2624, 3054, 796, 2463, 2205, 2783, 2803, 521, 4925, 6872, 8551, 31033, 9787, 10254, 13513, 14272,
      15447, 16037, 12133, 17105, 18514, 18871, 19697, 12643, 20666, 21506, 27645, 20742, 18800, 24027, 26226,
      26769, 28484, 11077, 26956, 28647, 30788, 33112, 9286, 29835, 32406, 35572, 30705, 2307, 26235, 28120,
      28891, 31952, 1072, 32607, 32968, 28632, 29051, 2010, 24916, 24573, 29684, 25905, 1089, 22359, 22607,
      21882, 24131, 1193, 22290, 24737, 26631, 28613, 1488, 24672, 24947 };

const int32_t MEDIUM_VIDEO_FRAME_SIZE[185] =
    { 22103, 16744, 13748, 382, 9401, 13781, 9113, 435, 12823, 8412, 12119, 7238, 306, 12701, 8220, 6960,
      12702, 10172, 270, 7705, 10033, 9878, 7798, 1342, 41629, 15084, 8142, 580, 6127, 15182, 9513, 887,
      6110, 17744, 6294, 19037, 8607, 619, 7364, 18685, 11962, 1045, 7779, 19412, 7723, 18739, 9719, 541,
      7403, 17992, 12088, 707, 7347, 17487, 7522, 17811, 10219, 466, 8300, 17740, 13157, 532, 9212, 16816,
      8511, 14516, 8134, 539, 5909, 13708, 8014, 962, 5523, 15699, 5901, 16471, 9072, 554, 7728, 12270,
      47278, 20035, 5555, 4769, 22270, 7743, 1596, 5426, 20404, 9258, 753, 5792, 23463, 5836, 22047, 7143,
      740, 5207, 20719, 8349, 708, 4717, 19710, 4223, 19145, 5435, 625, 4556, 18769, 7153, 1020, 4168,
      17812, 4103, 18842, 5908, 680, 4841, 17715, 8151, 677, 4729, 16668, 4822, 21720, 7800, 2387, 4916,
      20115, 8407, 682, 5056, 19555, 5056, 21030, 6721, 600, 4910, 15224, 4256, 24242, 8168, 1992, 6759,
      20484, 6797, 1233, 5343, 20065, 10362, 631, 7088, 23135, 10767, 6673, 733, 20229, 8049, 5457, 462,
      24554, 10294, 5752, 4203, 722, 12639, 13072, 7066, 8082, 644, 16680, 11435, 7087, 7500, 13629, 8425,
      662, 4856, 10455, 8992, 766, 3450, 8923, 8041, 3603 };

const int32_t LOW_VIDEO_FRAME_SIZE[159] =
    { 11963, 8677, 6697, 203, 4613, 8207, 4301, 7544, 4668, 139, 3383, 6434, 4685, 127, 3132, 7275, 3236,
      6232, 4464, 113, 3988, 4877, 3920, 510, 22070, 7263, 3637, 198, 2808, 7721, 4485, 261, 2815, 8954,
      2956, 9993, 4237, 209, 3563, 9617, 5869, 298, 3710, 9959, 3830, 9431, 4822, 151, 3699, 8976, 5808,
      235, 3679, 8821, 3691, 8684, 5162, 167, 4281, 8567, 8900, 5603, 157, 4697, 7274, 5552, 343, 3200,
      6821, 2505, 7542, 3238, 162, 2836, 7833, 4802, 270, 3412, 7253, 4400, 25342, 10450, 3096, 2121, 321,
      10916, 3913, 2447, 2377, 10279, 3460, 283, 2507, 11240, 4692, 192, 2325, 9849, 2036, 9591, 2603, 267,
      2025, 9519, 3120, 237, 1804, 9095, 1856, 8460, 2487, 319, 1774, 9010, 3358, 223, 2010, 9124, 1917,
      8491, 2851, 280, 2265, 9890, 3883, 433, 2235, 8336, 2028, 777, 11778, 4398, 2342, 2361, 10531, 2973,
      209, 2042, 7375, 1318, 11809, 3893, 1808, 1870, 9397, 2915, 318, 2231, 9576, 4471, 213, 3023, 10621,
      2734, 9527, 3700, 200, 2381, 4096 };

VideoCaptureStubImpl::VideoCaptureStubImpl()
    : surfaceWidth_(DEFAULT_SURFACE_WIDTH),
      surfaceHeight_(DEFAULT_SURFACE_HEIGHT),
      testFile_(nullptr),
      frameSequence_(0),
      codecData_(nullptr),
      codecDataSize_(0),
      pts_(0),
      dataConSurface_(nullptr),
      frameWidth_(DEFAULT_SURFACE_WIDTH),
      frameHeight_(DEFAULT_SURFACE_HEIGHT),
      filePath_(DEFAULT_FILE_PATH),
      frameLenArray_(HIGH_VIDEO_FRAME_SIZE),
      nalSize_(0)
{
}

VideoCaptureStubImpl::~VideoCaptureStubImpl()
{
    Stop();
}

int32_t VideoCaptureStubImpl::Prepare()
{
    auto iter = SURFACE_MAP.find(surfaceWidth_);
    CHECK_AND_RETURN_RET_LOG(iter != SURFACE_MAP.end(), MSERR_INVALID_VAL, "illegal surface width");
    CHECK_AND_RETURN_RET_LOG(surfaceHeight_ == iter->second, MSERR_INVALID_VAL, "illegal surface height");

    ProcessSysProperty();

    testFile_ = std::make_unique<std::ifstream>();
    CHECK_AND_RETURN_RET_LOG(testFile_ != nullptr, MSERR_NO_MEMORY, "no memory");
    testFile_->open(filePath_, std::ios::in | std::ios::binary);
    CHECK_AND_RETURN_RET_LOG(testFile_->is_open(), MSERR_UNKNOWN, "open local file fail");

    sptr<Surface> consumerSurface = Surface::CreateSurfaceAsConsumer();
    CHECK_AND_RETURN_RET_LOG(consumerSurface != nullptr, MSERR_UNKNOWN, "create surface fail");

    sptr<IBufferConsumerListener> listenerProxy = new (std::nothrow) ConsumerListenerProxy(*this);
    CHECK_AND_RETURN_RET_LOG(listenerProxy != nullptr, MSERR_UNKNOWN, "create consumer listener fail");

    if (consumerSurface->RegisterConsumerListener(listenerProxy) != SURFACE_ERROR_OK) {
        MEDIA_LOGW("register consumer listener fail");
    }

    sptr<IBufferProducer> producer = consumerSurface->GetProducer();
    CHECK_AND_RETURN_RET_LOG(producer != nullptr, MSERR_UNKNOWN, "get producer fail");
    sptr<Surface> producerSurface = Surface::CreateSurfaceAsProducer(producer);
    CHECK_AND_RETURN_RET_LOG(producerSurface != nullptr, MSERR_UNKNOWN, "get producer fail");

    dataConSurface_ = consumerSurface;
    producerSurface_ = producerSurface;

    SetSurfaceUserData();
    return MSERR_OK;
}

int32_t VideoCaptureStubImpl::Start()
{
    MEDIA_LOGD("Start");
    return MSERR_OK;
}

int32_t VideoCaptureStubImpl::Pause()
{
    MEDIA_LOGD("Pause");
    return MSERR_OK;
}

int32_t VideoCaptureStubImpl::Resume()
{
    MEDIA_LOGD("Resume");
    return MSERR_OK;
}

int32_t VideoCaptureStubImpl::Stop()
{
    MEDIA_LOGD("Stop");

    if ((testFile_ != nullptr) && (testFile_->is_open())) {
        testFile_->close();
    }
    testFile_ = nullptr;
    if (codecData_ != nullptr) {
        free(codecData_);
        codecData_ = nullptr;
    }

    if (dataConSurface_ != nullptr) {
        if (dataConSurface_->UnregisterConsumerListener() != SURFACE_ERROR_OK) {
            MEDIA_LOGW("deregister consumer listener fail");
        }
    }

    return MSERR_OK;
}

int32_t VideoCaptureStubImpl::SetSurfaceWidth(uint32_t width)
{
    surfaceWidth_ = width;
    return MSERR_OK;
}

int32_t VideoCaptureStubImpl::SetSurfaceHeight(uint32_t height)
{
    surfaceHeight_ = height;
    return MSERR_OK;
}

sptr<Surface> VideoCaptureStubImpl::GetSurface()
{
    CHECK_AND_RETURN_RET_LOG(producerSurface_ != nullptr, nullptr, "surface not created");
    return producerSurface_;
}

void VideoCaptureStubImpl::ConsumerListenerProxy::OnBufferAvailable()
{
    return owner_.OnBufferAvailable();
}

void VideoCaptureStubImpl::OnBufferAvailable()
{
    return;
}

std::shared_ptr<EsAvcCodecBuffer> VideoCaptureStubImpl::GetCodecBuffer()
{
    CHECK_AND_RETURN_RET_LOG(testFile_ != nullptr, nullptr, "ifstream is nullptr");
    char *buffer = (char *) malloc(sizeof(char) * (*frameLenArray_) + 1);
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, nullptr, "no memory");

    ON_SCOPE_EXIT(0) { free(buffer); };

    testFile_->read(buffer, *frameLenArray_);
    std::vector<uint8_t> sps;
    std::vector<uint8_t> pps;
    GetCodecData(reinterpret_cast<const uint8_t *>(buffer), *frameLenArray_, sps, pps, nalSize_);
    CHECK_AND_RETURN_RET_LOG(nalSize_ > 0 && sps.size() > 0 && pps.size() > 0, nullptr, "illegal codec buffer");

    // 11 is the length of AVCDecoderConfigurationRecord field except sps and pps
    uint32_t codecBufferSize = sps.size() + pps.size() + 11;
    GstBuffer *codec = gst_buffer_new_allocate(nullptr, codecBufferSize, nullptr);
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "no memory");
    GstMapInfo map;
    CHECK_AND_RETURN_RET_LOG(gst_buffer_map(codec, &map, GST_MAP_READ) == TRUE, nullptr, "gst_buffer_map fail");

    ON_SCOPE_EXIT(1) { gst_buffer_unmap(codec, &map); };
    ON_SCOPE_EXIT(2) { gst_buffer_unref(codec); };

    uint32_t offset = 0;
    map.data[offset++] = 0x01; // configurationVersion
    map.data[offset++] = sps[1]; // AVCProfileIndication
    map.data[offset++] = sps[2]; // profile_compatibility
    map.data[offset++] = sps[3]; // AVCLevelIndication
    map.data[offset++] = 0xff; // lengthSizeMinusOne

    map.data[offset++] = 0xe0 | 0x01; // numOfSequenceParameterSets
    map.data[offset++] = (sps.size() >> 8) & 0xff; // sequenceParameterSetLength high 8 bits
    map.data[offset++] = sps.size() & 0xff; // sequenceParameterSetLength low 8 bits
    // sequenceParameterSetNALUnit
    CHECK_AND_RETURN_RET_LOG(memcpy_s(map.data + offset, codecBufferSize - offset, &sps[0], sps.size()) == EOK,
                             nullptr, "memcpy_s fail");
    offset += sps.size();

    map.data[offset++] = 0x01; // numOfPictureParameterSets
    map.data[offset++] = (pps.size() >> 8) & 0xff; // pictureParameterSetLength  high 8 bits
    map.data[offset++] = pps.size() & 0xff; // pictureParameterSetLength  low 8 bits
    // pictureParameterSetNALUnit
    CHECK_AND_RETURN_RET_LOG(memcpy_s(map.data + offset, codecBufferSize - offset, &pps[0], pps.size()) == EOK,
                             nullptr, "memcpy_s fail");

    std::shared_ptr<EsAvcCodecBuffer> codecBuffer = std::make_shared<EsAvcCodecBuffer>();
    CHECK_AND_RETURN_RET_LOG(codecBuffer != nullptr, nullptr, "no memory");
    codecBuffer->width = frameWidth_;
    codecBuffer->height = frameHeight_;
    codecBuffer->segmentStart = 0;
    codecBuffer->gstCodecBuffer = codec;
    codecData_ = buffer;
    codecDataSize_ = nalSize_ + sps.size() + nalSize_ + pps.size();

    CANCEL_SCOPE_EXIT_GUARD(0);
    CANCEL_SCOPE_EXIT_GUARD(2);

    return codecBuffer;
}

std::shared_ptr<VideoFrameBuffer> VideoCaptureStubImpl::GetFrameBuffer()
{
    CHECK_AND_RETURN_RET_LOG(testFile_ != nullptr, nullptr, "ifstream is nullptr");
    g_usleep(30000);
    if (frameSequence_ == 0 && codecData_ == nullptr) {
        (void)GetCodecBuffer();
    }

    if (frameSequence_ == 0) {
        return GetFirstBuffer();
    }

    char *buffer = (char *)malloc(sizeof(char) * (*frameLenArray_) + 1);
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, nullptr, "no memory");

    ON_SCOPE_EXIT(0) {
        free(buffer);
        buffer = nullptr;
    };

    testFile_->read(buffer, *frameLenArray_);

    uint32_t frameSize = (*frameLenArray_) - nalSize_;
    // 0x00000001, 0x000001
    if (nalSize_ == 4) {
        buffer[0] = (char)((frameSize >> 24) & 0xff);
        buffer[1] = (char)((frameSize >> 16) & 0xff);
        buffer[2] = (char)((frameSize >> 8) & 0xff);
        buffer[3] = (char)(frameSize & 0xff);
    } else {
        buffer[0] = (char)((frameSize >> 16) & 0xff);
        buffer[1] = (char)((frameSize >> 8) & 0xff);
        buffer[2] = (char)(frameSize & 0xff);
    }

    GstBuffer *gstBuffer = gst_buffer_new_allocate(nullptr, *frameLenArray_, nullptr);
    CHECK_AND_RETURN_RET_LOG(gstBuffer != nullptr, nullptr, "no memory");

    ON_SCOPE_EXIT(1) { gst_buffer_unref(gstBuffer); };

    gsize size = gst_buffer_fill(gstBuffer, 0, buffer, *frameLenArray_);
    CHECK_AND_RETURN_RET_LOG(size == static_cast<gsize>(*frameLenArray_), nullptr,
                             "unkonwn error during gst_buffer_fill");

    std::shared_ptr<VideoFrameBuffer> frameBuffer = std::make_shared<VideoFrameBuffer>();
    frameBuffer->keyFrameFlag = 0;
    frameBuffer->timeStamp = pts_;
    pts_ += DEFAULT_FRAME_DURATION;
    frameBuffer->gstBuffer = gstBuffer;
    frameBuffer->duration = DEFAULT_FRAME_DURATION;
    frameBuffer->size = *frameLenArray_;
    frameLenArray_++;

    CANCEL_SCOPE_EXIT_GUARD(1);
    return frameBuffer;
}

void VideoCaptureStubImpl::SetSurfaceUserData()
{
    SurfaceError ret = dataConSurface_->SetUserData("surface_width", std::to_string(surfaceWidth_));
    if (ret != SURFACE_ERROR_OK) {
        MEDIA_LOGW("set surface width fail");
    }
    ret = dataConSurface_->SetUserData("surface_height", std::to_string(surfaceHeight_));
    if (ret != SURFACE_ERROR_OK) {
        MEDIA_LOGW("set surface height fail");
    }
    ret = dataConSurface_->SetQueueSize(DEFAULT_SURFACE_QUEUE_SIZE);
    if (ret != SURFACE_ERROR_OK) {
        MEDIA_LOGW("set queue size fail");
    }
    ret = dataConSurface_->SetUserData("surface_size", std::to_string(DEFAULT_SURFACE_SIZE));
    if (ret != SURFACE_ERROR_OK) {
        MEDIA_LOGW("set surface size fail");
    }
}

void VideoCaptureStubImpl::ProcessSysProperty()
{
    std::string videoQuality;
    int32_t res = OHOS::system::GetStringParameter("sys.media.videosrc.stub.quality", videoQuality, "");
    if (res != 0 || videoQuality.empty()) {
        videoQuality = "high";
    }

    if (videoQuality == "high") {
        frameWidth_ = 1920;
        frameHeight_ = 1080;
        frameLenArray_ = HIGH_VIDEO_FRAME_SIZE;
        filePath_ = "/data/h264_1920_1080.es";
    } else if (videoQuality == "medium") {
        frameWidth_ = 1280;
        frameHeight_ = 720;
        frameLenArray_ = MEDIUM_VIDEO_FRAME_SIZE;
        filePath_ = "/data/h264_1280_720.es";
    } else if (videoQuality == "low") {
        frameWidth_ = 720;
        frameHeight_ = 480;
        frameLenArray_ = LOW_VIDEO_FRAME_SIZE;
        filePath_ = "/data/h264_720_480.es";
    }
}

std::shared_ptr<VideoFrameBuffer> VideoCaptureStubImpl::GetFirstBuffer()
{
    CHECK_AND_RETURN_RET_LOG(codecData_ != nullptr, nullptr, "no valid codec data");

    ON_SCOPE_EXIT(0) {
        free(codecData_);
        codecData_ = nullptr;
    };

    int32_t bufferSize = *frameLenArray_ - codecDataSize_;
    GstBuffer *gstBuffer = gst_buffer_new_allocate(nullptr, bufferSize, nullptr);
    CHECK_AND_RETURN_RET_LOG(gstBuffer != nullptr, nullptr, "no memory");

    ON_SCOPE_EXIT(1) { gst_buffer_unref(gstBuffer); };

    // 0x00000001, 0x000001
    uint32_t frameSize = bufferSize - nalSize_;
    if (nalSize_ == 4) {
        codecData_[codecDataSize_] = (char)((frameSize >> 24) & 0xff);
        codecData_[codecDataSize_ + 1] = (char)((frameSize >> 16) & 0xff);
        codecData_[codecDataSize_ + 2] = (char)((frameSize >> 8) & 0xff);
        codecData_[codecDataSize_ + 3] = (char)(frameSize & 0xff);
    } else {
        codecData_[codecDataSize_] = (char)((frameSize >> 16) & 0xff);
        codecData_[codecDataSize_ + 1] = (char)((frameSize >> 8) & 0xff);
        codecData_[codecDataSize_ + 2] = (char)(frameSize & 0xff);
    }

    gsize size = gst_buffer_fill(gstBuffer, 0, codecData_ + codecDataSize_, bufferSize);
    CHECK_AND_RETURN_RET_LOG(size == (gsize)bufferSize, nullptr, "unkonwn error during gst_buffer_fill");

    std::shared_ptr<VideoFrameBuffer> frameBuffer = std::make_shared<VideoFrameBuffer>();
    CHECK_AND_RETURN_RET_LOG(frameBuffer != nullptr, nullptr, "no memory");
    const uint32_t firstFrameDuration = 40000000;
    frameBuffer->keyFrameFlag = 0;
    frameBuffer->timeStamp = pts_;
    pts_ += firstFrameDuration;
    frameBuffer->gstBuffer = gstBuffer;
    frameBuffer->duration = firstFrameDuration;
    frameBuffer->size = bufferSize;
    frameSequence_++;
    frameLenArray_++;

    CANCEL_SCOPE_EXIT_GUARD(1);
    return frameBuffer;
}

const uint8_t *VideoCaptureStubImpl::FindNextNal(const uint8_t *start, const uint8_t *end, uint32_t &nalSize)
{
    CHECK_AND_RETURN_RET(start != nullptr && end != nullptr, nullptr);
    while (start <= end - 3) {
        // 0x000001 Nal
        if (start[0] == 0x00 && start[1] == 0x00 && start[2] == 0x01) {
            nalSize = 3;
            return start;
        }
        // 0x00000001 Nal
        if (start[0] == 0x00 && start[1] == 0x00 && start[2] == 0x00 && start[3] == 0x01) {
            nalSize = 4;
            return start;
        }
        start++;
    }
    return end;
}

void VideoCaptureStubImpl::GetCodecData(const uint8_t *data, int32_t len,
    std::vector<uint8_t> &sps, std::vector<uint8_t> &pps, uint32_t &nalSize)
{
    CHECK_AND_RETURN(data != nullptr);
    const uint8_t *end = data + len;
    const uint8_t *pBegin = data;
    const uint8_t *pEnd = nullptr;
    while (pBegin < end) {
        pBegin = FindNextNal(pBegin, end, nalSize);
        if (pBegin == end) {
            break;
        }
        pBegin += nalSize;
        pEnd = FindNextNal(pBegin, end, nalSize);
        if (((*pBegin) & 0x1F) == 0x07) { // sps
            sps.assign(pBegin, pBegin + static_cast<int>(pEnd - pBegin));
        }
        if (((*pBegin) & 0x1F) == 0x08) { // pps
            pps.assign(pBegin, pBegin + static_cast<int>(pEnd - pBegin));
        }
        pBegin = pEnd;
    }
}
}  // namespace Media
}  // namespace OHOS