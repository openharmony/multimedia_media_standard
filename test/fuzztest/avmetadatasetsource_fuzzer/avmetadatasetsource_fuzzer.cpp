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
#include "image_type.h"
#include "avmetadatasetsource_fuzzer.h"

using namespace std;
using namespace OHOS;
using namespace Media;
using namespace PlayerTestParam;

namespace OHOS {
namespace Media {
AVMetadataSetSourceFuzzer::AVMetadataSetSourceFuzzer()
{
}

AVMetadataSetSourceFuzzer::~AVMetadataSetSourceFuzzer()
{
}

bool AVMetadataSetSourceFuzzer::FuzzAVMetadataSetSource(uint8_t *data, size_t size)
{
    constexpr int32_t USAGE_LIST = 2;
    avmetadata = AVMetadataHelperFactory::CreateAVMetadataHelper();
    cout << "start!" << endl;
    if (avmetadata == nullptr) {
        cout << "avmetadata is null" << endl;
        avmetadata->Release();
        return false;
    }

    const string path = "/data/test/media/H264_AAC.mp4";
    int32_t setsourcefd = open(path.c_str(), O_RDONLY);
    if (setsourcefd < 0) {
        cout << "Open file failed" << endl;
        (void)close(setsourcefd);
        avmetadata->Release();
        return false;
    }

    struct stat64 buffer;
    if (fstat64(setsourcefd, &buffer) != 0) {
        cout << "Get file state failed" << endl;
        (void)close(setsourcefd);
        avmetadata->Release();
        return false;
    }
    int64_t setsourcesize = static_cast<int64_t>(buffer.st_size);
    AVMetadataUsage usage[USAGE_LIST] {AVMetadataUsage::AV_META_USAGE_META_ONLY,
                                    AVMetadataUsage::AV_META_USAGE_PIXEL_MAP};
    int32_t setsourceusage = usage[ProduceRandomNumberCrypt() % USAGE_LIST];
    int32_t retSetsource = avmetadata->SetSource(setsourcefd,
        *reinterpret_cast<int64_t *>(data), setsourcesize, setsourceusage);
    if (retSetsource != 0) {
        cout << "expect SetSource fail!" << endl;
        (void)close(setsourcefd);
        avmetadata->Release();
        return true;
    }
    (void)close(setsourcefd);
    std::unordered_map<int32_t, std::string> retResolvenetadata = avmetadata->ResolveMetadata();

    if (retResolvenetadata.empty()) {
        cout << "expect avmetadata FetchFrameAtTime fail" << endl;
        avmetadata->Release();
        return true;
    }
    
    avmetadata->Release();
    cout << "success!" << endl;
    return true;
}
}

bool FuzzTestAVMetadataSetSource(uint8_t *data, size_t size)
{
    AVMetadataSetSourceFuzzer metadata;
    return metadata.FuzzAVMetadataSetSource(data, size);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzTestAVMetadataSetSource(data, size);
    return 0;
}