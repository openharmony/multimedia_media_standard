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

#ifndef AVCODEC_LIST_NATIVE_MOCK_H
#define AVCODEC_LIST_NATIVE_MOCK_H

#include "avcodec_mock.h"
#include "avcodec_list.h"

namespace OHOS {
namespace Media {
class AVCodecListNativeMock : public AVCodecListMock {
public:
    explicit AVCodecListNativeMock(std::shared_ptr<AVCodecList> avCodecList) : avCodecList_(avCodecList) {}
    AVCodecListNativeMock() = default;
    std::string FindVideoDecoder(std::shared_ptr<FormatMock> format) const override;
    std::string FindVideoEncoder(std::shared_ptr<FormatMock> format) const override;
    std::string FindAudioDecoder(std::shared_ptr<FormatMock> format) const override;
    std::string FindAudioEncoder(std::shared_ptr<FormatMock> format) const override;
    std::vector<std::shared_ptr<VideoCapsMock>> GetVideoDecoderCaps() const override;
    std::vector<std::shared_ptr<VideoCapsMock>> GetVideoEncoderCaps() const override;
    std::vector<std::shared_ptr<AudioCapsMock>> GetAudioDecoderCaps() const override;
    std::vector<std::shared_ptr<AudioCapsMock>> GetAudioEncoderCaps() const override;

private:
    std::shared_ptr<AVCodecList> avCodecList_ = nullptr;
};
} // Media
} // OHOS
#endif // AVCODEC_LIST_NATIVE_MOCK_H