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

#ifndef I_STANDARD_RECORDERPROFILES_SERVICE_H
#define I_STANDARD_RECORDERPROFILES_SERVICE_H

#include "ipc_types.h"
#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"
#include "i_recorder_profiles_service.h"
#include "avsharedmemory.h"

namespace OHOS {
namespace Media {
/**
 * IPC code ID
 */
enum class RecorderProfilesServiceMsg : uint32_t {
    RECORDER_PROFILES_IS_AUDIO_RECORDER_SUPPORT = 1,
    RECORDER_PROFILES_HAS_VIDEO_RECORD_PROFILE,
    RECORDER_PROFILES_GET_AUDIO_RECORDER_CAPS,
    RECORDER_PROFILES_GET_VIDEO_RECORDER_CAPS,
    RECORDER_PROFILES_GET_VIDEO_RECORDER_PROFILE,
    RECORDER_PROFILES_DESTROY
};
class IStandardRecorderProfilesService : public IRemoteBroker {
public:
    virtual ~IStandardRecorderProfilesService() = default;
    virtual bool IsAudioRecoderConfigSupported(const RecorderProfilesData &profile) = 0;
    virtual bool HasVideoRecorderProfile(int32_t sourceId, int32_t qualityLevel) = 0;
    virtual std::vector<RecorderProfilesData> GetAudioRecorderCapsInfo() = 0;
    virtual std::vector<RecorderProfilesData> GetVideoRecorderCapsInfo() = 0;
    virtual RecorderProfilesData GetVideoRecorderProfileInfo(int32_t sourceId, int32_t qualityLevel) = 0;
    virtual int32_t DestroyStub() = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardRecorderProfilesService");
};
}  // namespace Media
}  // namespace OHOS
#endif  // I_STANDARD_RECORDERPROFILES_SERVICE_H
