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

#ifndef AVCODEC_INFO_H
#define AVCODEC_INFO_H

#include <cstdint>
#include <vector>
#include "av_common.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
struct LevelParams
{
    int32_t maxBlockPerFrame = 0;
    int32_t maxBlockPerSecond = 0;
    int32_t maxFrameRate = 0;
    int32_t maxWidth = 0;
    int32_t maxHeight = 0;
    LevelParams(const int32_t &blockPerFrame, const int32_t &blockPerSecond,
                const int32_t &frameRate, const int32_t &width, const int32_t height)
    {
        this->maxBlockPerFrame = blockPerFrame;
        this->maxBlockPerSecond = blockPerSecond;
        this->maxFrameRate = frameRate;
        this->maxWidth = width;
        this->maxHeight = height;
    }
    LevelParams(const int32_t &blockPerFrame, const int32_t &blockPerSecond)
    {
        this->maxBlockPerFrame = blockPerFrame;
        this->maxBlockPerSecond = blockPerSecond;
    }
};

class __attribute__((visibility("default"))) AVCodecInfo {
public:
    explicit AVCodecInfo(CapabilityData &capabilityData);
    ~AVCodecInfo();

    /**
     * @brief Get name of this codec, used to create the codec instance.
     * @return Returns codec name.
     * @since 3.1
     * @version 3.1
     */
    std::string GetName();

    /**
     * @brief Get type of this codec
     * @return Returns codec type, see {@link AVCodecType}
     * @since 3.1
     * @version 3.1
     */
    AVCodecType GetType();

    /**
     * @brief Get mime type of this codec
     * @return Returns codec mime type, see {@link CodecMimeType}
     * @since 3.1
     * @version 3.1
     */
    std::string GetMimeType();

    /**
     * @brief Check whether the codec is accelerated by hardware.
     * @return Returns true if the codec is hardware accelerated; false otherwise.
     * @since 3.1
     * @version 3.1
     */
    bool IsHardwareAccelerated();

    /**
     * @brief Check whether the codec is software implemented only.
     * @return Returns true if the codec is software implemented only; false otherwise.
     * @since 3.1
     * @version 3.1
     */
    bool IsSoftwareOnly();

    /**
     * @brief Check whether the codec is provided by vendor.
     * @return Returns true if the codec is provided by vendor; false otherwise.
     * @since 3.1
     * @version 3.1
     */
    bool IsVendor();

private:
    CapabilityData data_;
};
class __attribute__((visibility("default"))) VideoCaps {
public:
    explicit VideoCaps(CapabilityData &capabilityData);
    ~VideoCaps();

    /**
     * @brief Get codec information,  such as the codec name, codec type,
     * whether hardware acceleration is supported, whether only software is supported,
     * and whether the codec is provided by the vendor.
     * @return Returns the pointer of {@link AVCodecInfo}.
     * @since 3.1
     * @version 3.1
     */
    std::shared_ptr<AVCodecInfo> GetCodecInfo();

    /**
     * @brief Get supported bitrate range.
     * @return Returns the range of supported bitrates.
     * @since 3.1
     * @version 3.1
     */
    Range GetSupportedBitrate();

    /**
     * @brief Get supported video raw formats.
     * @return Returns an array of supported formats. For Details, see {@link VideoPixelFormat}.
     * @since 3.1
     * @version 3.1
     */
    std::vector<int32_t> GetSupportedFormats();

    /**
     * @brief Get supported alignment of video height, only used for video codecs.
     * @return Returns the supported alignment of video height (in pixels).
     * @since 3.1
     * @version 3.1
     */
    int32_t GetSupportedHeightAlignment();

    /**
     * @brief Get supported alignment of video width, only used for video codecs.
     * @return Returns the supported alignment of video width (in pixels).
     * @since 3.1
     * @version 3.1
     */
    int32_t GetSupportedWidthAlignment();

    /**
     * @brief Get supported width range of video.
     * @return Returns the supported width range of video.
     * @since 3.1
     * @version 3.1
     */
    Range GetSupportedWidth();

    /**
     * @brief Get supported height range of video.
     * @return Returns the supported height range of video.
     * @since 3.1
     * @version 3.1
     */
    Range GetSupportedHeight();

    /**
     * @brief Get supported profiles of this codec.
     * @return Returns an array of supported profiles:
     * returns {@link H263Profile} array if codec is h263,
     * returns {@link AVCProfile} array if codec is h264,
     * returns {@link HEVCProfile} array if codec is h265,
     * returns {@link MPEG2Profile} array if codec is mpeg2,
     * returns {@link MPEG4Profile} array if codec is mpeg4,
     * returns {@link VP8Profile} array if codec is vp8.
     * @since 3.1
     * @version 3.1
     */
    std::vector<int32_t> GetSupportedProfiles();

    /**
     * @brief Get supported codec level array.
     * @return Returns an array of supported codec level number.
     * @since 3.1
     * @version 3.1
     */
    std::vector<int32_t> GetSupportedLevels();

    /**
     * @brief Get supported video encode quality Range.
     * @return Returns an array of supported video encode quality Range.
     * @since 3.1
     * @version 3.1
     */
    Range GetSupportedEncodeQuality();

    /**
     * @brief Check whether the width and height is supported.
     * @param width Indicates the specified video width (in pixels).
     * @param height Indicates the specified video height (in pixels).
     * @return Returns true if the codec supports {@link width} * {@link height} size video, false otherwise.
     * @since 3.1
     * @version 3.1
     */
    bool IsSizeSupported(int32_t width, int32_t height);

    /**
     * @brief Get supported frameRate.
     * @return Returns the supported frameRate range of video.
     * @since 3.1
     * @version 3.1
     */
    Range GetSupportedFrameRate();

    /**
     * @brief Get supported frameRate range for the specified width and height.
     * @param width Indicates the specified video width (in pixels).
     * @param height Indicates the specified video height (in pixels).
     * @return Returns the supported frameRate range for the specified width and height.
     * @since 3.1
     * @version 3.1
     */
    Range GetSupportedFrameRatesFor(int32_t width, int32_t height);

    /**
     * @brief Check whether the size and frameRate is supported.
     * @param width Indicates the specified video width (in pixels).
     * @param height Indicates the specified video height (in pixels).
     * @param frameRate Indicates the specified video frameRate.
     * @return Returns true if the codec supports the specified size and frameRate; false otherwise.
     * @since 3.1
     * @version 3.1
     */
    bool IsSizeAndRateSupported(int32_t width, int32_t height, double frameRate);

    /**
     * @brief Get preferred frameRate range for the specified width and height,
     * these framerates can be reach the performance.
     * @param width Indicates the specified video width (in pixels).
     * @param height Indicates the specified video height (in pixels).
     * @return Returns preferred frameRate range for the specified width and height.
     * @since 3.1
     * @version 3.1
     */
    Range GetPreferredFrameRate(int32_t width, int32_t height);

    /**
     * @brief Get supported encode bitrate mode.
     * @return Returns an array of supported encode bitrate mode. For details, see {@link VideoEncodeBitrateMode}.
     * @since 3.1
     * @version 3.1
     */
    std::vector<int32_t> GetSupportedBitrateMode();

    /**
     * @brief Get supported encode qualit range.
     * @return Returns supported encode qualit range.
     * @since 3.1
     * @version 3.1
     */
    Range GetSupportedQuality();

    /**
     * @brief Get supported encode complexity range.
     * @return Returns supported encode complexity range.
     * @since 3.1
     * @version 3.1
     */
    Range GetSupportedComplexity();

    /**
     * @brief Check video encoder wether support request key frame dynamicly.
     * @return Returns true if support, false not support.
     * @since 3.1
     * @version 3.1
     */
    bool IsSupportDynamicIframe();

private:
    CapabilityData data_;
    int32_t blockWidth_;
    int32_t blockHeight_;
    Range horizontalBlockRange_;
    Range verticalBlockRange_;
    Range blockPerFrameRange_;
    Range blockPerSecondRange_;
    Range widthRange_;
    Range heightRange_;
    Range frameRateRange_;
    void InitParams();
    void UpdateParams();
    void LoadLevelParams();
    void LoadAVCLevelParams();
    void LoadMPEG2LevelParams();
    void LoadMPEG4LevelParams();
    ImgSize MatchClosestSize(const ImgSize &imgSize);
    int32_t DivCeil(const int32_t &dividend, const int32_t &divisor);
    Range DivRange(const Range &range, const int32_t &divisor);
    void UpdateBlockParams(const int32_t &blockWidth, const int32_t &blockHeight,
                           Range &blockPerFrameRange, Range &blockPerSecondRange);
};

class __attribute__((visibility("default"))) AudioCaps {
public:
    explicit AudioCaps(CapabilityData &capabilityData);
    ~AudioCaps();

    /**
     * @brief Get codec information,  such as the codec name, codec type,
     * whether hardware acceleration is supported, whether only software is supported,
     * and whether the codec is provided by the vendor.
     * @return Returns the pointer of {@link AVCodecInfo}
     * @since 3.1
     * @version 3.1
     */
    std::shared_ptr<AVCodecInfo> GetCodecInfo();

    /**
     * @brief Get supported bitrate range.
     * @return Returns the range of supported bitrates.
     * @since 3.1
     * @version 3.1
     */
    Range GetSupportedBitrate();

    /**
     * @brief Get supported channel range.
     * @return Returns the range of supported channel.
     * @since 3.1
     * @version 3.1
     */
    Range GetSupportedChannel();

    /**
     * @brief Get supported audio raw format range.
     * @return Returns the range of supported audio raw format. For details, see {@link AudioRawFormat}.
     * @since 3.1
     * @version 3.1
     */
    std::vector<int32_t> GetSupportedFormats();

    /**
     * @brief Get supported audio samplerates.
     * @return Returns an array of supported samplerates.
     * @since 3.1
     * @version 3.1
     */
    std::vector<int32_t> GetSupportedSampleRates();

    /**
     * @brief Get supported codec profile number.
     * @return Returns an array of supported codec profile number. For details, see {@link AACProfile}.
     * @since 3.1
     * @version 3.1
     */
    std::vector<int32_t> GetSupportedProfiles();

    /**
     * @brief Get supported codec level array.
     * @return Returns an array of supported codec level number.
     * @since 3.1
     * @version 3.1
     */
    std::vector<int32_t> GetSupportedLevels();

    /**
     * @brief Get supported encode complexity range.
     * @return Returns supported encode complexity range.
     * @since 3.1
     * @version 3.1
     */
    Range GetSupportedComplexity();

private:
    CapabilityData data_;
};
} // namespace Media
} // namespace OHOS
#endif // AVCODEC_INFO_H