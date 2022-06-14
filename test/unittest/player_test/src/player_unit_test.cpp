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
    const string MEDIA_ROOT = "file://data/media/";
    const string VIDEO_FILE1 = MEDIA_ROOT + "test_1920_1080_1.mp4";
    const string VIDEO_FILE2 = "";
}

void PlayerUnitTest::SetUpTestCase(void) {}
void PlayerUnitTest::TearDownTestCase(void) {}

void PlayerUnitTest::SetUp(void) 
{
    player = PlayerFactory::CreatePlayer();
}

void PlayerUnitTest::TearDown(void)
{
    player->Reset();
    player->Release();
    if (previewWindow_ != nullptr) {
        previewWindow_->Destroy();
        previewWindow_ = nullptr;
    }
}


sptr<Surface> PlayerUnitTest::GetVideoSurface()
{
    sptr<Rosen::WindowOption> option = new Rosen::WindowOption();
    option->SetWindowRect({0, 0, 1920, 1080});
    option->SetWindowType(Rosen::WindowType::WINDOW_TYPE_APP_LAUNCHING);
    option->SetWindowMode(Rosen::WindowMode::WINDOW_MODE_FLOATING);
    previewWindow_ = Rosen::Window::Create("xcomponent_window_unittest", option);
    if (previewWindow_ == nullptr || previewWindow_->GetSurfaceNode() == nullptr) {
        return nullptr;
    }
    previewWindow_->Show();
    return previewWindow_->GetSurfaceNode()->GetSurface();
}

HWTEST_F(PlayerUnitTest, Player_Create_001, TestSize.Level0) 
{
    std::shared_ptr<Player> player = PlayerFactory::CreatePlayer();
    ASSERT_NE(nullptr, player);
}

HWTEST_F(PlayerUnitTest, Player_SetSource_001, TestSize.Level0)
{
    int32_t ret = player->SetSource(VIDEO_FILE1);
    EXPECT_EQ(MSERR_OK, ret);       
}

HWTEST_F(PlayerUnitTest, Player_SetSource_002, TestSize.Level0)
{
    int32_t ret = player->SetSource(VIDEO_FILE2);
    EXPECT_EQ(MSERR_INVALID_VAL, ret);       
}

HWTEST_F(PlayerUnitTest, Player_SetCallback_001, TestSize.Level1) 
{  
    std::shared_ptr<PlayerCallbackTest> playerCallbackTest = std::make_shared<PlayerCallbackTest>();
    int32_t ret = player->SetPlayerCallback(playerCallbackTest);
    EXPECT_EQ(MSERR_OK, ret);
}

HWTEST_F(PlayerUnitTest, Player_SetCallback_002, TestSize.Level1) 
{
    int32_t ret = player->SetPlayerCallback(nullptr);
    EXPECT_EQ(MSERR_INVALID_VAL, ret);
}

HWTEST_F(PlayerUnitTest, Player_PrePare_001, TestSize.Level0) 
{
    EXPECT_EQ(MSERR_OK, player->SetSource(VIDEO_FILE1));
    std::shared_ptr<PlayerCallbackTest> playerCallbackTest = std::make_shared<PlayerCallbackTest>();
    EXPECT_EQ(MSERR_OK, player->SetPlayerCallback(playerCallbackTest));
    EXPECT_EQ(MSERR_OK, player->Prepare());
}

HWTEST_F(PlayerUnitTest, Player_PrePareAsync_001, TestSize.Level0) 
{
    EXPECT_EQ(MSERR_OK, player->SetSource(VIDEO_FILE1));
    std::shared_ptr<PlayerCallbackTest> playerCallbackTest = std::make_shared<PlayerCallbackTest>();
    EXPECT_EQ(MSERR_OK, player->SetPlayerCallback(playerCallbackTest));
    EXPECT_EQ(MSERR_OK, player->PrepareAsync());   
}

HWTEST_F(PlayerUnitTest, Player_SetVideoSurface_001, TestSize.Level0) 
{
    EXPECT_EQ(MSERR_OK, player->SetSource(VIDEO_FILE1));
    EXPECT_EQ(MSERR_OK, player->SetVideoSurface(GetVideoSurface()));
}

HWTEST_F(PlayerUnitTest, Player_Play_001, TestSize.Level0) 
{
    EXPECT_EQ(MSERR_OK, player->SetSource(VIDEO_FILE1));
    std::shared_ptr<PlayerCallbackTest> playerCallbackTest = std::make_shared<PlayerCallbackTest>();
    EXPECT_EQ(MSERR_OK, player->SetPlayerCallback(playerCallbackTest));
    sptr<Surface> videoSurface = GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player->PrepareAsync());
    (void)sleep(1);
    EXPECT_EQ(MSERR_OK, player->Play());
    EXPECT_TRUE(player->IsPlaying());
}

} // namespace Media
} // namespace OHOS
