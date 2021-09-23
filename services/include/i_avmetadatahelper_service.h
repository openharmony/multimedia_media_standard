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

#ifndef IAVMETADATAHELPER_SERVICE_H
#define IAVMETADATAHELPER_SERVICE_H

#include "avmetadatahelper.h"
#include "avsharedmemory.h"

namespace OHOS {
namespace Media {
struct OutputFrame {
public:
    OutputFrame(int32_t width, int32_t height, int32_t bytesPerPixel)
        : width_(width), height_(height), bytesPerPixel_(bytesPerPixel),
          size_(width_ * height_ * bytesPerPixel_)
    {
    }

    int32_t GetFlattenedSize()
    {
        return sizeof(OutputFrame) + size_;
    }

    uint8_t *GetFlattenedData()
    {
        return reinterpret_cast<uint8_t *>(this) + sizeof(OutputFrame);
    }

    int32_t width_;
    int32_t height_;
    int32_t bytesPerPixel_;
    int32_t size_;
};

struct OutputConfiguration {
    int32_t dstWidth = -1;
    int32_t dstHeight = -1;
    int32_t colorFormat = PIXEL_FMT_RGB_565;
};

class IAVMetadataHelperService {
public:
    virtual ~IAVMetadataHelperService() = default;
    virtual int32_t SetSource(const std::string &uri, int32_t usage) = 0;
    virtual std::string ResolveMetadata(int32_t key) = 0;
    virtual std::unordered_map<int32_t, std::string> ResolveMetadata() = 0;
    virtual std::shared_ptr<AVSharedMemory> FetchFrameAtTime(
        int64_t timeUs, int32_t option, OutputConfiguration param) = 0;
    virtual void Release() = 0;
};
}
}

#endif