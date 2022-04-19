/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef TEST_PLAYER_SETVOLUME_FUZZER_H
#define TEST_PLAYER_SETVOLUME_FUZZER_H

#define FUZZ_PROJECT_NAME "test_Player_SetVolume_fuzzer"
#include "player.h"
#include "window.h"
#include "surface.h"
#include "display_type.h"

namespace OHOS {
namespace Media {
bool FuzzPlayerSetVolume(const uint8_t* data, size_t size);

class TestPlayer : public NoCopyable {
public:
    TestPlayer();
    ~TestPlayer();
    sptr<Surface> GetVideoSurface();
    bool FuzzSetVolume(const uint8_t* data, size_t size);

private:
    int32_t SetFdSource(const std::string &path);
    sptr<Rosen::Window> previewWindow_ = nullptr;
    std::shared_ptr<Player> player_ = nullptr;
};

class TestPlayerCallback : public PlayerCallback {
public:
    TestPlayerCallback() = default;
    ~TestPlayerCallback() = default;
    DISALLOW_COPY_AND_MOVE(TestPlayerCallback);
    void OnError(PlayerErrorType errorType, int32_t errorCode);
    void OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody = {});
};
}
}
#endif

