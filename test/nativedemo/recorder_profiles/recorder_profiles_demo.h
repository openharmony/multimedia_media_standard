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

#ifndef RECORDERPROFILES_DEMO_H
#define RECORDERPROFILES_DEMO_H

#include "securec.h"
#include "nocopyable.h"
#include "recorder_profiles.h"

namespace OHOS {
namespace Media {
class RecorderProfilesDemo : public NoCopyable {
public:
    RecorderProfilesDemo() = default;
    virtual ~RecorderProfilesDemo() = default;

    void RunCase(const std::string &path);
    void DoNext();
private:
    int32_t sourceId_ = 0;
    int32_t qualityLevel_ = 0;
    std::shared_ptr<AudioRecorderProfile> profile_;
    void PrintAudioRecorderCapsArray(const std::vector<std::shared_ptr<AudioRecorderCaps>> &audioRecorderArray) const;
    void PrintVideoRecorderCapsArray(const std::vector<std::shared_ptr<VideoRecorderCaps>> &videoRecorderArray) const;
    void PrintVideoRecorderProfile(const std::shared_ptr<VideoRecorderProfile> &videoRecorderProfile) const;
    void PrintIntArray(const std::vector<int32_t> &array, const std::string &logmsg) const;
    void PrintMuxerFormatList(const std::vector<std::string> &formatList) const;
    void PrintInputInfo() const;

    bool BuildSourceId();
    bool CreatProfile();
    bool SetIntParamValue(const std::string &strKey);
    bool SetStringParamValue(const std::string &strKey);
};
}  // namespace Media
}  // namespace OHOS
#endif  // RECORDERPROFILES_DEMO_H