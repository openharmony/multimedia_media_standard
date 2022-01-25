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
#include "player_demo.h"
#include "recorder_demo.h"
#include "avmetadatahelper_demo.h"
#include "avcodeclist_demo.h"
#include "avcodec_venc_demo.h"
#include "avcodec_vdec_demo.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace MediaDemo;
using namespace std;

static int RunPlayer(const string &path)
{
    auto player = std::make_unique<PlayerDemo>();
    if (player == nullptr) {
        cout << "player is null" << endl;
        return 0;
    }
    player->RunCase(path);
    cout << "demo player end" << endl;
    return 0;
}

static int RunCodecList(const string &path)
{
    auto avCodecList = std::make_unique<AVCodecListDemo>();
    if (avCodecList == nullptr) {
        cout << "avCodecList is null" << endl;
        return 0;
    }
    avCodecList->RunCase(path);
    cout << "demo avCodecList end" << endl;
    return 0;
}

static int RunRecorder()
{
    auto recorder = std::make_unique<RecorderDemo>();
    if (recorder == nullptr) {
        cout << "recorder is null" << endl;
        return 0;
    }
    recorder->RunCase();
    cout << "demo recorder end" << endl;
    return 0;
}

static int RunAVMetadataHelper(const string &path)
{
    auto avMetadataHelper = std::make_unique<AVMetadataHelperDemo>();
    if (avMetadataHelper == nullptr) {
        cout << "avMetadataHelper is null" << endl;
        return 0;
    }
    avMetadataHelper->RunCase(path);
    cout << "demo avMetadataHelper end" << endl;
    return 0;
}

static int RunVideoEncoder()
{
    auto venc = std::make_unique<VEncDemo>();
    if (venc == nullptr) {
        cout << "videoencoder is null" << endl;
        return 0;
    }
    venc->RunCase();
    cout << "demo videoencoder end" << endl;
    return 0;
}

static int RunVideoDecoder()
{
    auto vdec = std::make_unique<VDecDemo>();
    if (vdec == nullptr) {
        cout << "videodecoder is null" << endl;
        return 0;
    }
    vdec->RunCase();
    cout << "demo videodecoder end" << endl;
    return 0;
}

int main(int argc, char *argv[])
{
    constexpr int minRequiredArgCount = 2;
    string path;
    if (argc >= minRequiredArgCount && argv[1] != nullptr) {
        path = argv[1];
    }
    cout << "Please select a demo scenario number(defult player): " << endl;
    cout << "0:player" << endl;
    cout << "1:recorder" << endl;
    cout << "2:avmetadatahelper" << endl;
    cout << "3:codeclist" << endl;
    cout << "4:videoencoder" << endl;
    cout << "5:videodecoder" << endl;
    string mode;
    (void)getline(cin, mode);
    if (mode == "" || mode == "0") {
        (void)RunPlayer(path);
    } else if (mode == "1") {
        (void)RunRecorder();
    } else if (mode == "2") {
        (void)RunAVMetadataHelper(path);
    } else if (mode == "3") {
        (void)RunCodecList(path);
    } else if (mode == "4") {
        (void)RunVideoEncoder();
    } else if (mode == "5") {
        (void)RunVideoDecoder();
    } else {
        cout << "no that selection" << endl;
    }
    return 0;
}
