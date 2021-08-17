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

#ifndef VIDEO_CAPTURE_STUB_IMPL_H
#define VIDEO_CAPTURE_STUB_IMPL_H

#include <fstream>
#include "video_capture.h"

namespace OHOS {
namespace Media {
class VideoCaptureStubImpl : public VideoCapture {
public:
    VideoCaptureStubImpl();
    virtual ~VideoCaptureStubImpl();

    int32_t Prepare() override;
    int32_t Start() override;
    int32_t Pause() override;
    int32_t Resume() override;
    int32_t Stop() override;
    int32_t SetSurfaceWidth(uint32_t width) override;
    int32_t SetSurfaceHeight(uint32_t height) override;
    sptr<Surface> GetSurface() override;
    std::shared_ptr<EsAvcCodecBuffer> GetCodecBuffer() override;
    std::shared_ptr<VideoFrameBuffer> GetFrameBuffer() override;

private:
    void SetSurfaceUserData();
    void ProcessSysProperty();
    std::shared_ptr<VideoFrameBuffer> GetFirstBuffer();
    const uint8_t *FindNextNal(const uint8_t *start, const uint8_t *end, uint32_t &nalLen);
    void GetCodecData(const uint8_t *data, int32_t len, std::vector<uint8_t> &sps, std::vector<uint8_t> &pps,
                      uint32_t &nalSize);
    class ConsumerListenerProxy : public IBufferConsumerListener {
    public:
        ConsumerListenerProxy(VideoCaptureStubImpl &owner) : owner_(owner) {}
        ~ConsumerListenerProxy() = default;
        void OnBufferAvailable() override;
    private:
        VideoCaptureStubImpl &owner_;
    };
    void OnBufferAvailable();

    uint32_t surfaceWidth_;
    uint32_t surfaceHeight_;
    std::unique_ptr<std::ifstream> testFile_;
    int32_t frameSequence_;
    char *codecData_;
    uint32_t codecDataSize_;
    uint32_t pts_;
    sptr<Surface> dataConSurface_;
    sptr<Surface> producerSurface_;
    uint32_t frameWidth_;
    uint32_t frameHeight_;
    std::string filePath_;
    const int32_t *frameLenArray_;
    uint32_t nalSize_;
};
}  // namespace Media
}  // namespace OHOS
#endif // VIDEO_CAPTURE_STUB_IMPL_H
