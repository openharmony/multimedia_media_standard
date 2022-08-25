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

#include "avmetadataresolvemetadata_fuzzer.h"
#include <iostream>
#include "string_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "window_option.h"
#include "image_type.h"

using namespace std;
using namespace OHOS;
using namespace Media;

namespace OHOS {
namespace Media {
AVMetadataResolveMetadataFuzzer::AVMetadataResolveMetadataFuzzer()
{
}

AVMetadataResolveMetadataFuzzer::~AVMetadataResolveMetadataFuzzer()
{
}

bool AVMetadataResolveMetadataFuzzer::FuzzAVMetadataResolveMetadata(uint8_t *data, size_t size)
{
    constexpr int32_t AV_METADATA_CODELIST = 17;

    avmetadata = AVMetadataHelperFactory::CreateAVMetadataHelper();
    if (avmetadata == nullptr) {
        cout << "avmetadata is null" << endl;
        avmetadata->Release();
        return false;
    }

    const string path = "/data/test/media/H264_AAC.mp4";
    int32_t retMetadataSetsource = MetaDataSetSource(path);
    if (retMetadataSetsource != 0) {
        cout << "avmetadata SetSource file" << endl;
        avmetadata->Release();
        return false;
    }

    int32_t avMetadataCodes[AV_METADATA_CODELIST] {
        AV_KEY_ALBUM,
        AV_KEY_ALBUM_ARTIST,
        AV_KEY_ARTIST,
        AV_KEY_AUTHOR,
        AV_KEY_DATE_TIME,
        AV_KEY_COMPOSER,
        AV_KEY_DURATION,
        AV_KEY_GENRE,
        AV_KEY_HAS_AUDIO,
        AV_KEY_HAS_VIDEO,
        AV_KEY_MIME_TYPE,
        AV_KEY_NUM_TRACKS,
        AV_KEY_SAMPLE_RATE,
        AV_KEY_TITLE,
        AV_KEY_VIDEO_HEIGHT,
        AV_KEY_VIDEO_WIDTH,
        AV_KEY_VIDEO_ORIENTATION
    };
    int32_t keyParameter = avMetadataCodes[*reinterpret_cast<int64_t *>(data) % AV_METADATA_CODELIST];
    std::string retResolvemetadata = avmetadata->ResolveMetadata(keyParameter);
    avmetadata->Release();
    return true;
}
}

bool FuzzTestAVMetadataResolveMetadata(uint8_t *data, size_t size)
{
    AVMetadataResolveMetadataFuzzer metadata;
    return metadata.FuzzAVMetadataResolveMetadata(data, size);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzTestAVMetadataResolveMetadata(data, size);
    return 0;
}
