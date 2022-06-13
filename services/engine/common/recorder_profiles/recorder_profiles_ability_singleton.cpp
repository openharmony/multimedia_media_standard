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

#include "recorder_profiles_ability_singleton.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderProfilesAbilitySingleton"};
}

namespace OHOS {
namespace Media {
RecorderProfilesAbilitySingleton& RecorderProfilesAbilitySingleton::GetInstance()
{
    static RecorderProfilesAbilitySingleton instance;
    bool ret = instance.ParseRecorderProfilesXml();
    if (!ret) {
        MEDIA_LOGD("ParseRecorderProfilesXml failed");
    }
    return instance;
}

RecorderProfilesAbilitySingleton::RecorderProfilesAbilitySingleton()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

RecorderProfilesAbilitySingleton::~RecorderProfilesAbilitySingleton()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

bool RecorderProfilesAbilitySingleton::ParseRecorderProfilesXml()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (isParsered_) {
        return true;
    }
    capabilityDataArray_.clear();

    std::shared_ptr<RecorderProfilesXmlParser> xmlParser = std::make_shared<RecorderProfilesXmlParser>();

    bool ret = xmlParser->LoadConfiguration(MEDIA_PROFILE_CONFIG_FILE);
    if (!ret) {
        isParsered_ = false;
        MEDIA_LOGE("RecorderProfiles LoadConfiguration failed");
        return false;
    }
    ret = xmlParser->Parse();
    if (!ret) {
        isParsered_ = false;
        MEDIA_LOGE("RecorderProfiles Parse failed.");
        return false;
    }
    std::vector<RecorderProfilesData> data = xmlParser->GetRecorderProfileDataArray();
    capabilityDataArray_.insert(capabilityDataArray_.end(), data.begin(), data.end());
    isParsered_ = true;
    return true;
}

std::vector<RecorderProfilesData> RecorderProfilesAbilitySingleton::GetCapabilityDataArray() const
{
    return capabilityDataArray_;
}
}  // namespace Media
}  // namespace OHOS
