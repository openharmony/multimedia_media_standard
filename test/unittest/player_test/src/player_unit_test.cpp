/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "player_unit_test.h"
#include "media_errors.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media::PlayerTestParam;

namespace OHOS {
namespace Media {
namespace {
    const string MEDIA_ROOT = "file://data/media/";
    const string VIDEO_FILE1 = MEDIA_ROOT + "test_1920_1080_1.mp4";
    const string VIDEO_FILE2 = "";
}

void PlayerUnitTest::SetUpTestCase(void) {}
void PlayerUnitTest::TearDownTestCase(void) {}

void PlayerUnitTest::SetUp(void)
{
    testObj = std::make_shared<PlayerSignal>();
    player_ = std::make_shared<Player_mock>(testObj);
    ASSERT_NE(nullptr, player_);
    EXPECT_TRUE(player_->CreatePlayer());
}

void PlayerUnitTest::TearDown(void)
{
    player_->Reset();
    player_->Release();
}

HWTEST_F(PlayerUnitTest, Player_SetSource_001, TestSize.Level0)
{
    int32_t ret = player_->SetSource(VIDEO_FILE1);
    EXPECT_EQ(MSERR_OK, ret);
}

HWTEST_F(PlayerUnitTest, Player_SetSource_002, TestSize.Level0)
{
    int32_t ret = player_->SetSource(VIDEO_FILE2);
    EXPECT_EQ(MSERR_INVALID_VAL, ret);
}

HWTEST_F(PlayerUnitTest, Player_SetCallback_001, TestSize.Level1)
{
    std::shared_ptr<PlayerCallbackTest> player_CallbackTest = std::make_shared<PlayerCallbackTest>(testObj);
    int32_t ret = player_->SetPlayerCallback(player_CallbackTest);
    EXPECT_EQ(MSERR_OK, ret);
}

HWTEST_F(PlayerUnitTest, Player_SetCallback_002, TestSize.Level1)
{
    int32_t ret = player_->SetPlayerCallback(nullptr);
    EXPECT_EQ(MSERR_INVALID_VAL, ret);
}

HWTEST_F(PlayerUnitTest, Player_PrePare_001, TestSize.Level0)
{
    EXPECT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    std::shared_ptr<PlayerCallbackTest> player_CallbackTest = std::make_shared<PlayerCallbackTest>(testObj);
    EXPECT_EQ(MSERR_OK, player_->SetPlayerCallback(player_CallbackTest));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
}

HWTEST_F(PlayerUnitTest, Player_PrePareAsync_001, TestSize.Level0)
{
    EXPECT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    std::shared_ptr<PlayerCallbackTest> player_CallbackTest = std::make_shared<PlayerCallbackTest>(testObj);
    EXPECT_EQ(MSERR_OK, player_->SetPlayerCallback(player_CallbackTest));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
}

HWTEST_F(PlayerUnitTest, Player_SetVideoSurface_001, TestSize.Level0)
{
    EXPECT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(player_->GetVideoSurface()));
}

HWTEST_F(PlayerUnitTest, Player_Play_001, TestSize.Level0)
{
    EXPECT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    std::shared_ptr<PlayerCallbackTest> player_CallbackTest = std::make_shared<PlayerCallbackTest>(testObj);
    EXPECT_EQ(MSERR_OK, player_->SetPlayerCallback(player_CallbackTest));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_TRUE(player_->IsPlaying());
}

HWTEST_F(PlayerUnitTest, Player_Stop_001, TestSize.Level0)
{
    EXPECT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    std::shared_ptr<PlayerCallbackTest> player_CallbackTest = std::make_shared<PlayerCallbackTest>(testObj);
    EXPECT_EQ(MSERR_OK, player_->SetPlayerCallback(player_CallbackTest));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_EQ(MSERR_OK, player_->Stop());
}

HWTEST_F(PlayerUnitTest, Player_Pause_001, TestSize.Level0)
{
    EXPECT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    std::shared_ptr<PlayerCallbackTest> player_CallbackTest = std::make_shared<PlayerCallbackTest>(testObj);
    EXPECT_EQ(MSERR_OK, player_->SetPlayerCallback(player_CallbackTest));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_EQ(MSERR_OK, player_->Pause());
    EXPECT_FALSE(player_->IsPlaying());
}

HWTEST_F(PlayerUnitTest, Player_Seek_001, TestSize.Level0)
{
    EXPECT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    std::shared_ptr<PlayerCallbackTest> player_CallbackTest = std::make_shared<PlayerCallbackTest>(testObj);
    EXPECT_EQ(MSERR_OK, player_->SetPlayerCallback(player_CallbackTest));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_EQ(MSERR_OK, player_->Seek(SEEK_TIME_2_SEC, SEEK_CLOSEST));
    int32_t time;
    EXPECT_EQ(MSERR_OK, player_->GetCurrentTime(time));
    EXPECT_NEAR(SEEK_TIME_2_SEC, time, DELTA_TIME);
}

} // namespace Media
} // namespace OHOS
