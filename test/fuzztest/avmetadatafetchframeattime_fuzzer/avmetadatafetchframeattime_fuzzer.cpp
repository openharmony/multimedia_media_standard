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

#include "avmetadatafetchframeattime_fuzzer.h"
#include <iostream>
#include "aw_common.h"
#include "string_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "window_option.h"
#include "image_type.h"


using namespace std;
using namespace OHOS;
using namespace Media;
using namespace PlayerTestParam;

AVMetadataFetchFrameAtTimeFuzzer::AVMetadataFetchFrameAtTimeFuzzer()
{
}

AVMetadataFetchFrameAtTimeFuzzer::~AVMetadataFetchFrameAtTimeFuzzer()
{
}

bool AVMetadataFetchFrameAtTimeFuzzer::FuzzAVMetadataFetchFrameAtTime(uint8_t *data, size_t size)
{
    constexpr int32_t avMetadataQueryOptionList = 4;
    constexpr int32_t avColorFormatList = 11;

    avmetadata = AVMetadataHelperFactory::CreateAVMetadataHelper();
    if (avmetadata == nullptr) {
        cout << "avmetadata is null" << endl;
        avmetadata->Release();
        return false;
    }

    const string path = "/data/test/resource/H264_AAC.mp4";
    if (MetaDataSetSource(path) != 0) {
        cout << "avmetadata SetSource file" << endl;
        avmetadata->Release();
        return false;
    }

    if (size >= sizeof(int64_t)) {
        int32_t nameAVMetadataQueryOption[avMetadataQueryOptionList] {
            AV_META_QUERY_NEXT_SYNC,
            AV_META_QUERY_PREVIOUS_SYNC,
            AV_META_QUERY_CLOSEST_SYNC,
            AV_META_QUERY_CLOSEST
        };

        int32_t optionFetchFrameAtTime = nameAVMetadataQueryOption[ProduceRandomNumberCrypt() % avMetadataQueryOptionList];
        PixelFormat colorFormats[avColorFormatList] {
            PixelFormat::UNKNOWN,
            PixelFormat::ARGB_8888,
            PixelFormat::RGB_565,
            PixelFormat::RGBA_8888,
            PixelFormat::BGRA_8888,
            PixelFormat::RGB_888,
            PixelFormat::ALPHA_8,
            PixelFormat::RGBA_F16,
            PixelFormat::NV21,
            PixelFormat::NV12,
            PixelFormat::CMYK
        };
        PixelFormat colorFormat = colorFormats[ProduceRandomNumberCrypt() % avColorFormatList];

        struct PixelMapParams pixelMapParams = {ProduceRandomNumberCrypt(), ProduceRandomNumberCrypt(), colorFormat};
        
        std::shared_ptr<PixelMap> retFetchFrameAtTime =
            avmetadata->FetchFrameAtTime(*reinterpret_cast<int64_t *>(data), optionFetchFrameAtTime, pixelMapParams);

        if (retFetchFrameAtTime != 0) {
            cout << "expect avmetadata FetchFrameAtTime fail" << endl;
            avmetadata->Release();
            return true;
        }
    }
    avmetadata->Release();
    return true;
}

bool OHOS::Media::FuzzTestAVMetadataFetchFrameAtTime(uint8_t *data, size_t size)
{
    AVMetadataFetchFrameAtTimeFuzzer metadata;
    return metadata.FuzzAVMetadataFetchFrameAtTime(data, size);
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::Media::FuzzTestAVMetadataFetchFrameAtTime(data, size);
    return 0;
}
