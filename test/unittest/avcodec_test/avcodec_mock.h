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

#ifndef AVCODEC_MOCK_H
#define AVCODEC_MOCK_H

#include <string>
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class SurfaceMock : public NoCopyable {
public:
    virtual ~SurfaceMock() = default;
};

class FormatMock : public NoCopyable {
public:
    virtual ~FormatMock() = default;
    virtual bool PutIntValue(const std::string_view &key, int32_t value) = 0;
    virtual bool GetIntValue(const std::string_view &key, int32_t &value) = 0;
};

class AVMemoryMock : public NoCopyable {
public:
    virtual ~AVMemoryMock() = default;
    virtual uint8_t *GetAddr() const = 0;
    virtual int32_t GetSize() const = 0;
    virtual uint32_t GetFlags() const = 0;
};

struct AVCodecBufferAttrMock {
    int64_t pts = 0;
    int32_t size = 0;
    int32_t offset = 0;
    uint32_t flags = 0;
};

class AVCodecCallbackMock : public NoCopyable {
public:
    virtual ~AVCodecCallbackMock() = 0;
    virtual void OnError(int32_t errorCode) = 0;
    virtual void OnStreamChanged(std::shared_ptr<FormatMock> format) = 0;
    virtual void OnNeedInputData(uint32_t index, std::shared_ptr<AVMemoryMock> data) = 0;
    virtual void OnNewOutputData(uint32_t index, std::shared_ptr<AVMemoryMock> data, AVCodecBufferAttrMock attr) = 0;
};

class VideoDecMock : public NoCopyable {
public:
    virtual ~VideoDecMock() = default;
    virtual int32_t SetCallback(std::shared_ptr<AVCodecCallbackMock> cb) = 0;
    virtual int32_t SetOutputSurface(std::shared_ptr<SurfaceMock> surface) = 0;
    virtual int32_t Configure(std::shared_ptr<FormatMock> format) = 0;
    virtual int32_t Prepare() = 0;
    virtual int32_t Start() = 0;
    virtual int32_t Stop() = 0;
    virtual int32_t Flush() = 0;
    virtual int32_t Reset() = 0;
    virtual int32_t Release() = 0;
    virtual std::shared_ptr<FormatMock> GetOutputMediaDescription() = 0;
    virtual int32_t SetParameter(std::shared_ptr<FormatMock> format) = 0;
    virtual int32_t PushInputData(uint32_t index, AVCodecBufferAttrMock &attr) = 0;
    virtual int32_t RenderOutputData(uint32_t index) = 0;
    virtual int32_t FreeOutputData(uint32_t index) = 0;
};

class VideoEncMock : public NoCopyable {
public:
    virtual ~VideoEncMock() = default;
    virtual int32_t SetCallback(std::shared_ptr<AVCodecCallbackMock> cb) = 0;
    virtual std::shared_ptr<SurfaceMock> GetInputSurface() = 0;
    virtual int32_t Configure(std::shared_ptr<FormatMock> format) = 0;
    virtual int32_t Prepare() = 0;
    virtual int32_t Start() = 0;
    virtual int32_t Stop() = 0;
    virtual int32_t Flush() = 0;
    virtual int32_t Reset() = 0;
    virtual int32_t Release() = 0;
    virtual std::shared_ptr<FormatMock> GetOutputMediaDescription() = 0;
    virtual int32_t SetParameter(std::shared_ptr<FormatMock> format) = 0;
    virtual int32_t FreeOutputData(uint32_t index) = 0;
};

class __attribute__((visibility("default"))) AVCodecMockFactory {
public:
    static std::shared_ptr<VideoDecMock> CreateVideoDecMockByMine(const std::string &mime);
    static std::shared_ptr<VideoDecMock> CreateVideoDecMockByName(const std::string &name);
    static std::shared_ptr<VideoEncMock> CreateVideoEncMockByMine(const std::string &mime);
    static std::shared_ptr<VideoEncMock> CreateVideoEncMockByName(const std::string &name);
    static std::shared_ptr<FormatMock> CreateFormat();
    static std::shared_ptr<SurfaceMock> CreateSurface();

private:
    AVCodecMockFactory() = delete;
    ~AVCodecMockFactory() = delete;
};
} // Media
} // OHOS
#endif // AVCODEC_MOCK_H