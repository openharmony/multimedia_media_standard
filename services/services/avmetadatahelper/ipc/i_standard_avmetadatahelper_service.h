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

#ifndef I_STANDARD_AVMETADATAHELPER_SERVICE_H
#define I_STANDARD_AVMETADATAHELPER_SERVICE_H

#include "ipc_types.h"
#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"
#include "i_avmetadatahelper_service.h"
#include "avsharedmemory.h"

namespace OHOS {
namespace Media {
class IStandardAVMetadataHelperService : public IRemoteBroker {
public:
    virtual ~IStandardAVMetadataHelperService() = default;
    virtual int32_t SetSource(const std::string &uri, int32_t usage) = 0;
    virtual int32_t SetSource(int32_t fd, int64_t offset, int64_t size, int32_t usage) = 0;
    virtual std::string ResolveMetadata(int32_t key) = 0;
    virtual std::unordered_map<int32_t, std::string> ResolveMetadataMap() = 0;
    virtual std::shared_ptr<AVSharedMemory> FetchArtPicture() = 0;
    virtual std::shared_ptr<AVSharedMemory> FetchFrameAtTime(
        int64_t timeUs, int32_t option, const OutputConfiguration &param) = 0;
    virtual void Release() = 0;
    virtual int32_t DestroyStub() = 0;

    /**
     * IPC code ID
     */
    enum AVMetadataHelperServiceMsg {
        SET_URI_SOURCE = 0,
        SET_FD_SOURCE,
        RESOLVE_METADATA,
        RESOLVE_METADATA_MAP,
        FETCH_ART_PICTURE,
        FETCH_FRAME_AT_TIME,
        RELEASE,
        DESTROY,
    };

    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardAVMetadataHelperService");
};
} // namespace Media
} // namespace OHOS
#endif // I_STANDARD_AVMETADATAHELPER_SERVICE_H
