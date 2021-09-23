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

#include <climits>
#include <iostream>
#include "player_test.h"
#include "recorder_test.h"
#include "avmetadatahelper_test.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace MediaTest;
using namespace std;

static int TestPlayer(const string &path)
{
    auto playerTest = std::make_unique<PlayerTest>();
    if (playerTest == nullptr) {
        cout << "playerTest is null" << endl;
        return 0;
    }
    playerTest->TestCase(path);
    cout << "test player end" << endl;
    return 0;
}

static int TestRecorder()
{
    auto recorderTest = std::make_unique<RecorderTest>();
    if (recorderTest == nullptr) {
        cout << "recorderTest is null" << endl;
        return 0;
    }
    recorderTest->TestCase();
    cout << "test recorder end" << endl;
    return 0;
}

static int TestAVMetadataHelper(const string &path)
{
    auto avMetadataHelperTest = std::make_unique<AVMetadataHelperTest>();
    if (avMetadataHelperTest == nullptr) {
        cout << "avMetadataHelperTest is null" << endl;
        return 0;
    }
    avMetadataHelperTest->TestCase(path);
    cout << "test avMetadataHelper end" << endl;
    return 0;
}

int main(int argc, char *argv[])
{
    constexpr int minRequiredArgCount = 2;
    string path;
    if (argc >= minRequiredArgCount && argv[1] != nullptr) {
        path = argv[1];
    }
    cout << "Please select a test scenario number(defult player): " << endl;
    cout << "0:player" << endl;
    cout << "1:recorder" << endl;
    cout << "2:avmetadatahelper" << endl;
    string mode;
    (void)getline(cin, mode);
    if (mode == "" || mode == "0") {
        (void)TestPlayer(path);
    } else if (mode == "1") {
        (void)TestRecorder();
    } else if (mode == "2") {
        (void)TestAVMetadataHelper(path);
    } else {
        cout << "no that selection" << endl;
    }
    return 0;
}
