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

#include "test_player_setvolume_fuzzer.h"
#include <iostream>
#include "string_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "ui/rs_surface_node.h"
#include "window_option.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;

TestPlayerSetVolumeFuzz::TestPlayerSetVolumeFuzz()
{
}

TestPlayerSetVolumeFuzz::~TestPlayerSetVolumeFuzz()
{
}

bool TestPlayerSetVolumeFuzz::FuzzSetVolume(uint8_t* data, size_t size)
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
    if (size >= sizeof(float)) {
       ret = player_->SetVolume(*reinterpret_cast<float *>(data), *reinterpret_cast<float *>(data));
        sleep(1); 
    } 

    ret = player_->Release();
    if (ret != 0) {
        cout << "Release fail" << endl;
        return false;
    }
    return true;
}

bool OHOS::Media::FuzzPlayerSetVolume(uint8_t* data, size_t size)
{
    auto player = std::make_unique<TestPlayerSetVolumeFuzz>();
    if (player == nullptr) {
        cout << "player is null" << endl;
        return 0;
    }
    return player->FuzzSetVolume(data, size);
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::Media::FuzzPlayerSetVolume(data, size);
    return 0;
}

