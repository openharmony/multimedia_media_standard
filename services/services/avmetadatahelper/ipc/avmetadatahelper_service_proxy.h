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

#ifndef AVMETADATAHELPER_SERVICE_PROXY_H
#define AVMETADATAHELPER_SERVICE_PROXY_H

#include "i_standard_avmetadatahelper_service.h"

namespace OHOS {
namespace Media {
class AVMetadataHelperServiceProxy : public IRemoteProxy<IStandardAVMetadataHelperService>, public NoCopyable {
public:
    explicit AVMetadataHelperServiceProxy(const sptr<IRemoteObject> &impl);
    virtual ~AVMetadataHelperServiceProxy();

    int32_t SetSource(const std::string &uri, int32_t usage) override;
    int32_t SetSource(int32_t fd, int64_t offset, int64_t size, int32_t usage) override;
    std::string ResolveMetadata(int32_t key) override;
    std::unordered_map<int32_t, std::string> ResolveMetadataMap() override;
    std::shared_ptr<AVSharedMemory> FetchArtPicture() override;
    std::shared_ptr<AVSharedMemory> FetchFrameAtTime(int64_t timeUs,
        int32_t option, const OutputConfiguration &param) override;
    void Release() override;
    int32_t DestroyStub() override;
private:
    static inline BrokerDelegator<AVMetadataHelperServiceProxy> delegator_;
};
} // namespace Media
} // namespace OHOS
#endif // AVMETADATAHELPER_SERVICE_PROXY_H