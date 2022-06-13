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

#ifndef RECORDERPROFILES_H
#define RECORDERPROFILES_H

#include <cstdint>
#include <memory>
#include "avcodec_info.h"

namespace OHOS {
namespace Media {
/**
 * @brief Video recorder quality level
 *
 * @since 3.2
 * @version 3.2
 */
enum VideoRecorderQualityLevel : int32_t {
    /**
     * Quality level corresponding to the lowest available resolution.
     * @since 9
     */
    RECORDER_QUALITY_LOW = 0,

    /**
    * Quality level corresponding to the highest available resolution.
    * @since 9
    */
    RECORDER_QUALITY_HIGH = 1,

    /**
    * Quality level corresponding to the qcif (176 x 144) resolution.
    * @since 9
    */
    RECORDER_QUALITY_QCIF = 2,

    /**
    * Quality level corresponding to the cif (352 x 288) resolution.
    * @since 9
    */
    RECORDER_QUALITY_CIF = 3,

    /**
    * Quality level corresponding to the 480p (720 x 480) resolution.
    * @since 9
    */
    RECORDER_QUALITY_480P = 4,

    /**
    * Quality level corresponding to the 720P (1280 x 720) resolution.
    * @since 9
    */
    RECORDER_QUALITY_720P = 5,

    /**
    * Quality level corresponding to the 1080P (1920 x 1080) resolution.
    * @since 9
    */
    RECORDER_QUALITY_1080P = 6,

    /**
    * Quality level corresponding to the QVGA (320x240) resolution.
    * @since 9
    */
    RECORDER_QUALITY_QVGA = 7,

    /**
    * Quality level corresponding to the 2160p (3840x2160) resolution.
    * @since 9
    */
    RECORDER_QUALITY_2160P = 8,

    /**
    * Time lapse quality level corresponding to the lowest available resolution.
    * @since NA
    */
    RECORDER_QUALITY_TIME_LAPSE_LOW = 100,

    /**
    * Time lapse quality level corresponding to the highest available resolution.
    * @since NA
    */
    RECORDER_QUALITY_TIME_LAPSE_HIGH = 101,

    /**
    * Time lapse quality level corresponding to the qcif (176 x 144) resolution.
    * @since NA
    */
    RECORDER_QUALITY_TIME_LAPSE_QCIF = 102,

    /**
    * Time lapse quality level corresponding to the cif (352 x 288) resolution.
    * @since NA
    */
    RECORDER_QUALITY_TIME_LAPSE_CIF = 103,

    /**
    * Time lapse quality level corresponding to the 480p (720 x 480) resolution.
    * @since NA
    */
    RECORDER_QUALITY_TIME_LAPSE_480P = 104,

    /**
    * Time lapse quality level corresponding to the 720p (1280 x 720) resolution.
    * @since NA
    */
    RECORDER_QUALITY_TIME_LAPSE_720P = 105,

    /**
    * Time lapse quality level corresponding to the 1080p (1920 x 1088) resolution.
    * @since NA
    */
    RECORDER_QUALITY_TIME_LAPSE_1080P = 106,

    /**
    * Time lapse quality level corresponding to the QVGA (320 x 240) resolution.
    * @since NA
    */
    RECORDER_QUALITY_TIME_LAPSE_QVGA = 107,

    /**
    * Time lapse quality level corresponding to the 2160p (3840 x 2160) resolution.
    * @since NA
    */
    RECORDER_QUALITY_TIME_LAPSE_2160P = 108,

    /**
    * High speed ( >= 100fps) quality level corresponding to the lowest available resolution.
    * @since NA
    */
    RECORDER_QUALITY_HIGH_SPEED_LOW = 200,

    /**
    * High speed ( >= 100fps) quality level corresponding to the highest available resolution.
    * @since NA
    */
    RECORDER_QUALITY_HIGH_SPEED_HIGH = 201,

    /**
    * High speed ( >= 100fps) quality level corresponding to the 480p (720 x 480) resolution.
    * @since NA
    */
    RECORDER_QUALITY_HIGH_SPEED_480P = 202,

    /**
    * High speed ( >= 100fps) quality level corresponding to the 720p (1280 x 720) resolution.
    * @since NA
    */
    RECORDER_QUALITY_HIGH_SPEED_720P = 203,

    /**
    * High speed ( >= 100fps) quality level corresponding to the 1080p (1920 x 1080 or 1920x1088)
    * resolution.
    * @since NA
    */
    RECORDER_QUALITY_HIGH_SPEED_1080P = 204,
};

/**
 * @brief Capability data struct of video recorder caps
 *
 * @since 3.2
 * @version 3.2
 */
struct VideoRecorderCaps {
    std::string containerFormatType = "";
    std::string audioEncoderMime = "";
    Range audioBitrateRange;
    std::vector<int32_t> audioSampleRates;
    Range audioChannelRange;
    Range videoBitrateRange;
    Range videoFramerateRange;
    std::string videoEncoderMime = "";
    Range videoWidthRange;
    Range videoHeightRange;
};

/**
 * @brief Capability data struct of audio recorder caps
 *
 * @since 3.2
 * @version 3.2
 */
struct AudioRecorderCaps {
    std::string containerFormatType = "";
    std::string mimeType = "";
    Range bitrate;
    Range channels;
    std::vector<int32_t> sampleRate;
};

/**
 * @brief Capability data struct of video recorder profile
 *
 * @since 3.2
 * @version 3.2
 */
struct VideoRecorderProfile {
    std::string containerFormatType = "";
    std::string audioCodec = "";
    std::string videoCodec = "";
    int32_t audioBitrate = 0;
    int32_t audioChannels = 0;
    int32_t audioSampleRate = 0;
    int32_t durationTime = 0;
    int32_t qualityLevel = RECORDER_QUALITY_LOW;
    int32_t videoBitrate = 0;
    int32_t videoFrameWidth = 0;
    int32_t videoFrameHeight = 0;
    int32_t videoFrameRate = 0;
};

/**
 * @brief Capability data struct of audio recorder profile
 *
 * @since 3.2
 * @version 3.2
 */
struct  AudioRecorderProfile {
    std::string containerFormatType = "";
    std::string audioCodec = "";
    int32_t audioBitrate = 0;
    int32_t audioSampleRate = 0;
    int32_t audioChannels = 0;
};

class RecorderProfiles {
public:
    virtual ~RecorderProfiles() = default;

    /**
     * @brief check wether the audio recorder profile is supported.
     * @param profile see @AudioRecorderProfile .
     * @return  Returns the audio profile is support, if support, return true.
     * @since 3.2
     * @version 3.2
     */
    virtual bool IsAudioRecoderConfigSupported(const AudioRecorderProfile &profile) = 0;

    /**
     * @brief Checks if there is a profile of the recorder used for a specified sourceId and video record quality.
     * @param sourceId recorder source id, The lower 16 bits are valid. 8-15 bits indicates the type of source.
     * 0: camera, 1 virtual display. 0-7 bits indicates corresponding to the id under the source type.
     * @param qualityLevel record quality level, see @VideoRecorderQualityLevel .
     * @return  Returns the qualityLevel and sourceId of the video profile is exist, if exist, return true.
     * @since 3.2
     * @version 3.2
     */
    virtual bool HasVideoRecorderProfile(int32_t sourceId, int32_t qualityLevel) = 0;

    /**
     * @brief Get the supported audio recorder capabilities.
     * @return Returns an array of supported audio recorder capability.
     * @since 3.2
     * @version 3.2
     */
    virtual std::vector<std::shared_ptr<AudioRecorderCaps>> GetAudioRecorderCaps() = 0;

    /**
     * @brief Get the supported video recorder capabilities.
     * @return Returns an array of supported video recorder capability.
     * @since 3.2
     * @version 3.2
     */
    virtual std::vector<std::shared_ptr<VideoRecorderCaps>> GetVideoRecorderCaps() = 0;

    /**
     * @brief Get the  video recorder profile used for a specified sourceId and video record quality.
     * @param sourceId recorder source id, The lower 16 bits are valid. 8-15 bits indicates the type of source.
     * 0: camera, 1 virtual display. 0-7 bits indicates corresponding to the id under the source type.
     * @param qualityLevel record quality level, see @VideoRecorderQualityLevel .
     * @return Returns a supported video recorder profile.
     * @since 3.2
     * @version 3.2
     */
    virtual std::shared_ptr<VideoRecorderProfile> GetVideoRecorderProfile(int32_t sourceId, int32_t qualityLevel) = 0;
};

class __attribute__((visibility("default"))) RecorderProfilesFactory {
public:
    static RecorderProfiles& CreateRecorderProfiles();
private:
    RecorderProfilesFactory() = default;
    ~RecorderProfilesFactory() = default;
};
}  // namespace Media
}  // namespace OHOS
#endif  // RECORDERPROFILES_H
