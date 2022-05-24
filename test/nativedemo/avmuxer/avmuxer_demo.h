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

#ifndef AVMUXER_DEMO_H
#define AVMUXER_DEMO_H

#include "avmuxer.h"

namespace OHOS {
namespace Media {
class AVMuxerDemo : public NoCopyable {
public:
    AVMuxerDemo() = default;
    ~AVMuxerDemo() = default;
    void RunCase();
private:
    bool PushBuffer(std::shared_ptr<std::ifstream> File, const int32_t frameSize,
        int32_t i, int32_t trackId, int64_t stamp);
    void WriteTrackSample();
    void SetParameter(const std::string &type);
    bool AddTrackVideo(std::string &videoType);
    bool AddTrackAudio(std::string &audioType);
    void DoNext();
    void SetMode(int32_t mode);
    std::shared_ptr<AVMuxer> avmuxer_;
    int32_t videoTrackId_ = 0;
    int32_t audioTrackId_ = 0;
    uint32_t videoTimeDuration_ = 0;
    uint32_t audioTimeDuration_ = 0;
    uint32_t videoFrameNum_ = 0;
    uint32_t audioFrameNum_ = 0;
    const int32_t *videoFrameArray_ = nullptr;
    const int32_t *audioFrameArray_ = nullptr;
    std::shared_ptr<std::ifstream> videoFile_ = nullptr;
    std::shared_ptr<std::ifstream> audioFile_ = nullptr;
    std::string videoType_ = std::string("");
    std::string audioType_ = std::string("");
    std::string path_ = std::string("");
    std::string format_ = std::string("");
};
}  // namespace Media
}  // namespace OHOS
#endif  // AVMUXER_DEMO_H