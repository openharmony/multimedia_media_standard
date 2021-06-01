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

#include "playerModuleTest.h"
#include "audio_svc_manager.h"
#include "display_type.h"
#include "media_log.h"
#include "surface.h"
#include "window_manager.h"

#include <securec.h>

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

namespace PlayerTestConstants {
    const int SEEK_TIME = 5;
    const int SURFACE_QUEUE_SIZE = 5;
    const int WINDOW_HEIGHT = 720;
    const int WINDOW_WIDTH = 1280;
    const int TEST_VOLUME_LEVEL = 10;

    const string AUDIO_TEST_FILE_PATH1 = "file:///data/test.aac";
    const string AUDIO_TEST_FILE_PATH2 = "/data/test.aac";
    const string AUDIO_TEST_FILE_PATH3 = "file:///wrongpath/test.aac";
    const string AUDIO_TEST_FILE_PATH4 = "file:///data/empty.aac";
    const string AV_TEST_FILE_PATH = "file:///data/test.mp4";

    const string EMPTY_STRING = "";
    const string AUDIO_FORMAT = "audio/x-raw";
    const string VIDEO_FORMAT = "video/x-raw";
}

struct DummyPlayerCallback : public Player::PlayerCallback {
    void OnStateChanged(Player::State state,
                            const std::shared_ptr<Player> &player) const
    {
        MEDIA_INFO_LOG("State change");
    }
};

struct PlayTestCallback : public Player::PlayerCallback {
    void OnEndOfStream(const shared_ptr<Player> &player) const
    {
        PlayerModuleTest::m_isPlaybackCompletedFlag = FLAG1;
    }

    void OnStateChanged(Player::State state,
                            const std::shared_ptr<Player> &player) const
    {
        if (state == Player::PLAYER_STATE_PLAYING) {
            if (player->IsPlaying()) {
                PlayerModuleTest::m_isPlayingFlag = FLAG1;
            }
        }
    }
};

struct PauseTestCallback : public Player::PlayerCallback {
    void OnStateChanged(Player::State state,
                            const shared_ptr<Player> &player) const
    {
        if (!(PlayerModuleTest::m_callbackTestCompleted)) {
            if (state == Player::PLAYER_STATE_PLAYING) {
                if (player->Pause() == PLAYER_SUCCESS) {
                    PlayerModuleTest::m_callbackTestCompleted = true;
                    return;
                }
            }
        }

        if (state == Player::PLAYER_STATE_PAUSED) {
            if ((PlayerModuleTest::m_callbackTestCompleted)
                && (PlayerModuleTest::m_isPausedFlag == FLAG0)) {
                PlayerModuleTest::m_isPausedFlag = FLAG1;
                player->Pause();
            }
        }
    }
};

struct StopTestCallback : public Player::PlayerCallback {
    void OnStateChanged(Player::State state,
                            const shared_ptr<Player> &player) const
    {
        if (!(PlayerModuleTest::m_callbackTestCompleted)) {
            if (state == Player::PLAYER_STATE_PLAYING) {
                if (player->Stop() == PLAYER_SUCCESS) {
                    PlayerModuleTest::m_callbackTestCompleted = true;
                    return;
                }
            }
        }

        if (state == Player::PLAYER_STATE_STOPPED) {
            if ((PlayerModuleTest::m_callbackTestCompleted)
                && (PlayerModuleTest::m_isStoppedFlag == FLAG0)) {
                PlayerModuleTest::m_isStoppedFlag = FLAG1;
                player->Pause();
            }
        }
    }
};

struct RewindTestCallback : public Player::PlayerCallback {
    void OnStateChanged(Player::State state,
                            const shared_ptr<Player> &player) const
    {
        if (!(PlayerModuleTest::m_callbackTestCompleted)) {
            if (state == Player::PLAYER_STATE_PLAYING) {
                if (player->Rewind(PlayerTestConstants::SEEK_TIME, 0) == PLAYER_SUCCESS) {
                    PlayerModuleTest::m_callbackTestCompleted = true;
                }
            }
        }
    }

    void OnSeekDone(const shared_ptr<Player> &player) const
    {
        if (PlayerModuleTest::m_callbackTestCompleted) {
            PlayerModuleTest::m_isSeekDoneFlag = FLAG1;
        }
    }
};

struct GetCurrentTimeTestCallback : public Player::PlayerCallback {
    void OnStateChanged(Player::State state,
                            const shared_ptr<Player> &player) const
    {
        if (!(PlayerModuleTest::m_callbackTestCompleted)) {
            if (state == Player::PLAYER_STATE_PLAYING) {
                sleep(1);
                int64_t currentPosition = -1;
                if (player->GetCurrentTime(currentPosition) == PLAYER_SUCCESS) {
                    if (currentPosition >= 0) {
                        PlayerModuleTest::m_getCurrentPositionFlag = FLAG1;
                        PlayerModuleTest::m_callbackTestCompleted = true;
                    }
                }
            }
        }
    }
};

void PlayerModuleTest::SetUpTestCase(void) {}

void PlayerModuleTest::TearDownTestCase(void) {}

void PlayerModuleTest::SetUp() {}

void PlayerModuleTest::TearDown() {}

/*
 * Feature: Player
 * Function: Playback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Create player instance
 */
HWTEST(PlayerModuleTest, media_player_Create_test_001, TestSize.Level1)
{
    shared_ptr<Player> player = Player::Create(PlayerTestConstants::AUDIO_FORMAT,
                                               PlayerTestConstants::VIDEO_FORMAT);
    EXPECT_NE(nullptr, player);
}

/*
 * Feature: Player
 * Function: Playback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Set source
 */
HWTEST(PlayerModuleTest, media_player_SetSource_test_001, TestSize.Level1)
{
    shared_ptr<Player> player = Player::Create(PlayerTestConstants::AUDIO_FORMAT,
                                               PlayerTestConstants::VIDEO_FORMAT);
    EXPECT_NE(nullptr, player);
    int32_t ret = player->SetSource(PlayerTestConstants::AUDIO_TEST_FILE_PATH1);
    EXPECT_EQ(PLAYER_SUCCESS, ret);
}

/*
 * Feature: Player
 * Function: Playback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Set source with wrong parameters
 */
HWTEST(PlayerModuleTest, media_player_SetSource_test_002, TestSize.Level1)
{
    shared_ptr<Player> player = Player::Create(PlayerTestConstants::AUDIO_FORMAT,
                                               PlayerTestConstants::VIDEO_FORMAT);
    EXPECT_NE(nullptr, player);
    int32_t ret = player->SetSource(PlayerTestConstants::EMPTY_STRING);
    EXPECT_NE(PLAYER_SUCCESS, ret);
    ret = player->SetSource(PlayerTestConstants::AUDIO_TEST_FILE_PATH2);
    EXPECT_NE(PLAYER_SUCCESS, ret);
    ret = player->SetSource(PlayerTestConstants::AUDIO_TEST_FILE_PATH3);
    EXPECT_NE(PLAYER_SUCCESS, ret);
    ret = player->SetSource(PlayerTestConstants::AUDIO_TEST_FILE_PATH4);
    EXPECT_NE(PLAYER_SUCCESS, ret);
}

/*
 * Feature: Player
 * Function: Playback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Play
 */
HWTEST(PlayerModuleTest, media_player_Play_test_001, TestSize.Level1)
{
    shared_ptr<Player> player = Player::Create(PlayerTestConstants::AUDIO_FORMAT,
                                               PlayerTestConstants::VIDEO_FORMAT);
    EXPECT_NE(nullptr, player);
    int32_t ret = player->SetSource(PlayerTestConstants::AUDIO_TEST_FILE_PATH1);
    EXPECT_EQ(PLAYER_SUCCESS, ret);
    ret = player->Prepare();
    EXPECT_EQ(PLAYER_SUCCESS, ret);
    PlayTestCallback callback;
    player->SetPlayerCallback(callback);
    PlayerModuleTest::m_isPlaybackCompletedFlag = FLAG0;
    ret = player->Play();
    EXPECT_EQ(PLAYER_SUCCESS, ret);
    EXPECT_EQ(PlayerModuleTest::m_isPlaybackCompletedFlag, FLAG1);
}

/*
 * Feature: Player
 * Function: Playback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Is player playing
 */
HWTEST(PlayerModuleTest, media_player_IsPlaying_test_001, TestSize.Level1)
{
    shared_ptr<Player> player = Player::Create(PlayerTestConstants::AUDIO_FORMAT,
                                               PlayerTestConstants::VIDEO_FORMAT);
    EXPECT_NE(nullptr, player);
    int32_t ret = player->SetSource(PlayerTestConstants::AUDIO_TEST_FILE_PATH1);
    EXPECT_EQ(PLAYER_SUCCESS, ret);
    ret = player->Prepare();
    EXPECT_EQ(PLAYER_SUCCESS, ret);
    PlayTestCallback callback;
    player->SetPlayerCallback(callback);
    PlayerModuleTest::m_isPlayingFlag = FLAG0;
    ret = player->Play();
    EXPECT_EQ(PLAYER_SUCCESS, ret);
    EXPECT_EQ(PlayerModuleTest::m_isPlayingFlag, FLAG1);
}

/*
 * Feature: Player
 * Function: Playback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Playback pause
 */
HWTEST(PlayerModuleTest, media_player_Pause_test_001, TestSize.Level1)
{
    shared_ptr<Player> player = Player::Create(PlayerTestConstants::AUDIO_FORMAT,
                                               PlayerTestConstants::VIDEO_FORMAT);
    EXPECT_NE(nullptr, player);
    int32_t ret = player->SetSource(PlayerTestConstants::AUDIO_TEST_FILE_PATH1);
    EXPECT_EQ(PLAYER_SUCCESS, ret);
    ret = player->Prepare();
    EXPECT_EQ(PLAYER_SUCCESS, ret);
    PauseTestCallback callback;
    player->SetPlayerCallback(callback);
    PlayerModuleTest::m_callbackTestCompleted = false;
    PlayerModuleTest::m_isPausedFlag = FLAG0;
    ret = player->Play();
    EXPECT_EQ(PLAYER_SUCCESS, ret);
    EXPECT_EQ(PlayerModuleTest::m_isPausedFlag, FLAG1);
}

/*
 * Feature: Player
 * Function: Playback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Playback stop
 */
HWTEST(PlayerModuleTest, media_player_Stop_test_001, TestSize.Level1)
{
    shared_ptr<Player> player = Player::Create(PlayerTestConstants::AUDIO_FORMAT,
                                               PlayerTestConstants::VIDEO_FORMAT);
    EXPECT_NE(nullptr, player);
    int32_t ret = player->SetSource(PlayerTestConstants::AUDIO_TEST_FILE_PATH1);
    EXPECT_EQ(PLAYER_SUCCESS, ret);
    ret = player->Prepare();
    EXPECT_EQ(PLAYER_SUCCESS, ret);
    StopTestCallback callback;
    player->SetPlayerCallback(callback);
    PlayerModuleTest::m_callbackTestCompleted = false;
    PlayerModuleTest::m_isStoppedFlag = FLAG0;
    ret = player->Play();
    EXPECT_EQ(PLAYER_SUCCESS, ret);
    EXPECT_EQ(PlayerModuleTest::m_isStoppedFlag, FLAG1);
}

/*
 * Feature: Player
 * Function: Playback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Playback seek
 */
HWTEST(PlayerModuleTest, media_player_Rewind_test_001, TestSize.Level1)
{
    shared_ptr<Player> player = Player::Create(PlayerTestConstants::AUDIO_FORMAT,
                                               PlayerTestConstants::VIDEO_FORMAT);
    EXPECT_NE(nullptr, player);
    int32_t ret = player->SetSource(PlayerTestConstants::AUDIO_TEST_FILE_PATH1);
    EXPECT_EQ(PLAYER_SUCCESS, ret);
    ret = player->Prepare();
    EXPECT_EQ(PLAYER_SUCCESS, ret);
    RewindTestCallback callback;
    player->SetPlayerCallback(callback);
    PlayerModuleTest::m_callbackTestCompleted = false;
    PlayerModuleTest::m_isSeekDoneFlag = FLAG0;
    ret = player->Play();
    EXPECT_EQ(PLAYER_SUCCESS, ret);
    EXPECT_EQ(PlayerModuleTest::m_isSeekDoneFlag, FLAG1);
}

/*
 * Feature: Player
 * Function: Playback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Get Duration of the media file.
 */
HWTEST(PlayerModuleTest, media_player_GetDuration_test_001, TestSize.Level1)
{
    shared_ptr<Player> player = Player::Create(PlayerTestConstants::AUDIO_FORMAT,
                                               PlayerTestConstants::VIDEO_FORMAT);
    EXPECT_NE(nullptr, player);
    int32_t ret = player->SetSource(PlayerTestConstants::AUDIO_TEST_FILE_PATH1);
    EXPECT_EQ(PLAYER_SUCCESS, ret);
    ret = player->Prepare();
    EXPECT_EQ(PLAYER_SUCCESS, ret);
    DummyPlayerCallback callback;
    player->SetPlayerCallback(callback);
    ret = player->Play();
    EXPECT_EQ(PLAYER_SUCCESS, ret);
    int64_t duration = -1;
    ret = player->GetDuration(duration);
    EXPECT_EQ(true, (duration >= 0));
}

/*
 * Feature: Player
 * Function: Playback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Obtains the playback position.
 */
HWTEST(PlayerModuleTest, media_player_GetCurrentTime_test_001, TestSize.Level1)
{
    shared_ptr<Player> player = Player::Create(PlayerTestConstants::AUDIO_FORMAT,
                                               PlayerTestConstants::VIDEO_FORMAT);
    EXPECT_NE(nullptr, player);
    int32_t ret = player->SetSource(PlayerTestConstants::AUDIO_TEST_FILE_PATH1);
    EXPECT_EQ(PLAYER_SUCCESS, ret);
    ret = player->Prepare();
    EXPECT_EQ(PLAYER_SUCCESS, ret);
    GetCurrentTimeTestCallback callback;
    player->SetPlayerCallback(callback);
    PlayerModuleTest::m_callbackTestCompleted = false;
    PlayerModuleTest::m_getCurrentPositionFlag = FLAG0;
    ret = player->Play();
    EXPECT_EQ(PLAYER_SUCCESS, ret);
    EXPECT_EQ(PlayerModuleTest::m_getCurrentPositionFlag, FLAG1);
}

/*
 * Feature: Player
 * Function: Playback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Enable Single Loop
 */
HWTEST(PlayerModuleTest, media_player_IsSingleLooping_test_001, TestSize.Level1)
{
    shared_ptr<Player> player = Player::Create(PlayerTestConstants::AUDIO_FORMAT,
                                               PlayerTestConstants::VIDEO_FORMAT);
    EXPECT_NE(nullptr, player);
    bool flag = player->IsSingleLooping();
    EXPECT_EQ(false, flag);
    player->EnableSingleLooping(true);
    flag = player->IsSingleLooping();
    EXPECT_EQ(true, flag);
}

/*
 * Feature: Player
 * Function: Playback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Set video surface
 */
HWTEST(PlayerModuleTest, media_player_SetVideoSurface_test_001, TestSize.Level1)
{
    shared_ptr<Player> player = Player::Create(PlayerTestConstants::AUDIO_FORMAT,
                                               PlayerTestConstants::VIDEO_FORMAT);
    EXPECT_NE(nullptr, player);
    int32_t ret = player->SetSource(PlayerTestConstants::AV_TEST_FILE_PATH);
    EXPECT_EQ(PLAYER_SUCCESS, ret);
    ret = player->Prepare();
    EXPECT_EQ(PLAYER_SUCCESS, ret);
    PlayTestCallback callback;
    player->SetPlayerCallback(callback);
    WindowConfig config;
    (void)memset_s(&config, sizeof(WindowConfig), 0, sizeof(WindowConfig));
    config.height = PlayerTestConstants::WINDOW_HEIGHT;
    config.width = PlayerTestConstants::WINDOW_WIDTH;
    config.format = PIXEL_FMT_RGBA_8888;
    config.pos_x = 0;
    config.pos_y = 0;

    std::unique_ptr<Window> window = WindowManager::GetInstance()->CreateWindow(&config);
    window->GetSurface()->SetQueueSize(PlayerTestConstants::SURFACE_QUEUE_SIZE);

    sptr<Surface> producerSurface = window->GetSurface();
    producerSurface->SetUserData(SURFACE_STRIDE_ALIGNMENT, "8");
    producerSurface->SetUserData(SURFACE_FORMAT, std::to_string(PIXEL_FMT_RGBA_8888));
    ret = player->SetVideoSurface(producerSurface);
    EXPECT_EQ(PLAYER_SUCCESS, ret);
    PlayerModuleTest::m_isPlaybackCompletedFlag = FLAG0;
    ret = player->Play();
    EXPECT_EQ(PLAYER_SUCCESS, ret);
    EXPECT_EQ(PlayerModuleTest::m_isPlaybackCompletedFlag, FLAG1);
}

/*
 * Feature: Audio Service
 * Function: Volume
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Get and Set volume level through audio service manager
 */
HWTEST(PlayerModuleTest, media_player_GetVolume_test_001, TestSize.Level1)
{
    AudioSvcManager *audioServiceManager = AudioSvcManager::GetInstance();
    audioServiceManager->SetVolume(AudioSvcManager::AudioVolumeType::STREAM_MUSIC,
                                       PlayerTestConstants::TEST_VOLUME_LEVEL);
    int volume = audioServiceManager->GetVolume(AudioSvcManager::AudioVolumeType::STREAM_MUSIC);
    EXPECT_EQ(PlayerTestConstants::TEST_VOLUME_LEVEL, volume);
}
