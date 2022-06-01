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

#ifndef RECORDERPROFILES_SERVICE_STUB_H
#define RECORDERPROFILES_SERVICE_STUB_H

#include <map>
#include "i_standard_recorder_profiles_service.h"
#include "media_death_recipient.h"
#include "recorder_profiles_server.h"
#include "nocopyable.h"
#include "media_parcel.h"
#include "recorder_profiles_parcel.h"

namespace OHOS {
namespace Media {
class RecorderProfilesServiceStub : public IRemoteStub<IStandardRecorderProfilesService>, public NoCopyable {
public:
    static sptr<RecorderProfilesServiceStub> Create();
    virtual ~RecorderProfilesServiceStub();

    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
    bool IsAudioRecoderConfigSupported(const RecorderProfilesData &profile) override;
    bool HasVideoRecorderProfile(int32_t sourceId, int32_t qualityLevel) override;
    std::vector<RecorderProfilesData> GetAudioRecorderCapsInfo() override;
    std::vector<RecorderProfilesData> GetVideoRecorderCapsInfo() override;
    RecorderProfilesData GetVideoRecorderProfileInfo(int32_t sourceId, int32_t qualityLevel) override;
    int32_t DestroyStub() override;

private:
    RecorderProfilesServiceStub();
    int32_t Init();
    int32_t IsAudioRecoderConfigSupported(MessageParcel &data, MessageParcel &reply);
    int32_t HasVideoRecorderProfile(MessageParcel &data, MessageParcel &reply);
    int32_t GetAudioRecorderCapsInfo(MessageParcel &data, MessageParcel &reply);
    int32_t GetVideoRecorderCapsInfo(MessageParcel &data, MessageParcel &reply);
    int32_t GetVideoRecorderProfileInfo(MessageParcel &data, MessageParcel &reply);
    int32_t DestroyStub(MessageParcel &data, MessageParcel &reply);
    std::shared_ptr<IRecorderProfilesService> mediaProfileServer_ = nullptr;
    using MediaProfileStubFunc = int32_t(RecorderProfilesServiceStub::*)(MessageParcel &data, MessageParcel &reply);
    std::map<uint32_t, MediaProfileStubFunc> mediaProfileFuncs_;
    std::mutex mutex_;
};
}  // namespace Media
}  // namespace OHOS
#endif  // RECORDERPROFILES_SERVICE_STUB_H
