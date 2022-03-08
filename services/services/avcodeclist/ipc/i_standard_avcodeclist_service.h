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

#ifndef I_STANDARD_AVCODECLIST_SERVICE_H
#define I_STANDARD_AVCODECLIST_SERVICE_H

#include "ipc_types.h"
#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"
#include "avcodec_info.h"
#include "avsharedmemory.h"

namespace OHOS {
namespace Media {
class IStandardAVCodecListService : public IRemoteBroker {
public:
    virtual ~IStandardAVCodecListService() = default;
    virtual std::string FindVideoDecoder(const Format &format) = 0;
    virtual std::string FindVideoEncoder(const Format &format) = 0;
    virtual std::string FindAudioDecoder(const Format &format) = 0;
    virtual std::string FindAudioEncoder(const Format &format) = 0;
    virtual std::vector<CapabilityData>  GetCodecCapabilityInfos() = 0;
    virtual int32_t DestroyStub() = 0;

    /**
     * IPC code ID
     */
    enum AVCodecListServiceMsg : int32_t {
        SET_LISTENER_OBJ = 0,
        FIND_VIDEO_DECODER,
        FIND_VIDEO_ENCODER,
        FIND_AUDIO_DECODER,
        FIND_AUDIO_ENCODER,
        GET_CAPABILITY_INFOS,
        DESTROY
    };

    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardAVCodecListService");
};
} // namespace Media
} // namespace OHOS
#endif // I_STANDARD_AVCODECLIST_SERVICE_H
