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

#ifndef I_RECORDER_SERVICE_H
#define I_RECORDER_SERVICE_H

#include <string>
#include "recorder.h"
#include "refbase.h"
#include "surface.h"

namespace OHOS {
namespace Media {
class IRecorderService {
public:
    virtual ~IRecorderService() = default;

    /**
     * @brief Sets a video source for recording.
     *
     * If this function is not called, the output file does not contain the video track.
     *
     * @param source Indicates the video source type. For details, see {@link VideoSourceType}.
     * @param sourceId Indicates the video source ID. The value <b>-1</b> indicates an invalid ID and the setting fails.
     *
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetVideoSource(VideoSourceType source, int32_t &sourceId) = 0;

    /**
     * @brief Sets a video encoder for recording.
     *
     * If this function is not called, the output file does not contain the video track.
     * This function must be called after {@link SetVideoSource} but before {@link Prepare}.
     *
     * @param sourceId Indicates the video source ID, which can be obtained from {@link SetVideoSource}.
     * @param encoder Indicates the video encoder to set.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetVideoEncoder(int32_t sourceId, VideoCodecFormat encoder) = 0;

    /**
     * @brief Sets the width and height of the video to record.
     *
     * This function must be called after {@link SetVideoSource} but before {@link Prepare}.
     *
     * @param sourceId Indicates the video source ID, which can be obtained from {@link SetVideoSource}.
     * @param width Indicates the video width to set.
     * @param height Indicates the video height to set.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetVideoSize(int32_t sourceId, int32_t width, int32_t height) = 0;

    /**
     * @brief Sets the frame rate of the video to record.
     *
     * This function must be called after {@link SetVideoSource} but before {@link Prepare}.
     *
     * @param sourceId Indicates the video source ID, which can be obtained from {@link SetVideoSource}.
     * @param frameRate Indicates the frame rate to set.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetVideoFrameRate(int32_t sourceId, int32_t frameRate) = 0;

    /**
     * @brief Sets the encoding bit rate of the video to record.
     *
     * This function must be called after {@link SetVideoSource} but before {@link Prepare}.
     *
     * @param sourceId Indicates the video source ID, which can be obtained from {@link SetVideoSource}.
     * @param rate Indicates the encoding bit rate to set.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetVideoEncodingBitRate(int32_t sourceId, int32_t rate) = 0;

    /**
     * @brief Sets the video capture rate.
     *
     * This function must be called after {@link SetVideoSource} but before {@link Prepare}. It is valid when the
     * video source is YUV or RGB.
     *
     * @param sourceId Indicates the video source ID, which can be obtained from {@link SetVideoSource}.
     * @param fps Indicates the rate at which frames are captured per second.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetCaptureRate(int32_t sourceId, double fps) = 0;

    /**
     * @brief Obtains the surface of the video source.
     *
     * @param sourceId Indicates the video source ID, which can be obtained from {@link SetVideoSource}.
     * @return Returns the pointer to the surface.
     * @since 1.0
     * @version 1.0
     */
    virtual sptr<OHOS::Surface> GetSurface(int32_t sourceId) = 0;

    /**
     * @brief Sets the audio source for recording.
     *
     * If this function is not called, the output file does not contain the audio track.
     *
     * @param source Indicates the audio source type. For details, see {@link AudioSourceType}.
     * @param sourceId Indicates the audio source ID. The value <b>-1</b> indicates an invalid ID and the setting fails.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetAudioSource(AudioSourceType source, int32_t &sourceId) = 0;

    /**
     * @brief Sets an audio encoder for recording.
     *
     * If this function is not called, the output file does not contain the audio track.
     * This function must be called after {@link SetAudioSource} but before {@link Prepare}.
     *
     * @param sourceId Indicates the audio source ID, which can be obtained from {@link SetAudioSource}.
     * @param encoder Indicates the audio encoder to set.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetAudioEncoder(int32_t sourceId, AudioCodecFormat encoder) = 0;

    /**
     * @brief Sets the audio sampling rate for recording.
     *
     * This function must be called after {@link SetAudioSource} but before {@link Prepare}.
     *
     * @param sourceId Indicates the audio source ID, which can be obtained from {@link SetAudioSource}.
     * @param rate Indicates the sampling rate of the audio per second.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetAudioSampleRate(int32_t sourceId, int32_t rate) = 0;

    /**
     * @brief Sets the number of audio channels to record.
     *
     * This function must be called after {@link SetAudioSource} but before {@link Prepare}.
     *
     * @param sourceId Indicates the audio source ID, which can be obtained from {@link SetAudioSource}.
     * @param num Indicates the number of audio channels to set.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetAudioChannels(int32_t sourceId, int32_t num) = 0;

    /**
     * @brief Sets the encoding bit rate of the audio to record.
     *
     * This function must be called after {@link SetAudioSource} but before {@link Prepare}.
     *
     * @param sourceId Indicates the audio source ID, which can be obtained from {@link SetAudioSource}.
     * @param bitRate Indicates the audio encoding bit rate, in bit/s.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetAudioEncodingBitRate(int32_t sourceId, int32_t bitRate) = 0;

    /**
     * @brief Sets a data source for recording.
     *
     * If this function is not called, the output file does not contain the data track.
     *
     * @param sourceId Indicates the data source ID. The value <b>-1</b> indicates an invalid ID and the setting fails.
     *
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetDataSource(DataSourceType dataType, int32_t &sourceId) = 0;

    /**
     * @brief Sets the maximum duration of a recorded file, in seconds.
     *
     * This method must be called before {@link Prepare}. If the setting is valid,
     * {@link RECORDER_INFO_MAX_DURATION_APPROACHING} is reported through {@link OnInfo} in the {@link RecorderCallback}
     * class when only one second or 10% is left to reach the allowed duration.
     * If the recording output file is set by calling {@link SetOutputFile}, call {@link SetNextOutputFile} to set the
     * next output file. Otherwise, the current file will be overwritten when the allowed duration is reached.
     *
     * @param duration Indicates the maximum recording duration to set. If the value is <b>0</b> or a negative number,
     * a failure message is returned. The default duration is 60s.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetMaxDuration(int32_t duration) = 0;

    /**
     * @brief Sets the output file format.
     *
     * This function must be called before {@link Prepare}.
     *
     * @param format Indicates the output file format. For details, see {@link OutputFormatType}.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetOutputFormat(OutputFormatType format) = 0;

    /**
     * @brief Sets the output file path.
     *
     * This function must be called before {@link Prepare} and One of them {@link SetOutputFile} must be set.
     *
     * @param path Indicates the output file path.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetOutputPath(const std::string &path) = 0;

    /**
     * @brief Sets the file descriptor (FD) of the output file.
     *
     * This function must be called before {@link Prepare}.
     *
     * @param fd Indicates the FD of the file.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetOutputFile(int32_t fd) = 0;

    /**
     * @brief Sets the FD of the next output file.
     *
     * If {@link SetOutputFile} is successful, call this function to set the FD of the next output file after
     * {@link RECORDER_INFO_MAX_DURATION_APPROACHING} or {@link RECORDER_INFO_MAX_FILESIZE_APPROACHING} is received.
     *
     * @param fd Indicates the FD of the next output file.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetNextOutputFile(int32_t fd) = 0;

    /**
     * @brief Sets the maximum size of a recorded file, in bytes.
     *
     * This function must be called before {@link Prepare}. If the setting is valid,
     * {@link RECORDER_INFO_MAX_DURATION_APPROACHING} is reported through {@link OnInfo} in the {@link RecorderCallback}
     * class when only 100 KB or 10% is left to reach the allowed size.
     * If the recording output file is set by calling {@link SetOutputFile}, call {@link SetNextOutputFile} to set the
     * next output file. Otherwise, when the allowed size is reached, the current file will be overwritten. If
     * <b>MaxDuration</b> is also set by calling {@link SetMaxDuration}, <b>MaxDuration</b> or <b>MaxFileSize</b>
     * prevails depending on which of them is first satisfied.
     *
     * @param size Indicates the maximum file size to set. If the value is <b>0</b> or a negative number, a failure
     * message is returned.
     * By default, the maximum size of a single file supported by the current file system is used as the limit.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetMaxFileSize(int64_t size) = 0;

    /**
     * @brief Set and store the geodata (latitude and longitude) in the output file.
     * This method should be called before prepare(). The geodata is stored in udta box if
     * the output format is OutputFormat.THREE_GPP or OutputFormat.MPEG_4,
     * and is ignored for other output formats.
     *
     * @param latitude float: latitude in degrees. Its value must be in the range [-90, 90].
     * @param longitude float: longitude in degrees. Its value must be in the range [-180, 180].
     * @since openharmony 3.1
     * @version 1.0
     */
    virtual void SetLocation(float latitude, float longitude) = 0;

    /**
     * @brief set the orientation hint in output file, and for the file to playback. mp4 support.
     * the range of orientation should be {0, 90, 180, 270}, default is 0.
     *
     * @param rotation int32_t: should be {0, 90, 180, 270}, default is 0.
     * @since openharmony 3.1
     * @version 1.0
     */
    virtual void SetOrientationHint(int32_t rotation) = 0;

    /**
     * @brief Registers a recording listener.
     *
     * This function must be called before {@link Prepare}.
     *
     * @param callback Indicates the recording listener to register. For details, see {@link RecorderCallback}.
     * @return Returns {@link SUCCESS} if the listener is registered; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetRecorderCallback(const std::shared_ptr<RecorderCallback> &callback) = 0;

    /**
     * @brief Prepares for recording.
     *
     * This function must be called before {@link Start}.
     *
     * @return Returns {@link SUCCESS} if the preparation is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Prepare() = 0;

    /**
     * @brief Starts recording.
     *
     * This function must be called after {@link Prepare}.
     *
     * @return Returns {@link SUCCESS} if the recording is started; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Start() = 0;

    /**
     * @brief Pauses recording.
     *
     * After {@link Start} is called, you can call this function to pause recording.
     *
     * @return Returns {@link SUCCESS} if the recording is paused; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Pause() = 0;

    /**
    * @brief Resumes recording.
    *
    * You can call this function to resume recording after {@link Pause} is called.
     *
     * @return Returns {@link SUCCESS} if the recording is resumed; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Resume() = 0;

    /**
     * @brief Stops recording.
     *
     * @param block Indicates the stop mode. The value <b>true</b> indicates that the processing stops after all caches
     * are processed, and <b>false</b> indicates that the processing stops immediately and all caches are discarded.
     * @return Returns {@link SUCCESS} if the recording is stopped; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Stop(bool block) = 0;

    /**
     * @brief Resets the recording.
     *
     * After the function is called, add a recording source by calling {@link SetVideoSource} or {@link SetAudioSource},
     * set related parameters, and call {@link Start} to start recording again after {@link Prepare} is called.
     *
     * @return Returns {@link SUCCESS} if the recording is reset; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Reset() = 0;

    /**
     * @brief Releases recording resources.
     *
     * @return Returns {@link SUCCESS} if recording resources are released; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Release() = 0;

    /**
     * @brief Manually splits a video.
     *
     * This function must be called after {@link Start}. After this function is called, the file is split based on the
     * manual split type. After the manual split is complete, the initial split type is used. This function can be
     * called again only after {@link RECORDER_INFO_FILE_SPLIT_FINISHED} is reported.
     *
     * @param type Indicates the file split type. For details, see {@link FileSplitType}.
     * @param timestamp Indicates the file split timestamp. This parameter is not supported currently and can be set to
     * <b>-1</b>. The recording module splits a file based on the call time.
     * @param duration Indicates the duration for splitting the file.
     * @return Returns {@link SUCCESS} if the video is manually split; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetFileSplitDuration(FileSplitType type, int64_t timestamp, uint32_t duration) = 0;

    /**
     * @brief Sets an extended parameter for recording, for example, {@link RECORDER_PRE_CACHE_DURATION}.
     *
     * @param sourceId Indicates the data source ID. The value <b>-1</b> indicates all sources.
     * @param format Indicates the string key and value. For details, see {@link Format} and
     * {@link RECORDER_PRE_CACHE_DURATION}.
     * @return Returns {@link SUCCESS} if the setting is successful; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetParameter(int32_t sourceId, const Format &format) = 0;
};
} // namespace Media
} // namespace OHOS
#endif // I_RECORDER_SERVICE_H
