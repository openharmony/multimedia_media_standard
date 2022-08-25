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

#ifndef AVCODEC_AUDIO_ENCODER_IMPL_H
#define AVCODEC_AUDIO_ENCODER_IMPL_H

#include "avcodec_audio_encoder.h"
#include "nocopyable.h"
#include "i_avcodec_service.h"

namespace OHOS {
namespace Media {
class AVCodecAudioEncoderImpl : public AVCodecAudioEncoder, public NoCopyable {
public:
    AVCodecAudioEncoderImpl();
    ~AVCodecAudioEncoderImpl();

    int32_t Configure(const Format &format) override;
    int32_t Prepare() override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t Flush() override;
    int32_t Reset() override;
    int32_t Release() override;
    std::shared_ptr<AVSharedMemory> GetInputBuffer(uint32_t index) override;
    int32_t QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) override;
    std::shared_ptr<AVSharedMemory> GetOutputBuffer(uint32_t index) override;
    int32_t GetOutputFormat(Format &format) override;
    int32_t ReleaseOutputBuffer(uint32_t index) override;
    int32_t SetParameter(const Format &format) override;
    int32_t SetCallback(const std::shared_ptr<AVCodecCallback> &callback) override;
    int32_t Init(AVCodecType type, bool isMimeType, const std::string &name);

private:
    std::shared_ptr<IAVCodecService> codecService_ = nullptr;
};
} // namespace Media
} // namespace OHOS
#endif // AVCODEC_AUDIO_ENCODER_IMPL_H