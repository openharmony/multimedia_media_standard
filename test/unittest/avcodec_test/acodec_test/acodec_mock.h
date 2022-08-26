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

#ifndef ACODEC_MOCK_H
#define ACODEC_MOCK_H

#include <iostream>
#include <atomic>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>
#include <string>
#include "avcodec_mock.h"
#include "unittest_log.h"
#include "test_params_config.h"
#include "securec.h"

namespace OHOS {
namespace Media {
struct ACodecSignal {
public:
    std::mutex inMutexDec_;
    std::condition_variable inCondDec_;
    std::queue<uint32_t> inQueueDec_;
    std::queue<uint32_t> outQueueDec_;
    std::queue<uint32_t>  sizeQueueDec_;
    std::queue<uint32_t>  flagQueueDec_;
    std::queue<std::shared_ptr<AVMemoryMock>> inBufferQueueDec_;
    std::queue<std::shared_ptr<AVMemoryMock>> outBufferQueueDec_;

    std::mutex inMutexEnc_;
    std::mutex outMutexEnc_;
    std::condition_variable inCondEnc_;
    std::condition_variable outCondEnc_;
    std::queue<uint32_t> inQueueEnc_;
    std::queue<uint32_t> outQueueEnc_;
    std::queue<uint32_t>  sizeQueueEnc_;
    std::queue<uint32_t>  flagQueueEnc_;
    std::queue<std::shared_ptr<AVMemoryMock>> inBufferQueueEnc_;
    std::queue<std::shared_ptr<AVMemoryMock>> outBufferQueueEnc_;
    int32_t errorNum_ = 0;
    std::atomic<bool> isFlushing_ = false;
};

class ADecCallbackTest : public AVCodecCallbackMock {
public:
    explicit ADecCallbackTest(std::shared_ptr<ACodecSignal> signal);
    virtual ~ADecCallbackTest();
    void OnError(int32_t errorCode);
    void OnStreamChanged(std::shared_ptr<FormatMock> format);
    void OnNeedInputData(uint32_t index, std::shared_ptr<AVMemoryMock> data);
    void OnNewOutputData(uint32_t index, std::shared_ptr<AVMemoryMock> data, AVCodecBufferAttrMock attr);
private:
    std::shared_ptr<ACodecSignal> acodecSignal_;
};

class AEncCallbackTest : public AVCodecCallbackMock {
public:
    explicit AEncCallbackTest(std::shared_ptr<ACodecSignal> signal);
    virtual ~AEncCallbackTest();
    void OnError(int32_t errorCode);
    void OnStreamChanged(std::shared_ptr<FormatMock> format);
    void OnNeedInputData(uint32_t index, std::shared_ptr<AVMemoryMock> data);
    void OnNewOutputData(uint32_t index, std::shared_ptr<AVMemoryMock> data, AVCodecBufferAttrMock attr);
private:
    std::shared_ptr<ACodecSignal> acodecSignal_;
};

class ACodecMock : public NoCopyable {
public:
    ACodecMock(std::shared_ptr<ACodecSignal> signal);
    ~ACodecMock();

    bool CreateAudioDecMockByMime(const std::string &mime);
    bool CreateAudioDecMockByName(const std::string &name);
    int32_t SetCallbackDec(std::shared_ptr<AVCodecCallbackMock> cb);
    int32_t ConfigureDec(std::shared_ptr<FormatMock> format);
    int32_t PrepareDec();
    int32_t StartDec();
    int32_t StopDec();
    int32_t FlushDec();
    int32_t ResetDec();
    int32_t ReleaseDec();
    std::shared_ptr<FormatMock> GetOutputMediaDescriptionDec();
    int32_t SetParameterDec(std::shared_ptr<FormatMock> format);
    int32_t PushInputDataDec(uint32_t index, AVCodecBufferAttrMock &attr);
    int32_t FreeOutputDataDec(uint32_t index);
    bool CreateAudioEncMockByMime(const std::string &mime);
    bool CreateAudioEncMockByName(const std::string &name);
    int32_t SetCallbackEnc(std::shared_ptr<AVCodecCallbackMock> cb);
    int32_t ConfigureEnc(std::shared_ptr<FormatMock> format);
    int32_t PrepareEnc();
    int32_t StartEnc();
    int32_t StopEnc();
    int32_t FlushEnc();
    int32_t ResetEnc();
    int32_t ReleaseEnc();
    int32_t PushInputDataEnc(uint32_t index, AVCodecBufferAttrMock &attr);
    std::shared_ptr<FormatMock> GetOutputMediaDescriptionEnc();
    int32_t SetParameterEnc(std::shared_ptr<FormatMock> format);
    int32_t FreeOutputDataEnc(uint32_t index);
    void InputLoopFunc();
    void OutputLoopFunc();
    void SetOutPath(const std::string &path);

private:
    void clearIntQueue (std::queue<uint32_t>& q);
    void clearBufferQueue (std::queue<std::shared_ptr<AVMemoryMock>>& q);
    std::shared_ptr<AudioDecMock> audioDec_;
    std::shared_ptr<ACodecSignal> acodecSignal_;
    void InputFuncDec();
    void PopOutQueueDec();
    void PopInQueueDec();
    int32_t PushInputDataDecInner(uint32_t index, uint32_t bufferSize);
    std::atomic<bool> isDecRunning_ = false;
    std::unique_ptr<std::ifstream> testFile_;
    std::unique_ptr<std::thread> inputLoopDec_;
    std::unique_ptr<std::thread> outputLoopDec_;
    int64_t timeStampDec_ = 0;
    uint32_t decInCnt_ = 0;
    uint32_t decOutCnt_ = 0;

    std::shared_ptr<AudioEncMock> audioEnc_ = nullptr;
    void InputFuncEnc();
    void OutputFuncEnc();
    int32_t PushInputDataEncInner();
    void PopInQueueEnc();
    std::atomic<bool> isEncRunning_ = false;
    std::unique_ptr<std::thread> inputLoopEnc_;
    std::unique_ptr<std::thread> outputLoopEnc_;
    bool isDecInputEOS_ = false;
    bool isEncInputEOS_ = false;
    bool isDecOutputEOS_ = false;
    bool isEncOutputEOS_ = false;
    int64_t timeStampEnc_ = 0;
    std::string outPath_ = "/data/test/media/out.es";
};
}
}
#endif // ACODEC_MOCK_H
