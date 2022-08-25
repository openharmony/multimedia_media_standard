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

#include "playerfile_fuzzer.h"
#include <iostream>
#include "string_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "ui/rs_surface_node.h"
#include "window_option.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;

PlayerFileFuzzer::PlayerFileFuzzer()
{
}

PlayerFileFuzzer::~PlayerFileFuzzer()
{
}

bool PlayerFileFuzzer::FuzzFile(const uint8_t* data, size_t size)
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
        return false;
    }
    const string path = "/data/test/media/fuzztest.mp4";
    ret = WriteDataToFile(path, data, size);
    if (ret != 0) {
        cout << "WriteDataToFile fail" << endl;
        return false;
    }
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
        return true;
    }
    sleep(1);
    ret = player_->Play();
    if (ret != 0) {
        cout << "Play fail" << endl;
    }
    ret = player_->Seek(3000, SEEK_NEXT_SYNC); // seek 3000 ms
    if (ret != 0) {
        cout << "Seek fail" << endl;
    }
    ret = player_->Release();
    if (ret != 0) {
        cout << "Release fail" << endl;
        return false;
    }
    return true;
}

int32_t OHOS::Media::WriteDataToFile(const string &path, const uint8_t* data, size_t size)
{
    FILE *file = nullptr;
    file = fopen(path.c_str(), "w+");
    if (file == nullptr) {
        std::cout << "[fuzz] open file fstab.test failed";
        return -1;
    }
    if (fwrite(data, 1, size, file) != size) {
        std::cout << "[fuzz] write data failed";
        (void)fclose(file);
        return -1;
    }
    (void)fclose(file);
    return 0;
}

bool OHOS::Media::FuzzPlayerFile(const uint8_t* data, size_t size)
{
    auto player = std::make_unique<PlayerFileFuzzer>();
    if (player == nullptr) {
        cout << "player is null" << endl;
        return 0;
    }
    return player->FuzzFile(data, size);
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::Media::FuzzPlayerFile(data, size);
    return 0;
}

