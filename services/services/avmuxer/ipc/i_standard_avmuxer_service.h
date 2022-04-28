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

#ifndef I_STANDARD_AVMUXER_SERVICE_H
#define I_STANDARD_AVMUXER_SERVICE_H

#include "i_avmuxer_service.h"
#include "iremote_proxy.h"

namespace OHOS {
namespace Media {
class IStandardAVMuxerService : public IRemoteBroker {
public:
    virtual ~IStandardAVMuxerService() = default;
    virtual std::vector<std::string> GetAVMuxerFormatList() = 0;
    virtual int32_t SetOutput(int32_t fd, const std::string &format) = 0;
    virtual int32_t SetLocation(float latitude, float longitude) = 0;
    virtual int32_t SetRotation(int32_t rotation) = 0;
    virtual int32_t AddTrack(const MediaDescription &trackDesc, int32_t &trackId) = 0;
    virtual int32_t Start() = 0;
    virtual int32_t WriteTrackSample(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo) = 0;
    virtual int32_t Stop() = 0;
    virtual void Release() = 0;
    virtual int32_t DestroyStub() = 0;

    enum AVMuxerServiceMsg {
        GET_MUXER_FORMAT_LIST = 0,
        SET_OUTPUT,
        SET_LOCATION,
        SET_ORIENTATION_HINT,
        ADD_TRACK,
        START,
        WRITE_TRACK_SAMPLE,
        STOP,
        RELEASE,
        DESTROY,
    };

    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardAVMuxerServiceq1a");
};
}  // namespace Media
}  // namespace OHOS
#endif  // I_STANDARD_AVMUXER_SERVICE_H