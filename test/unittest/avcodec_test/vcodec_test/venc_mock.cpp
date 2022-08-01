/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include <sync_fence.h>
#include "nocopyable.h"
#include "media_errors.h"
#include "venc_mock.h"
using namespace std;
using namespace OHOS::Media::VCodecTestParam;

namespace OHOS {
namespace Media {
VEncCallbackTest::VEncCallbackTest(std::shared_ptr<VEncSignal> signal)
    : signal_(signal)
{
}

VEncCallbackTest::~VEncCallbackTest()
{
}

void VEncCallbackTest::OnError(int32_t errorCode)
{
    cout << "VEnc Error errorCode=" << errorCode << endl;
}

void VEncCallbackTest::OnStreamChanged(std::shared_ptr<FormatMock> format)
{
    cout << "VEnc Format Changed" << endl;
}

void VEncCallbackTest::OnNeedInputData(uint32_t index, std::shared_ptr<AVMemoryMock> data)
{
}

void VEncCallbackTest::OnNewOutputData(uint32_t index, std::shared_ptr<AVMemoryMock> data, AVCodecBufferAttrMock attr)
{
    unique_lock<mutex> lock(signal_->outMutex_);
    signal_->outIndexQueue_.push(index);
    signal_->outSizeQueue_.push(attr.size);
    signal_->outBufferQueue_.push(data);
    signal_->outCond_.notify_all();
}

VEncMock::VEncMock(std::shared_ptr<VEncSignal> signal)
    : signal_(signal)
{
}

VEncMock::~VEncMock()
{
}

bool VEncMock::CreateVideoEncMockByMine(const std::string &mime)
{
    videoEnc_ = AVCodecMockFactory::CreateVideoEncMockByMine(mime);
    return videoEnc_ != nullptr;
}

bool VEncMock::CreateVideoEncMockByName(const std::string &name)
{
    videoEnc_ = AVCodecMockFactory::CreateVideoEncMockByName(name);
    return videoEnc_ != nullptr;
}

int32_t VEncMock::SetCallback(std::shared_ptr<AVCodecCallbackMock> cb)
{
    return videoEnc_->SetCallback(cb);
}

std::shared_ptr<SurfaceMock> VEncMock::GetInputSurface()
{
    surface_ = videoEnc_->GetInputSurface();
    return surface_;
}

int32_t VEncMock::Configure(std::shared_ptr<FormatMock> format)
{
    return videoEnc_->Configure(format);
}

int32_t VEncMock::Prepare()
{
    return videoEnc_->Prepare();
}

int32_t VEncMock::Start()
{
    isRunning_.store(true);
    outLoop_ = make_unique<thread>(&VEncMock::OutLoopFunc, this);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(outLoop_ != nullptr, MSERR_UNKNOWN, "Fatal: No memory");
    return videoEnc_->Start();
}

int32_t VEncMock::Stop()
{
    isRunning_.store(false);
    if (outLoop_ != nullptr && outLoop_->joinable()) {
        unique_lock<mutex> queueLock(signal_->mutex_);
        signal_->outIndexQueue_.push(10000); // push 10000 to stop queue
        signal_->outCond_.notify_all();
        queueLock.unlock();
        outLoop_->join();
        outLoop_.reset();
    }
    return videoEnc_->Stop();
}

int32_t VEncMock::Flush()
{
    return videoEnc_->Flush();
}

int32_t VEncMock::Reset()
{
    isRunning_.store(false);
    if (outLoop_ != nullptr && outLoop_->joinable()) {
        unique_lock<mutex> queueLock(signal_->outMutex_);
        signal_->outIndexQueue_.push(10000); // push 10000 to stop queue
        signal_->outCond_.notify_all();
        queueLock.unlock();
        outLoop_->join();
        outLoop_.reset();
    }
    return videoEnc_->Reset();
}

int32_t VEncMock::Release()
{
    return videoEnc_->Release();
}

int32_t VEncMock::NotifyEos()
{
    return videoEnc_->NotifyEos();
}

std::shared_ptr<FormatMock> VEncMock::GetOutputMediaDescription()
{
    return videoEnc_->GetOutputMediaDescription();
}

int32_t VEncMock::SetParameter(std::shared_ptr<FormatMock> format)
{
    return videoEnc_->SetParameter(format);
}

int32_t VEncMock::FreeOutputData(uint32_t index)
{
    return videoEnc_->FreeOutputData(index);
}

void VEncMock::SetOutPath(const std::string &path)
{
    outPath_ = path;
}

void VEncMock::OutLoopFunc()
{
    while (true) {
        if (!isRunning_.load()) {
            break;
        }

        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this]() { return signal_->outIndexQueue_.size() > 0; });

        if (!isRunning_.load()) {
            break;
        }

        uint32_t index = signal_->outIndexQueue_.front();
        auto buffer = signal_->outBufferQueue_.front();
        uint32_t size = signal_->outSizeQueue_.front();

        if (buffer == nullptr) {
            cout << "Fatal: GetOutputBuffer fail, exit" << endl;
            break;
        }
        cout << "GetOutputBuffer size : " << size << " frameCount_ =  " << frameCount_ << endl;
        frameCount_++;

        if (NEED_DUMP) {
            FILE *outFile;
            const char *savepath = outPath_.c_str();
            outFile = fopen(savepath, "a");
            if (outFile == nullptr) {
                cout << "dump data fail" << endl;
            } else {
                fwrite(buffer->GetAddr(), 1, size, outFile);
            }
            fclose(outFile);
        }

        if (videoEnc_->FreeOutputData(index) != MSERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail, exit" << endl;
            break;
        }
        signal_->outIndexQueue_.pop();
        signal_->outSizeQueue_.pop();
        signal_->outBufferQueue_.pop();
    }
}
}  // namespace Media
}  // namespace OHOS
