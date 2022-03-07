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

#ifndef AUDIO_SINK_SV_IMPL_H
#define AUDIO_SINK_SV_IMPL_H

#include "audio_sink.h"
#include "audio_renderer.h"
#include "audio_system_manager.h"
#include "audio_errors.h"

namespace OHOS {
namespace Media {
class AudioSinkSvImpl : public AudioSink {
public:
    AudioSinkSvImpl();
    virtual ~AudioSinkSvImpl();

    GstCaps *GetCaps() override;
    int32_t SetVolume(float volume) override;
    int32_t GetVolume(float &volume) override;
    int32_t GetMaxVolume(float &volume) override;
    int32_t GetMinVolume(float &volume) override;
    int32_t Prepare() override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t Pause() override;
    int32_t Drain() override;
    int32_t Flush() override;
    int32_t Release() override;
    int32_t SetParameters(uint32_t bitsPerSample, uint32_t channels, uint32_t sampleRate) override;
    int32_t GetParameters(uint32_t &bitsPerSample, uint32_t &channels, uint32_t &sampleRate) override;
    int32_t GetMinimumBufferSize(uint32_t &bufferSize) override;
    int32_t GetMinimumFrameCount(uint32_t &frameCount) override;
    int32_t Write(uint8_t *buffer, size_t size) override;
    int32_t GetAudioTime(uint64_t &time) override;
    int32_t GetLatency(uint64_t &latency) const override;
    int32_t SetParameter(int32_t &param) override;
    bool Writeable() const override;

private:
    std::unique_ptr<OHOS::AudioStandard::AudioRenderer> audioRenderer_;
    void InitChannelRange(GstCaps *caps) const;
    void InitRateRange(GstCaps *caps) const;
};
} // namespace Media
} // namespace OHOS
#endif // AUDIO_SINK_SV_IMPL_H
