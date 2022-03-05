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

#ifndef AVSPLITER_H
#define AVSPLITER_H

#include <string>
#include "avmemory.h"
#include "avcontainer_types.h"
#include "media_data_source.h"
#include "media_description.h"

namespace OHOS {
namespace Media {
/**
 * @brief Enumerates the track select mode.
 */
enum TrackSelectMode : uint8_t {
    /**
     * @brief this mode indicates that all track will be processed synchronized. When
     * selecting a track, this track's starting position to read will be forced to keep
     * sync with the other already selected tracks. If it is the first track selected,
     * this track will be read from zero timestamp.
     */
    TRACK_TIME_SYNC,
    /**
     * @brief this mode indicates that all track will be processed independent. When
     * selecting a track, this track's starting position to read will be restored to
     * last unselected position. If it it selected for the first time, this track will
     * be read from zero timestamp.
     */
    TRACK_TIME_INDEPENDENT,
};

/**
 * @brief Provides the track spliter for media files to get sample data of each track.
 */
class AVSpliter {
public:
    virtual ~AVSpliter() = default;

    /**
     * @brief Set the uri source for avspliter. Calling this method before the reset
     * of the methods in this class. This method maybe time consuming.
     *
     * @param uri the URI of input media source.
     * @param mode the mode indicates how to set the track's sample read position
     * when select a new track, see {@link TrackSelectMode}.
     * @return Returns {@link MSERR_OK} if the setting is successful; returns
     * an error code otherwise.
     */
    virtual int32_t SetSource(const std::string &uri, TrackSelectMode mode) = 0;

    /**
     * @brief Set the data source for avspliter. Calling this method before the reset
     * of the methods in this class. This method maybe time consuming.
     *
     * @param dataSource the media data source to be split
     * @param mode the mode indicates how to set the track's sample read position
     * when select a new track, see {@link TrackSelectMode}.
     * @return Returns {@link MSERR_OK} if the setting is successful; returns
     * an error code otherwise.
     */
    virtual int32_t SetSource(std::shared_ptr<IMediaDataSource> dataSource, TrackSelectMode mode) = 0;

    /**
     * @brief Get the container description.
     *
     * @param desc the output result will be filled into this parameter, and
     * all container informations will be represented by key-value pairs. For
     * keys, see {@link media_description.h}
     * @return Returns {@link MSERR_OK} if resolving the container informations
     * is successful; returns an error code otherwise.
     */
    virtual int32_t GetContainerDescription(MediaDescription &desc) = 0;

    /**
     * @brief Get the track description for specified track index.
     *
     * @param trackIdx the specified track index, for all track count in the
     * container, refer to the {@link GetContainerDescription}'s result.
     * @param desc the output result will be filled into this parameter, and
     * the specified track's all informations will be represented by key-value pairs.
     * For keys, see {@link media_description.h}
     * @return Returns {@link MSERR_OK} if resolving the track informations
     * is successful; returns an error code otherwise.
     */
    virtual int32_t GetTrackDescription(uint32_t trackIdx, MediaDescription &desc) = 0;

    /**
     * @brief Selecting a specified track to read track sample. Selecting the same track
     * multiple times has no effect. This function has different behavior when set the
     * input source with different mode, see {@link TrackSelectMode}.
     *
     * @param trackIdx the specified track index.
     * @return Returns {@link MSERR_OK} if the selecting is success, returns an error code
     * otherwise.
     */
    virtual int32_t SelectTrack(uint32_t trackIdx) = 0;

    /**
     * @brief Unselecting a specified track. After this function called, the {@link ReadTrackSample}
     * will not output sample of the specified track. Unselecting the same track multiple
     * times has no effect.
     *
     * @param trackIdx the specified track index.
     * @return Returns {@link MSERR_OK} if the unselecting is success, returns an error code
     * otherwise.
     */
    virtual int32_t UnSelectTrack(uint32_t trackIdx) = 0;

    /**
     * @brief Read a encoded sample from all selected track sample, and store it in the
     * buffer starting at the given offset. All sample will be read in sequence based on
     * timestamps. If no track selected, the default track for supported media type will
     * be read. If the Codec Specific Data exists, it will be output before any frame data.
     * Such data would be marked using the flag {@link AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_CODEC_DATA}.
     *
     * @param buffer the destination output buffer, see{@link AVMemory}.
     * @param info the sample's description information, see {@link TrackSampleInfo}.
     * @return Returns {@link MSERR_OK} if the reading is success, returns an error code
     * otherwise.
     */
    virtual int32_t ReadTrackSample(std::shared_ptr<AVMemory> buffer, TrackSampleInfo &info) = 0;

    /**
     * @brief Seek all track to specified time position according the given seek mode.
     *
     * @param timeUs the time position in microseconds where the sample will be read.
     * @param mode the hint about how to seek to the specified time position.
     * @return Returns {@link MSERR_OK} if the seek is success, returns an error code
     * otherwise.
     */
    virtual int32_t Seek(int64_t timeUs, AVSeekMode mode) = 0;

    /**
     * @brief Get the an current estimate of how much data is cached in memory, and
     * the information about whether the cached data has reached the end of stream.
     * This API is only valid for network streams.
     *
     * @param durationUs cached duration in microseconds.
     * @param endOfStream true if the cached data has reached the end of stream.
     * @return Returns {@link MSERR_OK} if the query is success, returns an error code
     * otherwise.
     */
    virtual int32_t GetCacheState(int64_t &durationUs, bool &endOfStream) = 0;

    /**
     * @brief Release the internel resource. After this method called, the avspliter
     * instance can not be used again.
     */
    virtual void Release() = 0;
};

class __attribute__((visibility("default"))) AVSpliterFactory {
public:
    static std::shared_ptr<AVSpliter> CreateAVSpliter();
private:
    AVSpliterFactory() = default;
    ~AVSpliterFactory() = default;
};
}  // namespace Media
}  // namespace OHOS

#endif
