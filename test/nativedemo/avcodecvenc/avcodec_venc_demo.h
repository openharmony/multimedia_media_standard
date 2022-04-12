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

#ifndef AVCODEC_VENC_DEMO_H
#define AVCODEC_VENC_DEMO_H

#include <atomic>
#include <thread>
#include <queue>
#include <string>
#include "avcodec_video_encoder.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class VEncSignal {
public:
    std::mutex mutex_;
    std::condition_variable cond_;
    std::queue<uint32_t> bufferQueue_;
};

class VEncDemoCallback : public AVCodecCallback, public NoCopyable {
public:
    explicit VEncDemoCallback(std::shared_ptr<VEncSignal> signal);
    virtual ~VEncDemoCallback() = default;

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format &format) override;
    void OnInputBufferAvailable(uint32_t index) override;
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) override;

private:
    std::shared_ptr<VEncSignal> signal_;
};

class VEncDemo : public NoCopyable {
public:
    VEncDemo() = default;
    virtual ~VEncDemo() = default;

    void RunCase(bool enableProp);
    void GenerateData(uint32_t count, uint32_t fps);

private:
    int32_t CreateVenc();
    int32_t Configure(const Format &format);
    int32_t Prepare();
    int32_t Start();
    int32_t SetParameter(int32_t suspend, int32_t maxFps, int32_t repeatMs);
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t Release();
    sptr<Surface> GetVideoSurface();
    void LoopFunc();

    std::atomic<bool> isRunning_ = false;
    std::unique_ptr<std::thread> readLoop_;
    std::shared_ptr<AVCodecVideoEncoder> venc_;
    std::shared_ptr<VEncSignal> signal_;
    std::shared_ptr<VEncDemoCallback> cb_;
    sptr<Surface> surface_ = nullptr;
    int64_t timestampNs_ = 0;
};
} // namespace Media
} // namespace OHOS
#endif // AVCODEC_VENC_DEMO_H
