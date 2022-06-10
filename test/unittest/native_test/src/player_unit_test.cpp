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
#include "ui/rs_surface_node.h"
#include "window_option.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media{
namespace {
    constexpr size_t MAX_URI_SIZE = 4096;
    const string MEDIA_ROOT = " file://data/media/";
    const string VIDEO_FILE1 = MEDIA_ROOT + "test_1920_1080_1.mp4";
    const string VIDEO_FILE2 = "";
}

void PlayerUnitTest::SetUpTestCase(void) {}
void PlayerUnitTest::TearDownTestCase(void) {}
void PlayerUnitTest::SetUp(void) {}
void PlayerUnitTest::TearDown(void) {}


sptr<Surface> GetVideoSurface()
{
    sptr<Rosen::WindowOption> option = new Rosen::WindowOption();
    option->SetWindowRect({0, 0, 1920, 1080});
    option->SetWindowType(Rosen::WindowType::WINDOW_TYPE_APP_LAUNCHING);
    option->SetWindowMode(Rosen::WindowMode::WINDOW_MODE_FLOATING);
    auto previewWindow_ = Rosen::Window::Create("xcomponent_window_unittest", option);
    if (previewWindow_ == nullptr || previewWindow_->GetSurfaceNode() == nullptr) {
        return nullptr;
    }
    previewWindow_->Show();
    return previewWindow_->GetSurfaceNode()->GetSurface();
}

HWTEST(PlayerUnitTest, Player_Create_001, TestSize.Level0) 
{
    std::shared_ptr<Player> player = PlayerFactory::CreatePlayer();
    ASSERT_NE(nullptr, player);
}

HWTEST(PlayerUnitTest, Player_SetSource_001, TestSize.Level0)
{
    std::shared_ptr<Player> player = PlayerFactory::CreatePlayer();
    ASSERT_NE(nullptr, player);
    int32_t ret = player->SetSource(VIDEO_FILE1);
    EXPECT_EQ(MSERR_OK, ret);       
}

HWTEST(PlayerUnitTest, Player_SetSource_002, TestSize.Level0)
{
    std::shared_ptr<Player> player = PlayerFactory::CreatePlayer();
    ASSERT_NE(nullptr, player);
    int32_t ret = player->SetSource(VIDEO_FILE2);
    EXPECT_EQ(MSERR_INVALID_VAL, ret);       
}

HWTEST(PlayerUnitTest, Player_SetSource_003, TestSize.Level0)
{
    std::shared_ptr<Player> player = PlayerFactory::CreatePlayer();
    ASSERT_NE(nullptr, player);
    string video_file3 = MEDIA_ROOT;
    for (size_t i = 0; i <= MAX_URI_SIZE; ++i) {
        video_file3.append("0");
    }
    video_file3.append(".mp4");
    const string URL = video_file3;
    int32_t ret = player->SetSource(URL);
    EXPECT_EQ(MSERR_INVALID_VAL, ret);       
}

HWTEST(PlayerUnitTest, Player_SetCallBack_001, TestSize.Level0) 
{
    std::shared_ptr<Player> player = PlayerFactory::CreatePlayer();
    ASSERT_NE(nullptr, player);
    
    std::shared_ptr<PlayerCallBackTest> playerCallBackTest = std::make_shared<PlayerCallBackTest>();
    int32_t ret = player->SetPlayerCallback(playerCallBackTest);
    EXPECT_EQ(MSERR_OK, ret);
}

HWTEST(PlayerUnitTest, Player_PrePare_001, TestSize.Level0) 
{
    int32_t ret = -1;
    std::shared_ptr<Player> player = PlayerFactory::CreatePlayer();
    ASSERT_NE(nullptr, player);
    ret = player->SetSource(VIDEO_FILE1);
    EXPECT_EQ(MSERR_OK, ret);
    std::shared_ptr<PlayerCallBackTest> playerCallBackTest = std::make_shared<PlayerCallBackTest>();
    ret = player->SetPlayerCallback(playerCallBackTest);
    EXPECT_EQ(MSERR_OK, ret);
    ret = player->Prepare();
    EXPECT_EQ(MSERR_OK, ret);
}

HWTEST(PlayerUnitTest, Player_PrePareAsync_001, TestSize.Level0) 
{
    int32_t ret = -1;
    std::shared_ptr<Player> player = PlayerFactory::CreatePlayer();
    ASSERT_NE(nullptr, player);
    ret = player->SetSource(VIDEO_FILE1);
    EXPECT_EQ(MSERR_OK, ret);
    std::shared_ptr<PlayerCallBackTest> playerCallBackTest = std::make_shared<PlayerCallBackTest>();
    ret = player->SetPlayerCallback(playerCallBackTest);
    EXPECT_EQ(MSERR_OK, ret);
    ret = player->PrepareAsync();
    EXPECT_EQ(MSERR_OK, ret);
    
}

HWTEST(PlayerUnitTest, Player_SetVideoSurface_001, TestSize.Level0) 
{
    int32_t ret = -1;
    std::shared_ptr<Player> player = PlayerFactory::CreatePlayer();
    ASSERT_NE(nullptr, player);
    ret = player->SetSource(VIDEO_FILE1);
    EXPECT_EQ(MSERR_OK, ret);
    sptr<Surface> videoSurface = GetVideoSurface();
    ret = player->SetVideoSurface(videoSurface);
    EXPECT_EQ(MSERR_OK, ret);
}


HWTEST(PlayerUnitTest, Player_Play_001, TestSize.Level0) 
{
    int32_t ret = -1;
    std::shared_ptr<Player> player =  PlayerFactory::CreatePlayer();
    ASSERT_NE(nullptr, player);
    ret = player->SetSource(VIDEO_FILE1);
    EXPECT_EQ(MSERR_OK, ret);
    std::shared_ptr<PlayerCallBackTest> playerCallBackTest = std::make_shared<PlayerCallBackTest>();
    ret = player->SetPlayerCallback(playerCallBackTest);
    EXPECT_EQ(MSERR_OK, ret);
    ret = player->PrepareAsync();
    EXPECT_EQ(MSERR_OK, ret);
    ret = player->Play();
    EXPECT_EQ(MSERR_OK, ret);
    EXPECT_TRUE(player->IsPlaying());
}

HWTEST(PlayerUnitTest, Player_Stop_001, TestSize.Level0) 
{
    int32_t ret = -1;
    std::shared_ptr<Player> player =  PlayerFactory::CreatePlayer();
    ASSERT_NE(nullptr, player);
    ret = player->SetSource(VIDEO_FILE1);
    EXPECT_EQ(MSERR_OK, ret);
    std::shared_ptr<PlayerCallBackTest> playerCallBackTest = std::make_shared<PlayerCallBackTest>();
    ret = player->SetPlayerCallback(playerCallBackTest);
    EXPECT_EQ(MSERR_OK, ret);
    ret = player->PrepareAsync();
    EXPECT_EQ(MSERR_OK, ret);
    ret = player->Play();
    EXPECT_EQ(MSERR_OK, ret);
    EXPECT_TRUE(player->IsPlaying());
    ret = player->Stop();
    EXPECT_EQ(MSERR_OK, ret);
}


} // namespace Media
} // namespace OHOS
