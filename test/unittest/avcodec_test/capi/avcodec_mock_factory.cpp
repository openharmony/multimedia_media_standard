/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "avcodec_mock.h"
#include "avformat_capi_mock.h"
#include "avmemory_capi_mock.h"
#include "surface_capi_mock.h"
#include "audiodec_capi_mock.h"
#include "audioenc_capi_mock.h"
#include "videodec_capi_mock.h"
#include "videoenc_capi_mock.h"

namespace OHOS {
namespace Media {
std::shared_ptr<VideoDecMock> AVCodecMockFactory::CreateVideoDecMockByMime(const std::string &mime)
{
    OH_AVCodec *videoDec = OH_VideoDecoder_CreateByMime(mime.c_str());
    if (videoDec != nullptr) {
        return std::make_shared<VideoDecCapiMock>(videoDec);
    }
    return nullptr;
}

std::shared_ptr<VideoDecMock> AVCodecMockFactory::CreateVideoDecMockByName(const std::string &name)
{
    OH_AVCodec *videoDec = OH_VideoDecoder_CreateByName(name.c_str());
    if (videoDec != nullptr) {
        return std::make_shared<VideoDecCapiMock>(videoDec);
    }
    return nullptr;
}

std::shared_ptr<VideoEncMock> AVCodecMockFactory::CreateVideoEncMockByMime(const std::string &mime)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(mime.c_str());
    if (videoEnc != nullptr) {
        return std::make_shared<VideoEncCapiMock>(videoEnc);
    }
    return nullptr;
}

std::shared_ptr<VideoEncMock> AVCodecMockFactory::CreateVideoEncMockByName(const std::string &name)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByName(name.c_str());
    if (videoEnc != nullptr) {
        return std::make_shared<VideoEncCapiMock>(videoEnc);
    }
    return nullptr;
}

std::shared_ptr<AudioDecMock> AVCodecMockFactory::CreateAudioDecMockByMime(const std::string &mime)
{
    OH_AVCodec *audioDec = OH_AudioDecoder_CreateByMime(mime.c_str());
    if (audioDec != nullptr) {
        return std::make_shared<AudioDecCapiMock>(audioDec);
    }
    return nullptr;
}

std::shared_ptr<AudioDecMock> AVCodecMockFactory::CreateAudioDecMockByName(const std::string &name)
{
    OH_AVCodec *audioDec = OH_AudioDecoder_CreateByName(name.c_str());
    if (audioDec != nullptr) {
        return std::make_shared<AudioDecCapiMock>(audioDec);
    }
    return nullptr;
}

std::shared_ptr<AudioEncMock> AVCodecMockFactory::CreateAudioEncMockByMime(const std::string &mime)
{
    OH_AVCodec *audioEnc = OH_AudioEncoder_CreateByMime(mime.c_str());
    if (audioEnc != nullptr) {
        return std::make_shared<AudioEncCapiMock>(audioEnc);
    }
    return nullptr;
}

std::shared_ptr<AudioEncMock> AVCodecMockFactory::CreateAudioEncMockByName(const std::string &name)
{
    OH_AVCodec *audioEnc = OH_AudioEncoder_CreateByName(name.c_str());
    if (audioEnc != nullptr) {
        return std::make_shared<AudioEncCapiMock>(audioEnc);
    }
    return nullptr;
}

std::shared_ptr<FormatMock> AVCodecMockFactory::CreateFormat()
{
    return std::make_shared<AVFormatCapiMock>();
}

std::shared_ptr<SurfaceMock> AVCodecMockFactory::CreateSurface()
{
    return std::make_shared<SurfaceCapiMock>();
}
}
}