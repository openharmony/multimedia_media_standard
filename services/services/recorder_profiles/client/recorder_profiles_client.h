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

#ifndef RECORDERPROFILES_SERVICE_CLIENT_H
#define RECORDERPROFILES_SERVICE_CLIENT_H

#include <mutex>
#include "i_recorder_profiles_service.h"
#include "i_standard_recorder_profiles_service.h"

namespace OHOS {
namespace Media {
class RecorderProfilesClient : public IRecorderProfilesService {
public:
    static std::shared_ptr<RecorderProfilesClient> Create(const sptr<IStandardRecorderProfilesService> &ipcProxy);
    explicit RecorderProfilesClient(const sptr<IStandardRecorderProfilesService> &ipcProxy);
    ~RecorderProfilesClient();
    void MediaServerDied();

    bool IsAudioRecoderConfigSupported(const RecorderProfilesData &profile) override;
    bool HasVideoRecorderProfile(int32_t sourceId, int32_t qualityLevel) override;
    std::vector<RecorderProfilesData> GetAudioRecorderCapsInfo() override;
    std::vector<RecorderProfilesData> GetVideoRecorderCapsInfo() override;
    RecorderProfilesData GetVideoRecorderProfileInfo(int32_t sourceId, int32_t qualityLevel) override;

private:
    sptr<IStandardRecorderProfilesService> recorderProfilesProxy_ = nullptr;
    std::mutex mutex_;
};
}  // namespace Media
}  // namespace OHOS
#endif  // RECORDERPROFILES_SERVICE_CLIENT_H
