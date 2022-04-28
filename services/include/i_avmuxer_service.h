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

#ifndef I_AVMUXER_SERVICE_H
#define I_AVMUXER_SERVICE_H

#include <string>
#include <vector>
#include <memory>
#include "avsharedmemory.h"
#include "avcontainer_common.h"
#include "media_description.h"

namespace OHOS {
namespace Media {
class IAVMuxerService {
public:
    virtual ~IAVMuxerService() = default;

    virtual std::vector<std::string> GetAVMuxerFormatList() = 0;
    virtual int32_t SetOutput(int32_t fd, const std::string &format) = 0;
    virtual int32_t SetLocation(float latitude, float longtitude) = 0;
    virtual int32_t SetRotation(int32_t rotation) = 0;
    virtual int32_t AddTrack(const MediaDescription &trackDesc, int32_t &trackIdx) = 0;
    virtual int32_t Start() = 0;
    virtual int32_t WriteTrackSample(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo) = 0;
    virtual int32_t Stop() = 0;
    virtual void Release() = 0;
};
} // namespace Media
} // namespace OHOS
#endif // I_AVMUXER_SERVICE_H