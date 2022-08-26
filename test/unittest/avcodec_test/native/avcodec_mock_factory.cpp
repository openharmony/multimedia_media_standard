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
#include "avcodec_list_native_mock.h"
#include "enum_native_mock.h"
#include "avcodec_info_native_mock.h"
#include "avformat_native_mock.h"
#include "avmemory_native_mock.h"
#include "surface_native_mock.h"
#include "audiodec_native_mock.h"
#include "audioenc_native_mock.h"
#include "videodec_native_mock.h"
#include "videoenc_native_mock.h"

namespace OHOS {
namespace Media {
std::shared_ptr<VideoDecMock> AVCodecMockFactory::CreateVideoDecMockByMime(const std::string &mime)
{
    auto videoDec = VideoDecoderFactory::CreateByMime(mime);
    if (videoDec != nullptr) {
        return std::make_shared<VideoDecNativeMock>(videoDec);
    }
    return nullptr;
}

std::shared_ptr<VideoDecMock> AVCodecMockFactory::CreateVideoDecMockByName(const std::string &name)
{
    auto videoDec = VideoDecoderFactory::CreateByName(name);
    if (videoDec != nullptr) {
        return std::make_shared<VideoDecNativeMock>(videoDec);
    }
    return nullptr;
}

std::shared_ptr<VideoEncMock> AVCodecMockFactory::CreateVideoEncMockByMime(const std::string &mime)
{
    auto videoEnc = VideoEncoderFactory::CreateByMime(mime);
    if (videoEnc != nullptr) {
        return std::make_shared<VideoEncNativeMock>(videoEnc);
    }
    return nullptr;
}

std::shared_ptr<VideoEncMock> AVCodecMockFactory::CreateVideoEncMockByName(const std::string &name)
{
    auto videoEnc = VideoEncoderFactory::CreateByName(name);
    if (videoEnc != nullptr) {
        return std::make_shared<VideoEncNativeMock>(videoEnc);
    }
    return nullptr;
}

std::shared_ptr<AudioDecMock> AVCodecMockFactory::CreateAudioDecMockByMime(const std::string &mime)
{
    auto audioDec = AudioDecoderFactory::CreateByMime(mime);
    if (audioDec != nullptr) {
        return std::make_shared<AudioDecNativeMock>(audioDec);
    }
    return nullptr;
}

std::shared_ptr<AudioDecMock> AVCodecMockFactory::CreateAudioDecMockByName(const std::string &name)
{
    auto audioDec = AudioDecoderFactory::CreateByName(name);
    if (audioDec != nullptr) {
        return std::make_shared<AudioDecNativeMock>(audioDec);
    }
    return nullptr;
}

std::shared_ptr<AudioEncMock> AVCodecMockFactory::CreateAudioEncMockByMime(const std::string &mime)
{
    auto audioEnc = AudioEncoderFactory::CreateByMime(mime);
    if (audioEnc != nullptr) {
        return std::make_shared<AudioEncNativeMock>(audioEnc);
    }
    return nullptr;
}

std::shared_ptr<AudioEncMock> AVCodecMockFactory::CreateAudioEncMockByName(const std::string &name)
{
    auto audioEnc = AudioEncoderFactory::CreateByName(name);
    if (audioEnc != nullptr) {
        return std::make_shared<AudioEncNativeMock>(audioEnc);
    }
    return nullptr;
}

std::shared_ptr<FormatMock> AVCodecMockFactory::CreateFormat()
{
    return std::make_shared<AVFormatNativeMock>();
}

std::shared_ptr<SurfaceMock> AVCodecMockFactory::CreateSurface()
{
    return std::make_shared<SurfaceNativeMock>();
}

std::shared_ptr<AVCodecInfoMock> AVCodecMockFactory::CreateAVCodecInfo()
{
    return std::make_shared<AVCodecInfoNativeMock>();
}

std::shared_ptr<VideoCapsMock> AVCodecMockFactory::CreateVideoCaps()
{
    return std::make_shared<VideoCapsNativeMock>();
}

std::shared_ptr<AVCodecListMock> AVCodecMockFactory::CreateAVCodecList()
{
    auto avCodecList = AVCodecListFactory::CreateAVCodecList();
    if (avCodecList != nullptr) {
        return std::make_shared<AVCodecListNativeMock>(avCodecList);
    }
    return nullptr;
}

std::shared_ptr<EnumMock> AVCodecMockFactory::CreateEnum()
{
    return std::make_shared<EnumNativeMock>();
}
}
}