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

#ifndef VCODEC_UNIT_TEST_H
#define VCODEC_UNIT_TEST_H

#include "gtest/gtest.h"
#include "vdec_mock.h"
#include "venc_mock.h"
namespace OHOS {
namespace Media {
class VCodecUnitTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);
    bool CreateVideoCodecByName(const std::string &decName, const std::string &encName);
    bool CreateVideoCodecByMime(const std::string &decMime, const std::string &encMime);
protected:
    std::shared_ptr<VDecMock> videoDec_ = nullptr;
    std::shared_ptr<VDecCallbackTest> vdecCallback_ = nullptr;
    std::shared_ptr<VEncMock> videoEnc_ = nullptr;
    std::shared_ptr<VEncCallbackTest> vencCallback_ = nullptr;
    const ::testing::TestInfo *testInfo_ = nullptr;
    bool createCodecSuccess_ = false;
};
} // namespace Media
} // namespace OHOS
#endif // VCODEC_UNIT_TEST_H