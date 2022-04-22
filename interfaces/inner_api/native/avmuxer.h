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

#ifndef AVMUXER_H
#define AVMUXER_H

#include <string>
#include <vector>
#include <memory>
#include "avcontainer_common.h"
#include "media_description.h"

namespace OHOS {
namespace Media {
class AVMuxer {
public:
    virtual ~AVMuxer() = default;

    virtual std::vector<std::string> GetAVMuxerFormatList() = 0;
    virtual int32_t SetOutput(int32_t fd, const std::string &format) = 0;
    virtual int32_t SetLocation(float latitude, float longtitude) = 0;
    virtual int32_t SetRotation(int rotation) = 0;
    virtual int32_t AddTrack(const MediaDescription &trackDesc, int32_t &trackIdx) = 0;
    virtual int32_t Start() = 0;
    virtual int32_t WriteTrackSample(std::shared_ptr<AVMemory> sampleData, const TrackSampleInfo &info) = 0;
    virtual int32_t Stop() = 0;
    virtual void Release() = 0;
};

class __attribute__((visibility("default"))) AVMuxerFactory {
public:
    static std::shared_ptr<AVMuxer> CreateAVMuxer();
private:
    AVMuxerFactory() = default;
    ~AVMuxerFactory() = default;
};
} // namespace Media
} // namespace OHOS
#endif // AVMUXER_H