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
#ifndef AVCODEC_LIST_IMPL_H
#define AVCODEC_LIST_IMPL_H

#include "avcodec_info.h"
#include "avcodec_list.h"
#include "nocopyable.h"
#include "i_avcodeclist_service.h"

namespace OHOS {
namespace Media {
class AVCodecListImpl : public AVCodecList, public NoCopyable {
public:
    AVCodecListImpl();
    ~AVCodecListImpl();

    std::string FindVideoDecoder(const Format &format) override;
    std::string FindVideoEncoder(const Format &format) override;
    std::string FindAudioDecoder(const Format &format) override;
    std::string FindAudioEncoder(const Format &format) override;
    std::vector<std::shared_ptr<VideoCaps>> GetVideoDecoderCaps() override;
    std::vector<std::shared_ptr<VideoCaps>> GetVideoEncoderCaps() override;
    std::vector<std::shared_ptr<AudioCaps>> GetAudioDecoderCaps() override;
    std::vector<std::shared_ptr<AudioCaps>> GetAudioEncoderCaps() override;
    std::vector<CapabilityData> GetCodecCapabilityInfos();
    int32_t Init();

private:
    std::vector<CapabilityData> capabilityArray_;
    std::vector<std::shared_ptr<VideoCaps>> videoDecoderCapsArray_;
    std::vector<std::shared_ptr<VideoCaps>> videoEncoderCapsArray_;
    std::vector<std::shared_ptr<AudioCaps>> audioDecoderCapsArray_;
    std::vector<std::shared_ptr<AudioCaps>> audioEncoderCapsArray_;
    std::shared_ptr<IAVCodecListService> codecListService_ = nullptr;
    std::vector<CapabilityData> SelectTargetCapabilityDataArray(const AVCodecType &codecType) const;
};
} // namespace Media
} // namespace OHOS
#endif // AVCODEC_LIST_IMPL_H

