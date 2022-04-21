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

#ifndef AVMUXER_SERVER_H
#define AVMUXER_SERVER_H

#include <mutex>
#include "i_avmuxer_service.h"
#include "i_avmuxer_engine.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
enum AVMuxerStates : int32_t {
    AVMUXER_IDEL = 0,
    AVMUXER_OUTPUT_SET,
    AVMUXER_PARAMETER_SET,
    AVMUXER_STARTED,
    AVMUXER_SAMPLE_WRITING,
};
    
class AVMuxerServer : public IAVMuxerService, public NoCopyable {
public:
    static std::shared_ptr<IAVMuxerService> Create();
    AVMuxerServer();
    ~AVMuxerServer();

    std::vector<std::string> GetAVMuxerFormatList() override;
    int32_t SetOutput(int32_t fd, const std::string &format) override;
    int32_t SetLocation(float latitude, float longitude) override;
    int32_t SetRotation(int32_t rotation) override;
    int32_t AddTrack(const MediaDescription &trackDesc, int32_t &trackId) override;
    int32_t Start() override;
    int32_t WriteTrackSample(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo) override;
    int32_t Stop() override;
    void Release() override;
private:
    int32_t Init();
    std::mutex mutex_;
    std::shared_ptr<IAVMuxerEngine> avmuxerEngine_ = nullptr;
    AVMuxerStates curState_ = AVMUXER_IDEL;
    uint32_t trackNum_ = 0;
};
}  // namespace Media
}  // namespace OHOS
#endif