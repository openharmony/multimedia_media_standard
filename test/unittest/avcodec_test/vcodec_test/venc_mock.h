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

#ifndef VENC_MOCK_H
#define VENC_MOCK_H
#include <iostream>
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>
#include <string>
#include "avcodec_mock.h"
#include "test_params_config.h"
#include "unittest_log.h"
#include "securec.h"
namespace OHOS {
namespace Media {
struct VEncSignal {
public:
    std::mutex mutex_;
    std::mutex outMutex_;
    std::condition_variable outCond_;
    std::queue<std::shared_ptr<AVMemoryMock>> outBufferQueue_;
    std::queue<uint32_t> outIndexQueue_;
    std::queue<uint32_t> outSizeQueue_;
    std::atomic<bool> isRunning_ = false;
};

class VEncCallbackTest : public AVCodecCallbackMock {
public:
    explicit VEncCallbackTest(std::shared_ptr<VEncSignal> signal);
    virtual ~VEncCallbackTest();
    void OnError(int32_t errorCode) override;
    void OnStreamChanged(std::shared_ptr<FormatMock> format) override;
    void OnNeedInputData(uint32_t index, std::shared_ptr<AVMemoryMock> data) override;
    void OnNewOutputData(uint32_t index, std::shared_ptr<AVMemoryMock> data, AVCodecBufferAttrMock attr) override;
private:
    std::shared_ptr<VEncSignal> signal_;
};

class VEncMock : public NoCopyable {
public:
    explicit VEncMock(std::shared_ptr<VEncSignal> signal);
    virtual ~VEncMock();
    bool CreateVideoEncMockByMime(const std::string &mime);
    bool CreateVideoEncMockByName(const std::string &name);
    int32_t SetCallback(std::shared_ptr<AVCodecCallbackMock> cb);
    std::shared_ptr<SurfaceMock> GetInputSurface();
    int32_t Configure(std::shared_ptr<FormatMock> format);
    int32_t Prepare();
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t Release();
    int32_t NotifyEos();
    std::shared_ptr<FormatMock> GetOutputMediaDescription();
    int32_t SetParameter(std::shared_ptr<FormatMock> format);
    int32_t FreeOutputData(uint32_t index);
    void SetOutPath(const std::string &path);
    void OutLoopFunc();
private:
    void FlushInner();
    std::unique_ptr<std::thread> outLoop_;
    std::shared_ptr<VideoEncMock> videoEnc_ = nullptr;
    std::shared_ptr<VEncSignal> signal_ = nullptr;
    std::shared_ptr<SurfaceMock> surface_ = nullptr;
    uint32_t frameCount_ = 0;
    std::string outPath_ = "/data/test/media/vout.es";
};
}  // namespace Media
}  // namespace OHOS
#endif // VENC_MOCK_H