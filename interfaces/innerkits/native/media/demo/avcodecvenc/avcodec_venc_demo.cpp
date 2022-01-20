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

#include "avcodec_venc_demo.h"
#include <iostream>
#include "securec.h"
#include "demo_log.h"
#include "display_type.h"
#include "media_errors.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
namespace {
    constexpr uint32_t DEFAULT_WIDTH = 480;
    constexpr uint32_t DEFAULT_HEIGHT = 360;
    constexpr uint32_t DEFAULT_FRAME_RATE = 30;
    constexpr uint32_t YUV_BUFFER_SIZE = 259200; // 480 * 360 * 3 / 2
    constexpr uint32_t FRAME_INTERVAL_MS = 30000;
    constexpr uint32_t FRAME_DURATION_US = 40000000;
    constexpr uint32_t STRIDE_ALIGN = 8;
    constexpr uint32_t FRAME_COUNT = 150;
}

static BufferFlushConfig g_flushConfig = {
    .damage = {
        .x = 0,
        .y = 0,
        .w = DEFAULT_WIDTH,
        .h = DEFAULT_HEIGHT
    },
    .timestamp = 0
};

static BufferRequestConfig g_request = {
    .width = DEFAULT_WIDTH,
    .height = DEFAULT_HEIGHT,
    .strideAlignment = STRIDE_ALIGN,
    .format = PIXEL_FMT_YCRCB_420_SP,
    .usage = HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA,
    .timeout = 0
};

void VEncDemo::RunCase()
{
    DEMO_CHECK_AND_RETURN_LOG(CreateVenc() == MSERR_OK, "Fatal: CreateVenc fail");

    Format format;
    format.PutIntValue("width", DEFAULT_WIDTH);
    format.PutIntValue("height", DEFAULT_HEIGHT);
    format.PutIntValue("pixel_format", 3); // NV21
    format.PutIntValue("frame_rate", DEFAULT_FRAME_RATE);
    DEMO_CHECK_AND_RETURN_LOG(Configure(format) == MSERR_OK, "Fatal: Configure fail");

    sptr<Surface> surface = GetVideoSurface();
    DEMO_CHECK_AND_RETURN_LOG(surface != nullptr, "Fatal: GetVideoSurface fail");

    DEMO_CHECK_AND_RETURN_LOG(Prepare() == MSERR_OK, "Fatal: Prepare fail");
    DEMO_CHECK_AND_RETURN_LOG(Start() == MSERR_OK, "Fatal: Start fail");

    uint32_t frameCount = 0;
    int64_t timeStamp = 0;
    while (frameCount <= FRAME_COUNT) {
        usleep(FRAME_INTERVAL_MS);
        sptr<OHOS::SurfaceBuffer> buffer = nullptr;
        int32_t fence = -1;
        if (surface->RequestBuffer(buffer, fence, g_request) != SURFACE_ERROR_OK) {
            continue;
        }
        DEMO_CHECK_AND_BREAK_LOG(buffer != nullptr, "Fatal: SurfaceBuffer is nullptr");

        auto addr = static_cast<uint8_t *>(buffer->GetVirAddr());
        if (addr == nullptr) {
            cout << "Fatal: SurfaceBuffer address is nullptr" << endl;
            (void)surface->CancelBuffer(buffer);
            break;
        }
        DEMO_CHECK_AND_BREAK_LOG(memset_s(addr, buffer->GetSize(), 0xFF, YUV_BUFFER_SIZE) == EOK, "Fatal");
        (void)buffer->ExtraSet("timeStamp", timeStamp);
        timeStamp += FRAME_DURATION_US;
        (void)surface->FlushBuffer(buffer, -1, g_flushConfig);
        cout << "Generate input buffer success" << endl;
        frameCount++;
    }

    DEMO_CHECK_AND_RETURN_LOG(Stop() == MSERR_OK, "Fatal: Stop fail");
    DEMO_CHECK_AND_RETURN_LOG(Release() == MSERR_OK, "Fatal: Release fail");
}

int32_t VEncDemo::CreateVenc()
{
    venc_ = VideoEncoderFactory::CreateByMime("video/mp4v-es");
    DEMO_CHECK_AND_RETURN_RET_LOG(venc_ != nullptr, MSERR_UNKNOWN, "Fatal: CreateByMime fail");

    signal_ = make_shared<VEncSignal>();
    DEMO_CHECK_AND_RETURN_RET_LOG(signal_ != nullptr, MSERR_UNKNOWN, "Fatal: No memory");

    cb_ = make_shared<VEncDemoCallback>(signal_);
    DEMO_CHECK_AND_RETURN_RET_LOG(cb_ != nullptr, MSERR_UNKNOWN, "Fatal: No memory");
    DEMO_CHECK_AND_RETURN_RET_LOG(venc_->SetCallback(cb_) == MSERR_OK, MSERR_UNKNOWN, "Fatal: SetCallback fail");

    return MSERR_OK;
}

int32_t VEncDemo::Configure(const Format &format)
{
    return venc_->Configure(format);
}

int32_t VEncDemo::Prepare()
{
    return venc_->Prepare();
}

int32_t VEncDemo::Start()
{
    isRunning_.store(true);
    readLoop_ = make_unique<thread>(&VEncDemo::LoopFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(readLoop_ != nullptr, MSERR_UNKNOWN, "Fatal: No memory");
    return venc_->Start();
}

int32_t VEncDemo::Stop()
{
    isRunning_.store(false);

    if (readLoop_ != nullptr && readLoop_->joinable()) {
        unique_lock<mutex> queueLock(signal_->mutex_);
        signal_->bufferQueue_.push(10000); // wake up read loop thread
        signal_->cond_.notify_all();
        queueLock.unlock();
        readLoop_->join();
        readLoop_.reset();
    }

    return venc_->Stop();
}

int32_t VEncDemo::Flush()
{
    return venc_->Flush();
}

int32_t VEncDemo::Reset()
{
    return venc_->Reset();
}

int32_t VEncDemo::Release()
{
    return venc_->Release();
}

sptr<Surface> VEncDemo::GetVideoSurface()
{
    return venc_->CreateInputSurface();
}

void VEncDemo::LoopFunc()
{
    while (true) {
        if (!isRunning_.load()) {
            break;
        }

        unique_lock<mutex> lock(signal_->mutex_);
        signal_->cond_.wait(lock, [this](){ return signal_->bufferQueue_.size() > 0; });

        if (!isRunning_.load()) {
            break;
        }

        uint32_t index = signal_->bufferQueue_.front();
        auto buffer = venc_->GetOutputBuffer(index);
        if (!buffer) {
            cout << "Fatal: GetOutputBuffer fail, exit" << endl;
            break;
        }

        if (venc_->ReleaseOutputBuffer(index) != MSERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail, exit" << endl;
            break;
        }
        signal_->bufferQueue_.pop();
    }
}

VEncDemoCallback::VEncDemoCallback(shared_ptr<VEncSignal> signal)
    : signal_(signal)
{
}

void VEncDemoCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    cout << "Error received, errorType:" << errorType << " errorCode:" << errorCode << endl;
}

void VEncDemoCallback::OnOutputFormatChanged(const Format &format)
{
    cout << "OnOutputFormatChanged received" << endl;
}

void VEncDemoCallback::OnInputBufferAvailable(uint32_t index)
{
}

void VEncDemoCallback::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    cout << "OnOutputBufferAvailable received, index:" << index << " timestamp:" << info.presentationTimeUs << endl;
    unique_lock<mutex> lock(signal_->mutex_);
    signal_->bufferQueue_.push(index);
    signal_->cond_.notify_all();
}
