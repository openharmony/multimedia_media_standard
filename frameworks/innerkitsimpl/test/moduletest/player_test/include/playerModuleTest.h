/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "gtest/gtest.h"
#include "player.h"

#include <memory>

using TestSample = struct TagTestSample {
    std::shared_ptr<OHOS::Media::Player> adapter;
};

// CallBack Flag
enum TestCallBackFlag {
    FLAG0 = 0,
    FLAG1 = 1,
};

class PlayerModuleTest : public testing::Test {
public:
    // SetUpTestCase: before all testcasee
    static void SetUpTestCase(void);
    // TearDownTestCase: after all testcase
    static void TearDownTestCase(void);
    // SetUp
    void SetUp(void);
    // TearDown
    void TearDown(void);

    static bool m_callbackTestCompleted;
    static int32_t m_isPlayingFlag;
    static int32_t m_isPlaybackCompletedFlag;
    static int32_t m_isPausedFlag;
    static int32_t m_isStoppedFlag;
    static int32_t m_isSeekDoneFlag;
    static int32_t m_getCurrentPositionFlag;
};

static TagTestSample g_tagTestSample;

bool PlayerModuleTest::m_callbackTestCompleted = false;
int32_t PlayerModuleTest::m_isPlayingFlag = FLAG0;
int32_t PlayerModuleTest::m_isPlaybackCompletedFlag = FLAG0;
int32_t PlayerModuleTest::m_isPausedFlag = FLAG0;
int32_t PlayerModuleTest::m_isStoppedFlag = FLAG0;
int32_t PlayerModuleTest::m_isSeekDoneFlag = FLAG0;
int32_t PlayerModuleTest::m_getCurrentPositionFlag = FLAG0;