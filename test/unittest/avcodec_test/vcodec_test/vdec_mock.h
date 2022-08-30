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

#ifndef VDEC_MOCK_H
#define VDEC_MOCK_H
#include <iostream>
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
struct VDecSignal {
public:
    std::mutex inMutex_;
    std::mutex outMutex_;
    std::condition_variable inCond_;
    std::condition_variable outCond_;
    std::queue<uint32_t> inIndexQueue_;
    std::queue<uint32_t> outIndexQueue_;
    std::queue<uint32_t> inSizeQueue_;
    std::queue<uint32_t>  outSizeQueue_;
    std::queue<std::shared_ptr<AVMemoryMock>> inBufferQueue_;
    std::queue<std::shared_ptr<AVMemoryMock>> outBufferQueue_;
    std::atomic<bool> isRunning_ = false;
};

class VDecCallbackTest : public AVCodecCallbackMock {
public:
    explicit VDecCallbackTest(std::shared_ptr<VDecSignal> signal);
    virtual ~VDecCallbackTest();
    void OnError(int32_t errorCode) override;
    void OnStreamChanged(std::shared_ptr<FormatMock> format) override;
    void OnNeedInputData(uint32_t index, std::shared_ptr<AVMemoryMock> data) override;
    void OnNewOutputData(uint32_t index, std::shared_ptr<AVMemoryMock> data, AVCodecBufferAttrMock attr) override;
private:
    std::shared_ptr<VDecSignal> signal_ = nullptr;
};

class VDecMock : public NoCopyable {
public:
    explicit VDecMock(std::shared_ptr<VDecSignal> signal);
    virtual ~VDecMock();
    bool CreateVideoDecMockByMime(const std::string &mime);
    bool CreateVideoDecMockByName(const std::string &name);
    int32_t SetCallback(std::shared_ptr<AVCodecCallbackMock> cb);
    int32_t SetOutputSurface(std::shared_ptr<SurfaceMock> surface);
    int32_t Configure(std::shared_ptr<FormatMock> format);
    int32_t Prepare();
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t Release();
    std::shared_ptr<FormatMock> GetOutputMediaDescription();
    int32_t SetParameter(std::shared_ptr<FormatMock> format);
    int32_t PushInputData(uint32_t index, AVCodecBufferAttrMock &attr);
    int32_t RenderOutputData(uint32_t index);
    int32_t FreeOutputData(uint32_t index);
    void SetSource(const std::string &path, const uint32_t es[], const uint32_t &size);
private:
    void FlushInner();
    std::unique_ptr<std::ifstream> testFile_;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    std::shared_ptr<VideoDecMock> videoDec_ = nullptr;
    std::shared_ptr<VDecSignal> signal_ = nullptr;
    int32_t PushInputDataMock(uint32_t index, uint32_t bufferSize);
    void InpLoopFunc();
    void OutLoopFunc();
    bool isFirstFrame_ = true;
    int64_t timestamp_ = 0;
    uint32_t frameCount_ = 0;
    const uint32_t *es_;
    uint32_t esLength_;
    std::string inpPath_;
};
}  // namespace Media
}  // namespace OHOS
#endif // VDEC_MOCK_H