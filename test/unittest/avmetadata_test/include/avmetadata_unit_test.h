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
#ifndef AVMETADATA_UNIT_TEST_H
#define AVMETADATA_UNIT_TEST_H

#include "gtest/gtest.h"
#include "avmetadata_mock.h"

namespace OHOS {
namespace Media {
class AVMetadataUnitTest : public testing::Test {
public:
    // SetUpTestCase: before all testcases
    static void SetUpTestCase(void)
    {
        UNITTEST_INFO_LOG("AVMetadataUnitTest::SetUpTestCase");
    };
    // TearDownTestCase: after all testcase
    static void TearDownTestCase(void)
    {
        UNITTEST_INFO_LOG("AVMetadataUnitTest::TearDownTestCase");
    };
    // SetUp
    void SetUp(void)
    {
        testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
        UNITTEST_INFO_LOG("AVMetadataUnitTest::SetUp");
    };
    // TearDown
    void TearDown(void)
    {
        UNITTEST_INFO_LOG("AVMetadataUnitTest::TearDown");
    };
    void CheckMeta(std::string uri, std::unordered_map<int32_t, std::string> expectMeta);
    void GetThumbnail(const std::string uri);
    const ::testing::TestInfo *testInfo_ = nullptr;
};
}
}
#endif
