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

#ifndef AVCODECLIST_ENGINE_GST_IMPL_H
#define AVCODECLIST_ENGINE_GST_IMPL_H

#include <mutex>
#include "nocopyable.h"
#include "i_avcodeclist_engine.h"

namespace OHOS {
namespace Media {
class __attribute__((visibility("default"))) AVCodecListEngineGstImpl : public IAVCodecListEngine, public NoCopyable {
public:
    AVCodecListEngineGstImpl();
    ~AVCodecListEngineGstImpl();
    std::string FindVideoDecoder(const Format &format) override;
    std::string FindVideoEncoder(const Format &format) override;
    std::string FindAudioDecoder(const Format &format) override;
    std::string FindAudioEncoder(const Format &format) override;
    std::vector<CapabilityData> GetCodecCapabilityInfos() override;

private:
    std::string FindTargetCodec(const Format &format,
    const std::vector<CapabilityData> &capabilityDataArray, const AVCodecType &codecType);
    bool IsSupportMimeType(const Format &format, const CapabilityData &data);
    bool IsSupportBitrate(const Format &format, const CapabilityData &data);
    bool IsSupportSize(const Format &format, const CapabilityData &data);
    bool IsSupportPixelFormat(const Format &format, const CapabilityData &data);
    bool IsSupportFrameRate(const Format &format, const CapabilityData &data);
    bool IsSupportSampleRate(const Format &format, const CapabilityData &data);
    bool IsSupportChannel(const Format &format, const CapabilityData &data);
    std::mutex mutex_;
};
} // namespace Media
} // namespace OHOS
#endif // AVCODECLIST_ENGINE_GST_IMPL_H
