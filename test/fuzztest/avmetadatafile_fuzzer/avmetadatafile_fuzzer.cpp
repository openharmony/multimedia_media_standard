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
#include "aw_common.h"
#include "string_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "window_option.h"
#include "avmetadatafile_fuzzer.h"

using namespace std;
using namespace OHOS;
using namespace Media;
using namespace PlayerTestParam;

AVMetadataFileFuzzer::AVMetadataFileFuzzer()
{
}

AVMetadataFileFuzzer::~AVMetadataFileFuzzer()
{
}

bool AVMetadataFileFuzzer::FuzzAVMetadataFile(uint8_t *data, size_t size)
{
    avmetadata = OHOS::Media::AVMetadataHelperFactory::CreateAVMetadataHelper();
    cout << "start!" << endl;
    if (avmetadata == nullptr) {
        cout << "avmetadata is null" << endl;
        avmetadata->Release();
        return false;
    }

    const string path = "/data/test/media/fuzztest.mp4";

    int32_t retWritefile = WriteDataToFile(path, data, size);
    if (retWritefile != 0) {
        cout << "avmetadata WriteDataToFile fail!" << endl;
        avmetadata->Release();
        return false;
    }

    int32_t retMetadatasetsource = MetaDataSetSource(path);
    if (retMetadatasetsource != 0) {
        cout << "expect avmetadata SetSource file" << endl;
        avmetadata->Release();
        return true;
    }

    std::unordered_map<int32_t, std::string> retresolve = avmetadata->ResolveMetadata();
    if (retresolve.empty()) {
        cout << "expext avmetadata ResolveMetadata file" << endl;
        avmetadata->Release();
        return true;
    }

    std::shared_ptr<AVSharedMemory> retFetchartpicture = avmetadata->FetchArtPicture();
    if (retFetchartpicture == nullptr) {
        cout << "expect avmetadata FetchArtPicture file" << endl;
        avmetadata->Release();
        return true;
    }
    
    avmetadata->Release();
    cout << "success!" << endl;
    return true;
}

bool OHOS::Media::FuzzTestavMetadataFile(uint8_t *data, size_t size)
{
    AVMetadataFileFuzzer metadata;
    return metadata.FuzzAVMetadataFile(data, size);
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::Media::FuzzTestavMetadataFile(data, size);
    return 0;
}