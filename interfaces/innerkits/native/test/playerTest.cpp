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

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <gst/gst.h>
#include <iostream>
#include <securec.h>
#include <unistd.h>

#include "display_type.h"
#include "media_log.h"
#include "player.h"
#include "surface.h"
#include "surface_type.h"
#include "window_manager.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;

namespace MediaTest {
    constexpr int SURFACE_QUEUE_SIZE = 5;
    const float VOLUME = 0.5f;
    const int FIRST_ARG_IDX = 1;
    const int SECOND_ARG_IDX = 2;
    const int SECOND_ARG = 3;
    const int HEIGHT = 720;
    const int WIDTH = 1280;
    const int SEEK_TIME_SECONDS = 5;
}

static bool g_testLoop = true;

#define LOG(fmt, ...) MEDIA_INFO_LOG("[%d][%d]: " fmt, getpid(), __LINE__, ##__VA_ARGS__)

class PlayerTest {
public:
    struct MyPlayerCallback : public OHOS::Media::Player::PlayerCallback {
        void OnError(int32_t errorType, int32_t errorCode,
            const std::shared_ptr<OHOS::Media::Player> &player) const
        {
            MEDIA_INFO_LOG("Error received");
            g_testLoop = false;
        }

        void OnEndOfStream(const std::shared_ptr<OHOS::Media::Player> &player) const
        {
            MEDIA_INFO_LOG("End of stream received");
            g_testLoop = false;
        }

        void OnStateChanged(OHOS::Media::Player::State state,
                                const std::shared_ptr<OHOS::Media::Player> &player) const
        {
            PrintState(state);
            if (player != nullptr) {
                bool isPlaying = player->IsPlaying();
                MEDIA_INFO_LOG("isPlaying: %d", isPlaying);
            }
        }

        void OnMessage(int32_t type, int32_t extra,
                       const std::shared_ptr<OHOS::Media::Player> &player) const
        {
            MEDIA_INFO_LOG("Player info: %d", type);
        }

        void PrintState(OHOS::Media::Player::State state) const
        {
            switch (state) {
                case OHOS::Media::Player::PLAYER_STATE_STOPPED:
                    MEDIA_INFO_LOG("State: Stopped");
                    break;

                case OHOS::Media::Player::PLAYER_STATE_BUFFERING:
                    MEDIA_INFO_LOG("State: Buffering");
                    break;

                case OHOS::Media::Player::PLAYER_STATE_PAUSED:
                    MEDIA_INFO_LOG("State: Paused");
                    break;

                case OHOS::Media::Player::PLAYER_STATE_PLAYING:
                    MEDIA_INFO_LOG("State: Playing");
                    break;

                default:
                    MEDIA_INFO_LOG("Invalid state");
                    break;
            }
        }

        void OnSeekDone(const std::shared_ptr<Player> &player) const
        {
            MEDIA_INFO_LOG("Seek done callback");
            int64_t time = -1;
            player->GetCurrentTime(time);
            MEDIA_INFO_LOG("Time after seek: %lld", time);
        }
    };

    static int TestSetVideoSurface(sptr<Surface> producerSurface, std::shared_ptr<OHOS::Media::Player> player)
    {
        if (player == nullptr) {
            MEDIA_ERR_LOG("player is null");
            return 0;
        }
        int ret = player->SetVideoSurface(producerSurface);
        return ret;
    }

    int32_t TestPlayerAPI(int argc, char *argv[]) const
    {
        MEDIA_INFO_LOG("TestPlayerAPI start ");
        std::string audioFormat = "audio/x-raw";
        std::string videoFormat = "video/x-raw";
        float volume = MediaTest::VOLUME;

        if (argv == nullptr) {
            MEDIA_ERR_LOG("argv is null");
            return 0;
        }
        if (argc >= MediaTest::SECOND_ARG) {
            volume = strtof(argv[MediaTest::SECOND_ARG_IDX], nullptr);
            MEDIA_ERR_LOG("Got the Volume Input: %0.2f", volume);
        }

        std::shared_ptr<OHOS::Media::Player> player
                                = OHOS::Media::Player::Create(audioFormat, videoFormat);

        WindowConfig config;
        (void)memset_s(&config, sizeof(WindowConfig), 0, sizeof(WindowConfig));
        config.height = MediaTest::HEIGHT;
        config.width = MediaTest::WIDTH;
        config.format = PIXEL_FMT_RGBA_8888;
        config.pos_x = 0;
        config.pos_y = 0;

        std::unique_ptr<Window> window = WindowManager::GetInstance()->CreateWindow(&config);

        if (window != nullptr) {
            window->GetSurface()->SetQueueSize(MediaTest::SURFACE_QUEUE_SIZE);
            sptr<Surface> producerSurface = window->GetSurface();
            producerSurface->SetUserData(SURFACE_STRIDE_ALIGNMENT, "8");
            producerSurface->SetUserData(SURFACE_FORMAT, std::to_string(PIXEL_FMT_RGBA_8888));

            if (player == nullptr) {
                MEDIA_ERR_LOG("player is null");
                return -1;
            }

            int ret = player->SetSource(argv[MediaTest::FIRST_ARG_IDX]);
            if (ret != 0) {
                MEDIA_ERR_LOG("player set source failed");
                return ret;
            }

            ret = player->Prepare();
            if (ret != 0) {
                MEDIA_ERR_LOG("player prepare failed");
                return ret;
            }

            MyPlayerCallback callback;
            player->SetPlayerCallback(callback);

            ret = TestSetVideoSurface(producerSurface, player);
            LOG("TestSetVideoSurface returned %d", ret);
            player->IsSingleLooping();
            player->SetVolume(volume, volume);
            player->Play();

            MEDIA_INFO_LOG("Start loop");
            char input;
            while (g_testLoop) {
                MEDIA_INFO_LOG("Wait for input");
                cin >> input;
                switch (input) {
                    case 'p': {
                        MEDIA_INFO_LOG("pause pressed");
                        player->Pause();
                        break;
                    }
                    case 's': {
                        MEDIA_INFO_LOG("stop pressed");
                        player->Stop();
                        break;
                    }
                    case 'd': {
                        MEDIA_INFO_LOG("get duration pressed");
                        int64_t duration = -1;
                        player->GetDuration(duration);
                        break;
                    }
                    case 't': {
                        MEDIA_INFO_LOG("get time pressed");
                        int64_t time = -1;
                        player->GetCurrentTime(time);
                        break;
                    }
                    case 'r': {
                        MEDIA_INFO_LOG("seek pressed");
                        int64_t time = -1;
                        player->GetCurrentTime(time);
                        player->Rewind(MediaTest::SEEK_TIME_SECONDS, 0);
                        player->GetCurrentTime(time);
                        break;
                    }
                    case 'l': {
                        MEDIA_INFO_LOG("single loop pressed");
                        bool isSingleLoop = player->IsSingleLooping();
                        MEDIA_INFO_LOG("Is current single looping enabled: %d", isSingleLoop);
                        player->EnableSingleLooping(!isSingleLoop);
                        break;
                    }
                    case 'q': {
                        MEDIA_INFO_LOG("Exit test");
                        player->Stop();
                        player->Release();
                        g_testLoop = false;
                        break;
                    }
                    default:
                        break;
                }
            }
            MEDIA_INFO_LOG("Loop exit\n");
        }

        MEDIA_INFO_LOG("player test out");

        return 0;
    }
};

int main(int argc, char *argv[])
{
    MEDIA_INFO_LOG("player test in");

    if ((argv == nullptr) || (argc < MediaTest::SECOND_ARG_IDX)) {
        MEDIA_ERR_LOG("argv is null");
        return 0;
    }

    MEDIA_INFO_LOG("argc=%d, argv[0]=%s, argv[1]=%s", argc, argv[0], argv[1]);
    PlayerTest testObj;
    int ret = testObj.TestPlayerAPI(argc, argv);

    return ret;
}
