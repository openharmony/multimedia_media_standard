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

#ifndef AVMUXER_CLIENT_H
#define AVMUXER_CLIENT_H

#include "i_avmuxer_service.h"
#include "i_standard_avmuxer_service.h"

namespace OHOS {
namespace Media {
class AVMuxerClient : public IAVMuxerService, public NoCopyable {
public:
    static std::shared_ptr<AVMuxerClient> Create(const sptr<IStandardAVMuxerService> &ipcProxy);
    explicit AVMuxerClient(const sptr<IStandardAVMuxerService> &ipcProxy);
    ~AVMuxerClient();

    std::vector<std::string> GetAVMuxerFormatList() override;
    int32_t SetOutput(int32_t fd, const std::string &format) override;
    int32_t SetLocation(float latitude, float longitude) override;
    int32_t SetRotation(int32_t rotation) override;
    int32_t AddTrack(const MediaDescription &trackDesc, int32_t &trackId) override;
    int32_t Start() override;
    int32_t WriteTrackSample(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo) override;
    int32_t Stop() override;
    void Release() override;

    void MediaServerDied();
private:
    std::mutex mutex_;
    sptr<IStandardAVMuxerService> avmuxerProxy_ = nullptr;
};
}  // namespace Media
}  // namespace OHOS
#endif  // AVMUXER_CLIENT_H