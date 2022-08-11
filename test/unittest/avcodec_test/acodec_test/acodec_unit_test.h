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

#ifndef ACODEC_UNIT_TEST_H
#define ACODEC_UNIT_TEST_H

#include "gtest/gtest.h"
#include "acodec_mock.h"
namespace OHOS {
namespace Media {
class ACodecUnitTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

protected:
    std::shared_ptr<ACodecMock> audioCodec_ = nullptr;
    std::shared_ptr<ADecCallbackTest> adecCallback_ = nullptr;
    std::shared_ptr<AEncCallbackTest> aencCallback_ = nullptr;
    bool createByMineFlag_ = true;
    const ::testing::TestInfo *testInfo_ = nullptr;
};
} // namespace Media
} // namespace OHOS
#endif // ACODEC_UNIT_TEST_H