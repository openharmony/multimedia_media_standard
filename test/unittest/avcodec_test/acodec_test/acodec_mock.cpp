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

#include "acodec_mock.h"
#include <sync_fence.h>
#include "avcodec_common.h"
#include "media_errors.h"

using namespace std;
using namespace OHOS::Media::ACodecTestParam;

namespace OHOS {
namespace Media {
ADecCallbackTest::ADecCallbackTest(std::shared_ptr<ACodecSignal> signal)
    : acodecSignal_(signal)
{
}

ADecCallbackTest::~ADecCallbackTest()
{
}

void ADecCallbackTest::OnError(int32_t errorCode)
{
    cout << "DEC Error errorCode=" << errorCode << endl;
    if (acodecSignal_ == nullptr) {
        return;
    }
    acodecSignal_->errorNum_ += 1;
}

void ADecCallbackTest::OnStreamChanged(std::shared_ptr<FormatMock> format)
{
    cout << "DEC Format Changed" << endl;
}

void ADecCallbackTest::OnNeedInputData(uint32_t index, std::shared_ptr<AVMemoryMock> data)
{
    if (acodecSignal_ == nullptr) {
        return;
    }
    unique_lock<mutex> lock(acodecSignal_->inMutexDec_);
    if (acodecSignal_->isFlushing_.load()) {
        return;
    }
    acodecSignal_->inQueueDec_.push(index);
    acodecSignal_->inBufferQueueDec_.push(data);
    acodecSignal_->inCondDec_.notify_all();
}

void ADecCallbackTest::OnNewOutputData(uint32_t index, std::shared_ptr<AVMemoryMock> data, AVCodecBufferAttrMock attr)
{
    if (acodecSignal_ == nullptr) {
        return;
    }
    unique_lock<mutex> lock(acodecSignal_->inMutexEnc_);
    if (acodecSignal_->isFlushing_.load()) {
        cout << "DEC OutputAvailable: isFlushing_.load() is true, return" << endl;
        return;
    }
    acodecSignal_->outQueueDec_.push(index);
    acodecSignal_->sizeQueueDec_.push(attr.size);
    acodecSignal_->flagQueueDec_.push(attr.flags);
    acodecSignal_->outBufferQueueDec_.push(data);
    acodecSignal_->inCondEnc_.notify_all();
}

AEncCallbackTest::AEncCallbackTest(std::shared_ptr<ACodecSignal> signal)
    : acodecSignal_(signal)
{
}

AEncCallbackTest::~AEncCallbackTest()
{
}

void AEncCallbackTest::OnError(int32_t errorCode)
{
    cout << "ENC Error errorCode=" << errorCode << endl;
}

void AEncCallbackTest::OnStreamChanged(std::shared_ptr<FormatMock> format)
{
    cout << "ENC Format Changed" << endl;
}

void AEncCallbackTest::OnNeedInputData(uint32_t index, std::shared_ptr<AVMemoryMock> data)
{
    if (acodecSignal_ == nullptr) {
        return;
    }
    unique_lock<mutex> lock(acodecSignal_->inMutexEnc_);
    if (acodecSignal_->isFlushing_.load()) {
        return;
    }
    acodecSignal_->inQueueEnc_.push(index);
    acodecSignal_->inBufferQueueEnc_.push(data);
    acodecSignal_->inCondEnc_.notify_all();
}

void AEncCallbackTest::OnNewOutputData(uint32_t index, std::shared_ptr<AVMemoryMock> data, AVCodecBufferAttrMock attr)
{
    if (acodecSignal_ == nullptr) {
        return;
    }
    unique_lock<mutex> lock(acodecSignal_->outMutexEnc_);
    if (acodecSignal_->isFlushing_.load()) {
        return;
    }
    acodecSignal_->outQueueEnc_.push(index);
    acodecSignal_->sizeQueueEnc_.push(attr.size);
    acodecSignal_->flagQueueEnc_.push(attr.flags);
    acodecSignal_->outBufferQueueEnc_.push(data);
    acodecSignal_->outCondEnc_.notify_all();
}

void ACodecMock::clearIntQueue (std::queue<uint32_t>& q)
{
    std::queue<uint32_t> empty;
    swap(empty, q);
}

void ACodecMock::clearBufferQueue (std::queue<std::shared_ptr<AVMemoryMock>>& q)
{
    std::queue<std::shared_ptr<AVMemoryMock>> empty;
    swap(empty, q);
}

ACodecMock::ACodecMock(std::shared_ptr<ACodecSignal> signal)
    : acodecSignal_(signal)
{
}

ACodecMock::~ACodecMock()
{
}

bool ACodecMock::CreateAudioDecMockByMime(const std::string &mime)
{
    audioDec_ = AVCodecMockFactory::CreateAudioDecMockByMime(mime);
    return audioDec_ != nullptr;
}

bool ACodecMock::CreateAudioDecMockByName(const std::string &name)
{
    audioDec_ = AVCodecMockFactory::CreateAudioDecMockByName(name);
    return audioDec_ != nullptr;
}

int32_t ACodecMock::SetCallbackDec(std::shared_ptr<AVCodecCallbackMock> cb)
{
    if (audioDec_ == nullptr) {
        return MSERR_INVALID_VAL;
    }
    return audioDec_->SetCallback(cb);
}

int32_t ACodecMock::ConfigureDec(std::shared_ptr<FormatMock> format)
{
    if (audioDec_ == nullptr) {
        return MSERR_INVALID_VAL;
    }
    return audioDec_->Configure(format);
}

int32_t ACodecMock::PrepareDec()
{
    if (audioDec_ == nullptr) {
        return MSERR_INVALID_VAL;
    }
    return audioDec_->Prepare();
}

int32_t ACodecMock::StartDec()
{
    if (audioDec_ == nullptr) {
        return MSERR_INVALID_VAL;
    }
    isDecRunning_.store(true);

    if (testFile_ == nullptr) {
        testFile_ = std::make_unique<std::ifstream>();
        UNITTEST_CHECK_AND_RETURN_RET_LOG(testFile_ != nullptr, MSERR_OK, "Fatal: No memory");
        testFile_->open("/data/test/media/AAC_48000_32_1.aac", std::ios::in | std::ios::binary);
    }
    if (inputLoopDec_ == nullptr) {
        inputLoopDec_ = make_unique<thread>(&ACodecMock::InputFuncDec, this);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(inputLoopDec_ != nullptr, MSERR_OK, "Fatal: No memory");
    }
    return audioDec_->Start();
}

int32_t ACodecMock::StopDec()
{
    if (acodecSignal_ == nullptr || audioDec_ == nullptr) {
        return MSERR_INVALID_VAL;
    }
    unique_lock<mutex> lock(acodecSignal_->inMutexDec_);
    unique_lock<mutex> lock2(acodecSignal_->inMutexEnc_);
    acodecSignal_->isFlushing_.store(true);
    lock.unlock();
    lock2.unlock();
    int32_t ret = audioDec_->Stop();
    unique_lock<mutex> lockIn(acodecSignal_->inMutexDec_);
    clearIntQueue(acodecSignal_->inQueueDec_);
    clearBufferQueue(acodecSignal_->inBufferQueueDec_);
    acodecSignal_->inCondDec_.notify_all();
    unique_lock<mutex> lockOut(acodecSignal_->inMutexEnc_);
    clearIntQueue(acodecSignal_->outQueueDec_);
    clearIntQueue(acodecSignal_->sizeQueueDec_);
    clearIntQueue(acodecSignal_->flagQueueDec_);
    clearBufferQueue(acodecSignal_->outBufferQueueDec_);
    acodecSignal_->inCondEnc_.notify_all();
    acodecSignal_->isFlushing_.store(false);
    lockIn.unlock();
    lockOut.unlock();
    return ret;
}

int32_t ACodecMock::FlushDec()
{
    if (acodecSignal_ == nullptr || audioDec_ == nullptr) {
        return MSERR_INVALID_VAL;
    }
    unique_lock<mutex> lock(acodecSignal_->inMutexDec_);
    unique_lock<mutex> lock2(acodecSignal_->inMutexEnc_);
    acodecSignal_->isFlushing_.store(true);
    lock.unlock();
    lock2.unlock();
    int32_t ret = audioDec_->Flush();
    unique_lock<mutex> lockIn(acodecSignal_->inMutexDec_);
    clearIntQueue(acodecSignal_->inQueueDec_);
    clearBufferQueue(acodecSignal_->inBufferQueueDec_);
    acodecSignal_->inCondDec_.notify_all();
    unique_lock<mutex> lockOut(acodecSignal_->inMutexEnc_);
    clearIntQueue(acodecSignal_->outQueueDec_);
    clearIntQueue(acodecSignal_->sizeQueueDec_);
    clearIntQueue(acodecSignal_->flagQueueDec_);
    clearBufferQueue(acodecSignal_->outBufferQueueDec_);
    acodecSignal_->inCondEnc_.notify_all();
    acodecSignal_->isFlushing_.store(false);
    lockIn.unlock();
    lockOut.unlock();
    return ret;
}

int32_t ACodecMock::ResetDec()
{
    if (acodecSignal_ == nullptr || audioDec_ == nullptr) {
        return MSERR_INVALID_VAL;
    }
    unique_lock<mutex> lock(acodecSignal_->inMutexDec_);
    unique_lock<mutex> lock2(acodecSignal_->inMutexEnc_);
    acodecSignal_->isFlushing_.store(true);
    lock.unlock();
    lock2.unlock();
    int32_t ret = audioDec_->Reset();
    unique_lock<mutex> lockIn(acodecSignal_->inMutexDec_);
    clearIntQueue(acodecSignal_->inQueueDec_);
    clearBufferQueue(acodecSignal_->inBufferQueueDec_);
    acodecSignal_->inCondDec_.notify_all();
    unique_lock<mutex> lockOut(acodecSignal_->inMutexEnc_);
    clearIntQueue(acodecSignal_->outQueueDec_);
    clearIntQueue(acodecSignal_->sizeQueueDec_);
    clearIntQueue(acodecSignal_->flagQueueDec_);
    clearBufferQueue(acodecSignal_->outBufferQueueDec_);
    acodecSignal_->inCondEnc_.notify_all();
    acodecSignal_->isFlushing_.store(false);
    lockIn.unlock();
    lockOut.unlock();
    return ret;
}

int32_t ACodecMock::ReleaseDec()
{
    if (acodecSignal_ == nullptr || audioDec_ == nullptr) {
        return MSERR_INVALID_VAL;
    }
    isDecRunning_.store(false);
    isEncRunning_.store(false);
    if (inputLoopDec_ != nullptr && inputLoopDec_->joinable()) {
        unique_lock<mutex> lock(acodecSignal_->inMutexDec_);
        acodecSignal_->inQueueDec_.push(10000); // push 10000 to stop queue
        acodecSignal_->inCondDec_.notify_all();
        lock.unlock();
        inputLoopDec_->join();
        inputLoopDec_.reset();
    }
    return audioDec_->Release();
}

std::shared_ptr<FormatMock> ACodecMock::GetOutputMediaDescriptionDec()
{
    if (audioDec_ == nullptr) {
        return nullptr;
    }
    return audioDec_->GetOutputMediaDescription();
}

int32_t ACodecMock::SetParameterDec(std::shared_ptr<FormatMock> format)
{
    if (audioDec_ == nullptr) {
        return MSERR_INVALID_VAL;
    }
    return audioDec_->SetParameter(format);
}

int32_t ACodecMock::PushInputDataDec(uint32_t index, AVCodecBufferAttrMock &attr)
{
    if (audioDec_ == nullptr) {
        return MSERR_INVALID_VAL;
    }
    return audioDec_->PushInputData(index, attr);
}

int32_t ACodecMock::FreeOutputDataDec(uint32_t index)
{
    if (audioDec_ == nullptr) {
        return MSERR_INVALID_VAL;
    }
    return audioDec_->FreeOutputData(index);
}

void ACodecMock::SetOutPath(const std::string &path)
{
    outPath_ = path;
}

void ACodecMock::PopOutQueueDec()
{
    if (acodecSignal_ == nullptr) {
        return;
    }
    acodecSignal_->outQueueDec_.pop();
    acodecSignal_->sizeQueueDec_.pop();
    acodecSignal_->flagQueueDec_.pop();
    acodecSignal_->outBufferQueueDec_.pop();
}

void ACodecMock::PopInQueueDec()
{
    if (acodecSignal_ == nullptr) {
        return;
    }
    acodecSignal_->inQueueDec_.pop();
    acodecSignal_->inBufferQueueDec_.pop();
}

int32_t ACodecMock::PushInputDataDecInner(uint32_t index, uint32_t bufferSize)
{
    if (audioDec_ == nullptr) {
        return MSERR_INVALID_VAL;
    }
    struct AVCodecBufferAttrMock attr;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAG_NONE;
    if (decInCnt_ == ES_LENGTH) {
        cout << "DEC input: set EOS" << endl;
        attr.flags = AVCODEC_BUFFER_FLAG_EOS;
        attr.pts = 0;
        attr.size = 0;
        attr.offset = 0;
        isDecInputEOS_ = true;
    } else {
        attr.pts = timeStampDec_;
        attr.size = bufferSize;
        attr.offset = 0;
        if (decInCnt_ == 0 && MIME_TYPE == "audio/vorbis") {
            attr.flags = AVCODEC_BUFFER_FLAG_CODEC_DATA;
        } else {
            attr.flags = AVCODEC_BUFFER_FLAG_NONE;
        }
    }
    return audioDec_->PushInputData(index, attr);
}

void ACodecMock::InputFuncDec()
{
    if (acodecSignal_ == nullptr || audioDec_ == nullptr) {
        return;
    }
    while (true) {
        if (!isDecRunning_.load()) {
            break;
        }

        unique_lock<mutex> lock(acodecSignal_->inMutexDec_);
        acodecSignal_->inCondDec_.wait(lock, [this]() { return acodecSignal_->inQueueDec_.size() > 0; });

        if (!isDecRunning_.load()) {
            break;
        }

        uint32_t index = acodecSignal_->inQueueDec_.front();
        std::shared_ptr<AVMemoryMock> buffer = acodecSignal_->inBufferQueueDec_.front();
        if (acodecSignal_->isFlushing_.load() || isDecInputEOS_ || buffer == nullptr) {
            PopInQueueDec();
            continue;
        }
        UNITTEST_CHECK_AND_RETURN_LOG(testFile_ != nullptr && testFile_->is_open(), "Fatal: open file fail");

        uint32_t bufferSize = 0; // replace with the actual size
        if (decInCnt_ < ES_LENGTH) {
            bufferSize = ES[decInCnt_];
            char *fileBuffer = (char *)malloc(sizeof(char) * bufferSize + 1);
            UNITTEST_CHECK_AND_RETURN_LOG(fileBuffer != nullptr, "Fatal: malloc fail");

            (void)testFile_->read(fileBuffer, bufferSize);
            if (testFile_->eof()) {
                free(fileBuffer);
                break;
            }
            if (memcpy_s(buffer->GetAddr(), buffer->GetSize(), fileBuffer, bufferSize) != EOK) {
                free(fileBuffer);
                PopInQueueDec();
                break;
            }
            free(fileBuffer);
        }
        if (PushInputDataDecInner(index, bufferSize) != MSERR_OK) {
            acodecSignal_->errorNum_ += 1;
        } else {
            decInCnt_ ++;
        }
        timeStampDec_ += SAMPLE_DURATION_US;
        PopInQueueDec();
    }
}

bool ACodecMock::CreateAudioEncMockByMime(const std::string &mime)
{
    audioEnc_ = AVCodecMockFactory::CreateAudioEncMockByMime(mime);
    return audioEnc_ != nullptr;
}

bool ACodecMock::CreateAudioEncMockByName(const std::string &name)
{
    audioEnc_ = AVCodecMockFactory::CreateAudioEncMockByName(name);
    return audioEnc_ != nullptr;
}

int32_t ACodecMock::SetCallbackEnc(std::shared_ptr<AVCodecCallbackMock> cb)
{
    if (audioEnc_ == nullptr) {
        return MSERR_INVALID_VAL;
    }
    return audioEnc_->SetCallback(cb);
}

int32_t ACodecMock::ConfigureEnc(std::shared_ptr<FormatMock> format)
{
    if (audioEnc_ == nullptr) {
        return MSERR_INVALID_VAL;
    }
    return audioEnc_->Configure(format);
}

int32_t ACodecMock::PrepareEnc()
{
    if (audioEnc_ == nullptr) {
        return MSERR_INVALID_VAL;
    }
    return audioEnc_->Prepare();
}

int32_t ACodecMock::StartEnc()
{
    if (audioEnc_ == nullptr) {
        return MSERR_INVALID_VAL;
    }
    
    isEncRunning_.store(true);
    if (inputLoopEnc_ == nullptr) {
        inputLoopEnc_ = make_unique<thread>(&ACodecMock::InputFuncEnc, this);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(inputLoopEnc_ != nullptr, MSERR_OK, "Fatal: No memory");
    }

    if (outputLoopEnc_ == nullptr) {
        outputLoopEnc_ = make_unique<thread>(&ACodecMock::OutputFuncEnc, this);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(outputLoopEnc_ != nullptr, MSERR_OK, "Fatal: No memory");
    }
    return audioEnc_->Start();
}

int32_t ACodecMock::StopEnc()
{
    if (acodecSignal_ == nullptr || audioEnc_ == nullptr) {
        return MSERR_INVALID_VAL;
    }
    unique_lock<mutex> lock(acodecSignal_->outMutexEnc_);
    unique_lock<mutex> lock2(acodecSignal_->inMutexEnc_);
    acodecSignal_->isFlushing_.store(true);
    lock.unlock();
    lock2.unlock();
    int32_t ret = audioEnc_->Stop();
    unique_lock<mutex> lockIn(acodecSignal_->outMutexEnc_);
    clearIntQueue(acodecSignal_->outQueueEnc_);
    clearIntQueue(acodecSignal_->sizeQueueEnc_);
    clearIntQueue(acodecSignal_->flagQueueEnc_);
    clearBufferQueue(acodecSignal_->outBufferQueueEnc_);
    acodecSignal_->outCondEnc_.notify_all();
    unique_lock<mutex> lockOut(acodecSignal_->inMutexEnc_);
    clearIntQueue(acodecSignal_->inQueueEnc_);
    clearBufferQueue(acodecSignal_->inBufferQueueEnc_);
    acodecSignal_->inCondEnc_.notify_all();
    acodecSignal_->isFlushing_.store(false);
    lockIn.unlock();
    lockOut.unlock();
    return ret;
}

int32_t ACodecMock::FlushEnc()
{
    if (acodecSignal_ == nullptr || audioEnc_ == nullptr) {
        return MSERR_INVALID_VAL;
    }
    unique_lock<mutex> lock(acodecSignal_->outMutexEnc_);
    unique_lock<mutex> lock2(acodecSignal_->inMutexEnc_);
    acodecSignal_->isFlushing_.store(true);
    lock.unlock();
    lock2.unlock();
    int32_t ret = audioEnc_->Flush();
    unique_lock<mutex> lockIn(acodecSignal_->outMutexEnc_);
    clearIntQueue(acodecSignal_->outQueueEnc_);
    clearIntQueue(acodecSignal_->sizeQueueEnc_);
    clearIntQueue(acodecSignal_->flagQueueEnc_);
    clearBufferQueue(acodecSignal_->outBufferQueueEnc_);
    acodecSignal_->outCondEnc_.notify_all();
    unique_lock<mutex> lockOut(acodecSignal_->inMutexEnc_);
    clearIntQueue(acodecSignal_->inQueueEnc_);
    clearBufferQueue(acodecSignal_->inBufferQueueEnc_);
    acodecSignal_->inCondEnc_.notify_all();
    acodecSignal_->isFlushing_.store(false);
    lockIn.unlock();
    lockOut.unlock();
    return ret;
}

int32_t ACodecMock::ResetEnc()
{
    if (acodecSignal_ == nullptr || audioEnc_ == nullptr) {
        return MSERR_INVALID_VAL;
    }
    unique_lock<mutex> lock(acodecSignal_->outMutexEnc_);
    unique_lock<mutex> lock2(acodecSignal_->inMutexEnc_);
    acodecSignal_->isFlushing_.store(true);
    lock.unlock();
    lock2.unlock();
    int32_t ret = audioEnc_->Reset();
    unique_lock<mutex> lockIn(acodecSignal_->outMutexEnc_);
    clearIntQueue(acodecSignal_->outQueueEnc_);
    clearIntQueue(acodecSignal_->sizeQueueEnc_);
    clearIntQueue(acodecSignal_->flagQueueEnc_);
    clearBufferQueue(acodecSignal_->outBufferQueueEnc_);
    acodecSignal_->outCondEnc_.notify_all();
    unique_lock<mutex> lockOut(acodecSignal_->inMutexEnc_);
    clearIntQueue(acodecSignal_->inQueueEnc_);
    clearBufferQueue(acodecSignal_->inBufferQueueEnc_);
    acodecSignal_->inCondEnc_.notify_all();
    acodecSignal_->isFlushing_.store(false);
    lockIn.unlock();
    lockOut.unlock();
    return ret;
}

int32_t ACodecMock::ReleaseEnc()
{
    if (acodecSignal_ == nullptr || audioEnc_ == nullptr) {
        return MSERR_INVALID_VAL;
    }
    isEncRunning_.store(false);
    if (inputLoopEnc_ != nullptr && inputLoopEnc_->joinable()) {
        unique_lock<mutex> lock(acodecSignal_->inMutexEnc_);
        acodecSignal_->outQueueDec_.push(10000); // push 10000 to stop queue
        acodecSignal_->inQueueEnc_.push(10000); // push 10000 to stop queue
        acodecSignal_->inCondEnc_.notify_all();
        lock.unlock();
        inputLoopEnc_->join();
        inputLoopEnc_.reset();
    }
    if (outputLoopEnc_ != nullptr && outputLoopEnc_->joinable()) {
        unique_lock<mutex> lock(acodecSignal_->outMutexEnc_);
        acodecSignal_->outQueueEnc_.push(10000); // push 10000 to stop queue
        acodecSignal_->outCondEnc_.notify_all();
        lock.unlock();
        outputLoopEnc_->join();
        outputLoopEnc_.reset();
    }
    return audioEnc_->Release();
}

std::shared_ptr<FormatMock> ACodecMock::GetOutputMediaDescriptionEnc()
{
    if (audioEnc_ == nullptr) {
        return nullptr;
    }
    return audioEnc_->GetOutputMediaDescription();
}

int32_t ACodecMock::SetParameterEnc(std::shared_ptr<FormatMock> format)
{
    if (audioEnc_ == nullptr) {
        return MSERR_INVALID_VAL;
    }
    return audioEnc_->SetParameter(format);
}

int32_t ACodecMock::PushInputDataEnc(uint32_t index, AVCodecBufferAttrMock &attr)
{
    if (audioEnc_ == nullptr) {
        return MSERR_INVALID_VAL;
    }
    return audioEnc_->PushInputData(index, attr);
}

int32_t ACodecMock::FreeOutputDataEnc(uint32_t index)
{
    if (audioEnc_ == nullptr) {
        return MSERR_INVALID_VAL;
    }
    return audioEnc_->FreeOutputData(index);
}

void ACodecMock::PopInQueueEnc()
{
    if (acodecSignal_ == nullptr) {
        return;
    }
    acodecSignal_->inQueueEnc_.pop();
    acodecSignal_->inBufferQueueEnc_.pop();
}

int32_t ACodecMock::PushInputDataEncInner()
{
    uint32_t indexEnc = acodecSignal_->inQueueEnc_.front();
    std::shared_ptr<AVMemoryMock> bufferEnc = acodecSignal_->inBufferQueueEnc_.front();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(bufferEnc != nullptr, MSERR_INVALID_VAL, "Fatal: GetEncInputBuffer fail");

    uint32_t indexDec = acodecSignal_->outQueueDec_.front();
    std::shared_ptr<AVMemoryMock> bufferDec = acodecSignal_->outBufferQueueDec_.front();
    uint32_t sizeDecOut = acodecSignal_->sizeQueueDec_.front();
    uint32_t flagDecOut = acodecSignal_->flagQueueDec_.front();
    struct AVCodecBufferAttrMock attr;
    attr.offset = 0;
    attr.size = sizeDecOut;
    attr.pts = timeStampEnc_;
    attr.flags = 0;
    if (flagDecOut == 1) {
        cout << "DEC output EOS " << endl;
        isDecOutputEOS_ = true;
        isEncInputEOS_ = true;
        attr.flags = 1;
    } else {
        if (memcpy_s(bufferEnc->GetAddr(), bufferEnc->GetSize(), bufferDec->GetAddr(), sizeDecOut) != MSERR_OK) {
            cout << "Fatal: memcpy fail" << endl;
            acodecSignal_->errorNum_ += 1;
            PopOutQueueDec();
            PopInQueueEnc();
            return MSERR_INVALID_VAL;
        }
        if (audioDec_->FreeOutputData(indexDec) != MSERR_OK) {
            cout << "Fatal: FreeOutputData fail" << endl;
            acodecSignal_->errorNum_ += 1;
        } else {
            decOutCnt_ += 1;
        }
    }

    PopOutQueueDec();
    PopInQueueEnc();
    return audioEnc_->PushInputData(indexEnc, attr);
}

void ACodecMock::InputFuncEnc()
{
    if (acodecSignal_ == nullptr || audioEnc_ == nullptr) {
        return;
    }
    while (true) {
        if (!isEncRunning_.load()) {
            break;
        }

        unique_lock<mutex> lock(acodecSignal_->inMutexEnc_);
        acodecSignal_->inCondEnc_.wait(lock, [this]() { return acodecSignal_->inQueueEnc_.size() > 0; });
        acodecSignal_->inCondEnc_.wait(lock, [this]() { return acodecSignal_->outQueueDec_.size() > 0; });

        if (!isEncRunning_.load()) {
            break;
        }
        if (acodecSignal_->isFlushing_.load() || isDecOutputEOS_) {
            PopOutQueueDec();
            PopInQueueEnc();
            continue;
        }
        if (PushInputDataEncInner() != MSERR_OK) {
            cout << "Fatal: PushInputData fail, exit" << endl;
            acodecSignal_->errorNum_ += 1;
            break;
        }
        timeStampEnc_ += SAMPLE_DURATION_US;
    }
}

void ACodecMock::OutputFuncEnc()
{
    if (acodecSignal_ == nullptr || audioEnc_ == nullptr) {
        return;
    }
    while (!isEncOutputEOS_) {
        if (!isEncRunning_.load()) {
            break;
        }

        unique_lock<mutex> lock(acodecSignal_->outMutexEnc_);
        acodecSignal_->outCondEnc_.wait(lock, [this]() { return acodecSignal_->outQueueEnc_.size() > 0; });

        if (!isEncRunning_.load()) {
            break;
        }
        if (acodecSignal_->isFlushing_.load() || isEncOutputEOS_) {
            acodecSignal_->outQueueEnc_.pop();
            acodecSignal_->sizeQueueEnc_.pop();
            acodecSignal_->flagQueueEnc_.pop();
            acodecSignal_->outBufferQueueEnc_.pop();
            continue;
        }
        uint32_t index = acodecSignal_->outQueueEnc_.front();
        auto buffer = acodecSignal_->outBufferQueueEnc_.front();
        uint32_t size = acodecSignal_->sizeQueueEnc_.front();
        uint32_t encOutflag = acodecSignal_->flagQueueEnc_.front();
        if (encOutflag == 1) {
            cout << "ENC get output EOS" << endl;
            isEncOutputEOS_ = true;
        } else {
            FILE *outFile;
            const char *savepath = outPath_.c_str();
            outFile = fopen(savepath, "a");
            if (outFile == nullptr) {
                cout << "dump data fail" << endl;
            } else {
                fwrite(buffer->GetAddr(), 1, size, outFile);
            }
            fclose(outFile);

            if (audioEnc_->FreeOutputData(index) != MSERR_OK) {
                cout << "Fatal: FreeOutputData fail" << endl;
                acodecSignal_->errorNum_ += 1;
            }
        }
        acodecSignal_->outQueueEnc_.pop();
        acodecSignal_->sizeQueueEnc_.pop();
        acodecSignal_->flagQueueEnc_.pop();
        acodecSignal_->outBufferQueueEnc_.pop();
    }
}
}
}

