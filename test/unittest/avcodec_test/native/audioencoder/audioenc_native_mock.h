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

#ifndef AUDIO_ENC_NATIVE_MOCK_H
#define AUDIO_ENC_NATIVE_MOCK_H

#include "avcodec_mock.h"
#include "avcodec_common.h"
#include "avcodec_audio_encoder.h"

namespace OHOS {
namespace Media {
class AudioEncNativeMock : public AudioEncMock {
public:
    explicit AudioEncNativeMock(std::shared_ptr<AVCodecAudioEncoder> audioEnc) : audioEnc_(audioEnc) {}
    ~AudioEncNativeMock() = default;
    int32_t SetCallback(std::shared_ptr<AVCodecCallbackMock> cb) override;
    int32_t Configure(std::shared_ptr<FormatMock> format) override;
    int32_t Prepare() override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t Flush() override;
    int32_t Reset() override;
    int32_t Release() override;
    int32_t PushInputData(uint32_t index, AVCodecBufferAttrMock &attr) override;
    std::shared_ptr<FormatMock> GetOutputMediaDescription() override;
    int32_t SetParameter(std::shared_ptr<FormatMock> format) override;
    int32_t FreeOutputData(uint32_t index) override;

private:
    std::shared_ptr<AVCodecAudioEncoder> audioEnc_ = nullptr;
};

class AudioEncCallbackMock : public AVCodecCallback {
public:
    AudioEncCallbackMock(std::shared_ptr<AVCodecCallbackMock> cb, std::weak_ptr<AVCodecAudioEncoder> ae);
    ~AudioEncCallbackMock() = default;
    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format &format) override;
    void OnInputBufferAvailable(uint32_t index) override;
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) override;

private:
    std::shared_ptr<AVCodecCallbackMock> mockCb_ = nullptr;
    std::weak_ptr<AVCodecAudioEncoder> audioEnc_;
};
} // namespace Media
} // namespace OHOS
#endif // AUDIO_ENC_NATIVE_MOCK_H