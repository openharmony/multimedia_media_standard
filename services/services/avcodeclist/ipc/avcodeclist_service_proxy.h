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

#ifndef AVCODECLIST_SERVICE_PROXY_H
#define AVCODECLIST_SERVICE_PROXY_H

#include "i_standard_avcodeclist_service.h"
#include "nocopyable.h"
#include "media_parcel.h"
#include "avcodeclist_parcel.h"

namespace OHOS {
namespace Media {
class AVCodecListServiceProxy : public IRemoteProxy<IStandardAVCodecListService>, public NoCopyable {
public:
    explicit AVCodecListServiceProxy(const sptr<IRemoteObject> &impl);
    virtual ~AVCodecListServiceProxy();

    std::string FindVideoDecoder(const Format &format) override;
    std::string FindVideoEncoder(const Format &format) override;
    std::string FindAudioDecoder(const Format &format) override;
    std::string FindAudioEncoder(const Format &format) override;
    std::vector<CapabilityData> GetCodecCapabilityInfos() override;
    int32_t DestroyStub() override;

private:
    static inline BrokerDelegator<AVCodecListServiceProxy> delegator_;
};
} // namespace Media
} // namespace OHOS
#endif // AVCODECLIST_SERVICE_PROXY_H
