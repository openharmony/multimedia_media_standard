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

#ifndef RECORDERPROFILESABILITY_SINGLETON_H
#define RECORDERPROFILESABILITY_SINGLETON_H

#include <mutex>
#include "format.h"
#include "recorder_profiles_xml_parser.h"

namespace OHOS {
namespace Media {
class RecorderProfilesAbilitySingleton {
public:
    const char *MEDIA_PROFILE_CONFIG_FILE = "/etc/recorder/recorder_configs.xml";
    ~RecorderProfilesAbilitySingleton();
    static RecorderProfilesAbilitySingleton& GetInstance();
    std::vector<RecorderProfilesData> GetCapabilityDataArray() const;

private:
    bool isParsered_ = false;
    RecorderProfilesAbilitySingleton();
    bool ParseRecorderProfilesXml();
    std::vector<RecorderProfilesData> capabilityDataArray_;
    std::mutex mutex_;
};
}  // namespace Media
}  // namespace OHOS
#endif  // RECORDERPROFILESABILITY_SINGLETON_H