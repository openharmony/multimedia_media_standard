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

#ifndef AVCODECLIST_SERVICE_STUB_H
#define AVCODECLIST_SERVICE_STUB_H

#include <map>
#include "i_standard_avcodeclist_service.h"
#include "media_death_recipient.h"
#include "avcodeclist_server.h"
#include "nocopyable.h"
#include "media_parcel.h"
#include "avcodeclist_parcel.h"

namespace OHOS {
namespace Media {
class AVCodecListServiceStub : public IRemoteStub<IStandardAVCodecListService>, public NoCopyable {
public:
    static sptr<AVCodecListServiceStub> Create();
    virtual ~AVCodecListServiceStub();

    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
    std::string FindVideoDecoder(const Format &format) override;
    std::string FindVideoEncoder(const Format &format) override;
    std::string FindAudioDecoder(const Format &format) override;
    std::string FindAudioEncoder(const Format &format) override;
    std::vector<CapabilityData>  GetCodecCapabilityInfos() override;
    int32_t DestroyStub() override;

private:
    AVCodecListServiceStub();
    int32_t Init();
    int32_t FindVideoDecoder(MessageParcel &data, MessageParcel &reply);
    int32_t FindVideoEncoder(MessageParcel &data, MessageParcel &reply);
    int32_t FindAudioDecoder(MessageParcel &data, MessageParcel &reply);
    int32_t FindAudioEncoder(MessageParcel &data, MessageParcel &reply);
    int32_t GetCodecCapabilityInfos(MessageParcel &data, MessageParcel &reply);
    int32_t DestroyStub(MessageParcel &data, MessageParcel &reply);
    std::shared_ptr<IAVCodecListService> codecListServer_ = nullptr;
    using AVCodecListStubFunc = int32_t(AVCodecListServiceStub::*)(MessageParcel &data, MessageParcel &reply);
    std::map<uint32_t, AVCodecListStubFunc> codecListFuncs_;
    std::mutex mutex_;
};
} // namespace Media
} // namespace OHOS
#endif // AVCODECLIST_SERVICE_STUB_H
