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

#ifndef VIDEO_CAPTURE_SF_ES_AVC_IMPL_H
#define VIDEO_CAPTURE_SF_ES_AVC_IMPL_H

#include "video_capture_sf_impl.h"

namespace OHOS {
namespace Media {
class VideoCaptureSfEsAvcImpl : public VideoCaptureSfImpl {
public:
    VideoCaptureSfEsAvcImpl();
    virtual ~VideoCaptureSfEsAvcImpl();

protected:
    std::shared_ptr<EsAvcCodecBuffer> DoGetCodecBuffer() override;
    std::shared_ptr<VideoFrameBuffer> DoGetFrameBuffer() override;

private:
    std::shared_ptr<VideoFrameBuffer> GetIDRFrame();
    const uint8_t *FindNextNal(const uint8_t *start, const uint8_t *end, uint32_t &nalLen);
    void GetCodecData(const uint8_t *data, int32_t len, std::vector<uint8_t> &sps, std::vector<uint8_t> &pps,
            std::vector<uint8_t> &sei);
    GstBuffer* AVCDecoderConfiguration(std::vector<uint8_t> &sps,
            std::vector<uint8_t> &pps);

    int32_t frameSequence_ = 0;
    char *codecData_ = nullptr;
    int32_t codecDataSize_ = 0;
    uint32_t nalSize_ = 0;
};
}  // namespace Media
}  // namespace OHOS
#endif // VIDEO_CAPTURE_SF_ES_AVC_IMPL_H
