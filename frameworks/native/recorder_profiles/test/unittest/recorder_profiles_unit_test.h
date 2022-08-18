/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef RECORDER_PROFILES_UNIT_TEST_H
#define RECORDER_PROFILES_UNIT_TEST_H

#include "gtest/gtest.h"
#include "recorder_profiles.h"

namespace OHOS {
namespace Media {
class RecorderProfilesUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
protected:
    bool CheckAudioRecorderCapsArray(const std::vector<std::shared_ptr<AudioRecorderCaps>> &audioRecorderArray) const;
    bool CheckVideoRecorderCapsArray(const std::vector<std::shared_ptr<VideoRecorderCaps>> &videoRecorderArray) const;
};
} // namespace Media
} // namespace OHOS
#endif
