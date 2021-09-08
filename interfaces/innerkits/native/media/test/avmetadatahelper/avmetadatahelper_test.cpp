/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License\n");
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

#include "avmetadatahelper_test.h"
#include <iostream>
#include <string>
#include "string_ex.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;

void AVMetadataHelperTest::GetMetadata(const std::string cmd)
{
    if (cmd == "md") {
        std::unordered_map<int32_t, std::string> metadataMap = avMetadataHelper_->ResolveMetadata();
        for (auto it = metadataMap.begin(); it != metadataMap.end(); it++) {
            cout << "key: " <<it->first << " metadata: "  << it->second.c_str() << endl;
        }
    } else if (cmd.find("md") != std::string::npos) {
        std::string keyStr = cmd.substr(cmd.find_last_of("md ") + 1);
        int32_t key = -1;
        if (!StrToInt(keyStr, key) || key < 0) {
            cout << "You need to configure the key parameter" << endl;
            return;
        }

        string metadata = avMetadataHelper_->ResolveMetadata(key);
        cout << "key: " << key << " metadata: " << metadata.c_str() << endl;
    }
}

void AVMetadataHelperTest::DoNext()
{
    cout << "Enter your step:" << endl;
    std::string cmd;
    while (std::getline(std::cin, cmd)) {
        if (cmd == "md" || cmd.find("md") != std::string::npos) {
            GetMetadata(cmd);
            continue;
        }

        if (cmd == "release") {
            avMetadataHelper_->Release();
            continue;
        }

        if (cmd == "quit" || cmd == "q") {
            break;
        }
    }
}

void AVMetadataHelperTest::TestCase(const std::string &pathOuter)
{
    avMetadataHelper_ = OHOS::Media::AVMetadataHelperFactory::CreateAVMetadataHelper();
    if (avMetadataHelper_ == nullptr) {
        cout << "avMetadataHelper_ is null" << endl;
        return;
    }

    string path;
    if (pathOuter == "") {
        cout << "Please enter the video/audio path: " << endl;
        (void)getline(cin, path);
    } else {
        path = pathOuter;
    }
    cout << "Path is " << path << endl;

    int32_t ret = avMetadataHelper_->SetSource(path);
    if (ret != 0) {
        cout << "SetSource fail" << endl;
        return;
    }

    DoNext();
}
