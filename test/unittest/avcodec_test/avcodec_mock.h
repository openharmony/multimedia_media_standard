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

#ifndef AVCODEC_MOCK_H
#define AVCODEC_MOCK_H

#include <string>
#include "enum_mock.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
enum AVCodecTypeMock : int32_t {
    AVCODEC_TYPE_MOCK_NONE = -1,
    AVCODEC_TYPE_MOCK_VIDEO_ENCODER = 0,
    AVCODEC_TYPE_MOCK_VIDEO_DECODER,
    AVCODEC_TYPE_MOCK_AUDIO_ENCODER,
    AVCODEC_TYPE_MOCK_AUDIO_DECODER,
};

struct RangeMock {
    int32_t minVal = 0;
    int32_t maxVal = 0;
};

class SurfaceMock : public NoCopyable {
public:
    virtual ~SurfaceMock() = default;
};

class FormatMock : public NoCopyable {
public:
    virtual ~FormatMock() = default;
    virtual bool PutIntValue(const std::string_view &key, int32_t value) = 0;
    virtual bool GetIntValue(const std::string_view &key, int32_t &value) = 0;
    virtual bool PutStringValue(const std::string_view &key, const std::string_view &value) = 0;
    virtual bool GetStringValue(const std::string_view &key, std::string &value) = 0;
};

class AVMemoryMock : public NoCopyable {
public:
    virtual ~AVMemoryMock() = default;
    virtual uint8_t *GetAddr() const = 0;
    virtual int32_t GetSize() const = 0;
    virtual uint32_t GetFlags() const = 0;
};

class AVCodecInfoMock : public NoCopyable {
public:
    virtual ~AVCodecInfoMock() = default;
    virtual std::string GetName() const = 0;
    virtual int32_t GetType() const = 0;
    virtual std::string GetMimeType() const = 0;
    virtual bool IsHardwareAccelerated() const = 0;
    virtual bool IsSoftwareOnly() const = 0;
    virtual bool IsVendor() const = 0;
};

class VideoCapsMock : public NoCopyable {
public:
    virtual ~VideoCapsMock() = default;
    virtual std::shared_ptr<AVCodecInfoMock> GetCodecInfo() const = 0;
    virtual RangeMock GetSupportedBitrate() const = 0;
    virtual std::vector<int32_t> GetSupportedFormats() const = 0;
    virtual int32_t GetSupportedHeightAlignment() const = 0;
    virtual int32_t GetSupportedWidthAlignment() const = 0;
    virtual RangeMock GetSupportedWidth() const = 0;
    virtual RangeMock GetSupportedHeight() const = 0;
    virtual std::vector<int32_t> GetSupportedProfiles() const = 0;
    virtual std::vector<int32_t> GetSupportedLevels() const = 0;
    virtual RangeMock GetSupportedEncodeQuality() const = 0;
    virtual bool IsSizeSupported(int32_t width, int32_t height) const = 0;
    virtual RangeMock GetSupportedFrameRate() const = 0;
    virtual RangeMock GetSupportedFrameRatesFor(int32_t width, int32_t height) const = 0;
    virtual bool IsSizeAndRateSupported(int32_t width, int32_t height, double frameRate) const = 0;
    virtual RangeMock GetPreferredFrameRate(int32_t width, int32_t height) const = 0;
    virtual std::vector<int32_t> GetSupportedBitrateMode() const = 0;
    virtual RangeMock GetSupportedQuality() const = 0;
    virtual RangeMock GetSupportedComplexity() const = 0;
    virtual bool IsSupportDynamicIframe() const = 0;
};

class AudioCapsMock : public NoCopyable {
public:
    virtual ~AudioCapsMock() = default;
    virtual std::shared_ptr<AVCodecInfoMock> GetCodecInfo() const = 0;
    virtual RangeMock GetSupportedBitrate() const = 0;
    virtual RangeMock GetSupportedChannel() const = 0;
    virtual std::vector<int32_t> GetSupportedFormats() const = 0;
    virtual std::vector<int32_t> GetSupportedSampleRates() const = 0;
    virtual std::vector<int32_t> GetSupportedProfiles() const = 0;
    virtual std::vector<int32_t> GetSupportedLevels() const = 0;
    virtual RangeMock GetSupportedComplexity() const = 0;
};

class AVCodecListMock : public NoCopyable {
public:
    virtual ~AVCodecListMock() = default;
    virtual std::string FindVideoDecoder(std::shared_ptr<FormatMock> format) const = 0;
    virtual std::string FindVideoEncoder(std::shared_ptr<FormatMock> format) const = 0;
    virtual std::string FindAudioDecoder(std::shared_ptr<FormatMock> format) const = 0;
    virtual std::string FindAudioEncoder(std::shared_ptr<FormatMock> format) const = 0;
    virtual std::vector<std::shared_ptr<VideoCapsMock>> GetVideoDecoderCaps() const = 0;
    virtual std::vector<std::shared_ptr<VideoCapsMock>> GetVideoEncoderCaps() const = 0;
    virtual std::vector<std::shared_ptr<AudioCapsMock>> GetAudioDecoderCaps() const = 0;
    virtual std::vector<std::shared_ptr<AudioCapsMock>> GetAudioEncoderCaps() const = 0;
};

struct AVCodecBufferAttrMock {
    int64_t pts = 0;
    int32_t size = 0;
    int32_t offset = 0;
    uint32_t flags = 0;
};

class AVCodecCallbackMock : public NoCopyable {
public:
    virtual ~AVCodecCallbackMock() = default;
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
    virtual int32_t NotifyEos() = 0;
    virtual std::shared_ptr<FormatMock> GetOutputMediaDescription() = 0;
    virtual int32_t SetParameter(std::shared_ptr<FormatMock> format) = 0;
    virtual int32_t FreeOutputData(uint32_t index) = 0;
};

class AudioDecMock : public NoCopyable {
public:
    virtual ~AudioDecMock() = default;
    virtual int32_t SetCallback(std::shared_ptr<AVCodecCallbackMock> cb) = 0;
    virtual int32_t Configure(std::shared_ptr<FormatMock> format) = 0;
    virtual int32_t Prepare() = 0;
    virtual int32_t Start() = 0;
    virtual int32_t Stop() = 0;
    virtual int32_t Flush() = 0;
    virtual int32_t Reset() = 0;
    virtual int32_t Release() = 0;
    virtual int32_t PushInputData(uint32_t index, AVCodecBufferAttrMock &attr) = 0;
    virtual std::shared_ptr<FormatMock> GetOutputMediaDescription() = 0;
    virtual int32_t SetParameter(std::shared_ptr<FormatMock> format) = 0;
    virtual int32_t FreeOutputData(uint32_t index) = 0;
};

class AudioEncMock : public NoCopyable {
public:
    virtual ~AudioEncMock() = default;
    virtual int32_t SetCallback(std::shared_ptr<AVCodecCallbackMock> cb) = 0;
    virtual int32_t Configure(std::shared_ptr<FormatMock> format) = 0;
    virtual int32_t Prepare() = 0;
    virtual int32_t Start() = 0;
    virtual int32_t Stop() = 0;
    virtual int32_t Flush() = 0;
    virtual int32_t Reset() = 0;
    virtual int32_t Release() = 0;
    virtual int32_t PushInputData(uint32_t index, AVCodecBufferAttrMock &attr) = 0;
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
    static std::shared_ptr<AudioDecMock> CreateAudioDecMockByMine(const std::string &mime);
    static std::shared_ptr<AudioDecMock> CreateAudioDecMockByName(const std::string &name);
    static std::shared_ptr<AudioEncMock> CreateAudioEncMockByMine(const std::string &mime);
    static std::shared_ptr<AudioEncMock> CreateAudioEncMockByName(const std::string &name);
    static std::shared_ptr<FormatMock> CreateFormat();
    static std::shared_ptr<SurfaceMock> CreateSurface();
    static std::shared_ptr<AVCodecInfoMock> CreateAVCodecInfo();
    static std::shared_ptr<AVCodecListMock> CreateAVCodecList();
    static std::shared_ptr<VideoCapsMock> CreateVideoCaps();
    static std::shared_ptr<EnumMock> CreateEnum();
private:
    AVCodecMockFactory() = delete;
    ~AVCodecMockFactory() = delete;
};
} // Media
} // OHOS
#endif // AVCODEC_MOCK_H