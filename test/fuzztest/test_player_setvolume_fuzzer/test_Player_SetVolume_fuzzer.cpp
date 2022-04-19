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

#include <iostream>
#include <stdint.h>
#include "test_player_setvolume_fuzzer.h"
#include "string_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "ui/rs_surface_node.h"
#include "window_option.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;

void TestPlayerCallback::OnError(PlayerErrorType errorType, int32_t errorCode)
{
    (void)errorType;
    cout << "Error received, errorCode:" << MSErrorToString(static_cast<MediaServiceErrCode>(errorCode)) << endl;
}

void TestPlayerCallback::OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    (void)infoBody;
    switch (type) {
        case INFO_TYPE_SEEKDONE:
            cout << "PlayerCallback: OnSeekDone currentPositon is " << extra << endl;
            break;
        case INFO_TYPE_SPEEDDONE:
            cout << "PlayerCallback: SpeedDone " << endl;
            break;
        case INFO_TYPE_EOS:
            cout << "PlayerCallback: OnEndOfStream isLooping is " << extra << endl;
            break;
        case INFO_TYPE_BUFFERING_UPDATE:
            break;
        case INFO_TYPE_STATE_CHANGE:
            break;
        case INFO_TYPE_POSITION_UPDATE:
            cout << "OnPositionUpdated position is " << extra << endl;
            break;
        case INFO_TYPE_MESSAGE:
            cout << "PlayerCallback: OnMessage is " << extra << endl;
            break;
        case INFO_TYPE_RESOLUTION_CHANGE:
            break;
        case INFO_TYPE_VOLUME_CHANGE:
            cout << "PlayerCallback: volume changed" << endl;
            break;
        default:
            break;
    }
}
TestPlayer::TestPlayer()
{
}

TestPlayer::~TestPlayer()
{
    if (previewWindow_ != nullptr) {
        previewWindow_->Destroy();
        previewWindow_ = nullptr;
    }
}
sptr<Surface> TestPlayer::GetVideoSurface()
{
    sptr<Rosen::WindowOption> option = new Rosen::WindowOption();
    option->SetWindowRect({ 0, 0, 640, 480 });
    option->SetWindowType(Rosen::WindowType::WINDOW_TYPE_APP_LAUNCHING);
    option->SetWindowMode(Rosen::WindowMode::WINDOW_MODE_FLOATING);
    previewWindow_ = Rosen::Window::Create("xcomponent_window_fuzztest", option);
    if (previewWindow_ == nullptr || previewWindow_->GetSurfaceNode() == nullptr) {
        cout << "previewWindow_ is nullptr" << endl;
        return nullptr;
    }
    previewWindow_->Show();
    return previewWindow_->GetSurfaceNode()->GetSurface();
}

int32_t TestPlayer::SetFdSource(const string &path)
{
    int32_t fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        cout << "Open file failed" << endl;
        return -1;
    }
    int32_t offset = 0;

    struct stat64 buffer;
    if (fstat64(fd, &buffer) != 0) {
        cout << "Get file state failed" << endl;
        return -1;
    }
    int64_t length = static_cast<int64_t>(buffer.st_size);
    cout << "fd = " << fd << ", offset = " << offset << ", length = " << length << endl;

    int32_t ret = player_->SetSource(fd, offset, length);
    if (ret != 0) {
        cout << "SetSource fail" << endl;
        return -1;
    }
    (void)close(fd);
    return 0;
}

bool TestPlayer::FuzzSetVolume(const uint8_t* data, size_t size)
{
    player_ = OHOS::Media::PlayerFactory::CreatePlayer();
    if (player_ == nullptr) {
        cout << "player_ is null" << endl;
        return false;
    }
    std::shared_ptr<TestPlayerCallback> cb = std::make_shared<TestPlayerCallback>();
    int32_t ret = player_->SetPlayerCallback(cb);
    if (ret != 0) {
        cout << "SetPlayerCallback fail" << endl;
    }
    const string path = "/data/media/H264_AAC.mp4";
    ret = SetFdSource(path);
    if (ret != 0) {
        cout << "SetFdSource fail" << endl;
        return false;
    }
    sptr<Surface> producerSurface = nullptr;
    producerSurface = GetVideoSurface();
    ret = player_->SetVideoSurface(producerSurface);
    if (ret != 0) {
        cout << "SetVideoSurface fail" << endl;
    }

    ret = player_->PrepareAsync();
    if (ret != 0) {
        cout << "PrepareAsync fail" << endl;
        return false;
    }
    sleep(1);
    ret = player_->Play();
    if (ret != 0) {
        cout << "Play fail" << endl;
        return false;
    }
    ret = player_->SetVolume(static_cast<float>(*data), static_cast<float>(*data));
    sleep(1);
    ret = player_->Release();
    if (ret != 0) {
        cout << "Release fail" << endl;
        return false;
    }
    return true;
}

bool OHOS::Media::FuzzPlayerSetVolume(const uint8_t* data, size_t size)
{
    auto player = std::make_unique<TestPlayer>();
    if (player == nullptr) {
        cout << "player is null" << endl;
        return 0;
    }
    return player->FuzzSetVolume(data, size);
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::Media::FuzzPlayerSetVolume(data, size);
    return 0;
}

