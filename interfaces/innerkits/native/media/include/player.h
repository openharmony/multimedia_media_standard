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

#ifndef PLAYER_H
#define PLAYER_H

#include <cstdint>
#include "surface.h"
#include "format.h"
#include "media_data_source.h"

namespace OHOS {
namespace Media {
enum PlayerErrorType : int32_t {
    /* Valid error, error code reference defined in media_errors.h */
    PLAYER_ERROR,
    /* Unknown error */
    PLAYER_ERROR_UNKNOWN,
     /** extend error type start,The extension error type agreed upon by the plug-in and
         the application will be transparently transmitted by the service. */
    PLAYER_ERROR_EXTEND_START = 0X10000,
};

enum PlayerMessageType : int32_t {
    /* unknown info */
    PLAYER_INFO_UNKNOWN = 0,
    /* first video frame start to render. */
    PLAYER_INFO_VIDEO_RENDERING_START,
    /* start buffering. */
    PLAYER_INFO_BUFFERING_START,
    /* end buffering. */
    PLAYER_INFO_BUFFERING_END,
    /* network bandwidth, uint is KB and passed by "extra"(arg 2). */
    PLAYER_INFO_NETWORK_BANDWIDTH,
    /* video size changed after prepared.format key:play-vid-width;play-vid-height */
    PLAYER_INFO_VIDEO_SIZE_CHANGED,
    /* buffering percent update. passed by "extra"(arg 2), [0~100]. */
    PLAYER_INFO_BUFFER_PERCENT,
    /* not fatal errors accured, errorcode see "media_errors.h" and passed by "extra"(arg 2). */
    PLAYER_INFO_WARNING,
    /* system new info type should be added here.
       extend start. App and plugins or PlayerEngine extended info type start. */
    PLAYER_INFO_EXTEND_START = 0X1000,
};

enum PlayerOnInfoType : int32_t {
    /* return the message when seeking done. */
    INFO_TYPE_SEEKDONE = 1,
    /* return the message when playback is end of steam.  */
    INFO_TYPE_EOS,
    /* return the message when PlayerStates changed.  */
    INFO_TYPE_STATE_CHANGE,
    /* return the current posion of playback automaticly.  */
    INFO_TYPE_POSITION_UPDATE,
    /* return the playback message.  */
    INFO_TYPE_MESSAGE,
    /* return the message when volume changed.  */
    INFO_TYPE_VOLUME_CHANGE,
    /* return the message with extra infomation in format. */
    INFO_TYPE_EXTRA_FORMAT
};

enum PlayerStates : int32_t {
    /* error states */
    PLAYER_STATE_ERROR = 0,
    /* idle states */
    PLAYER_IDLE = 1,
    /* initialized states(Internal states) */
    PLAYER_INITIALIZED = 2,
    /* preparing states(Internal states) */
    PLAYER_PREPARING = 3,
    /* prepared states */
    PLAYER_PREPARED = 4,
    /* started states */
    PLAYER_STARTED = 5,
    /* paused states */
    PLAYER_PAUSED = 6,
    /* stopped states */
    PLAYER_STOPPED = 7,
    /* Play to the end states */
    PLAYER_PLAYBACK_COMPLETE = 8,
};

enum PlayerSeekMode : int32_t {
    /* sync to keyframes before the time point. */
    SEEK_PREVIOUS_SYNC = 0,
    /* sync to keyframes after the time point. */
    SEEK_NEXT_SYNC,
    /* sync to closest keyframes. */
    SEEK_CLOSEST_SYNC,
    /* seek to frames closest the time point. */
    SEEK_CLOSEST,
};

enum PlaybackRateMode : int32_t {
    /* Video playback at 0.75x normal speed */
    SPEED_FORWARD_0_75_X,
    /* Video playback at normal speed */
    SPEED_FORWARD_1_00_X,
    /* Video playback at 1.25x normal speed */
    SPEED_FORWARD_1_25_X,
    /* Video playback at 1.75x normal speed */
    SPEED_FORWARD_1_75_X,
    /* Video playback at 2.0x normal speed */
    SPEED_FORWARD_2_00_X,
};

class PlayerCallback {
public:
    virtual ~PlayerCallback() = default;
    /**
     * Called when an error occurred.
     *
     * @param errorType Error type. For details, see {@link PlayerErrorType}.
     * @param errorCode Error code.
     */
    virtual void OnError(PlayerErrorType errorType, int32_t errorCode) = 0;

    /**
     * Called when a player message or alarm is received.
     *
     * @param type Indicates the information type. For details, see {@link PlayerOnInfoType}.
     * @param extra Indicates other information, for example, the start time position of a playing file.
     * @param infoBody According to the info type, the information carrier passed.Is an optional parameter.
     */
    virtual void OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody) = 0;
};

class Player {
public:
    virtual ~Player() = default;

    /**
     * @brief Sets the playback source for the player. The corresponding source can be l
     * ocal file URI
     *
     * @param uri Indicates the playback source. Currently, only local file URIs are supported.
     * @return Returns {@link MSERR_OK} if the uri is set successfully; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetSource(const std::string &uri) = 0;

    /**
     * @brief Sets the playback media data source for the player.
     *
     * @param dataSrc Indicates the media data source. in {@link media_data_source.h}
     * @return Returns {@link MSERR_OK} if the mediadatasource is set successfully; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc) = 0;

    /**
     * @brief Start playback.
     *
     * This function must be called after {@link Prepare}. If the player state is <b>Prepared</b>,
     * this function is called to start playback.
     *
     * @return Returns {@link MSERR_OK} if the playback is started; otherwise returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Play() = 0;

    /**
     * @brief Prepares the playback environment and buffers media data.
     *
     * This function must be called after {@link SetSource}.
     *
     * @return Returns {@link MSERR_OK} if the playback is prepared; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Prepare() = 0;

    /**
     * @brief Prepare the playback environment and buffers media data asynchronous.
     *
     * This function must be called after {@link SetSource}.
     *
     * @return Returns {@link MSERR_OK} if the playback is preparing; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t PrepareAsync() = 0;

    /**
     * @brief Pauses playback.
     *
     * @return Returns {@link MSERR_OK} if the playback is paused; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Pause() = 0;

    /**
     * @brief Stop playback.
     *
     * @return Returns {@link MSERR_OK} if the playback is stopped; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Stop() = 0;

    /**
     * @brief Restores the player to the initial state.
     *
     * After the function is called, add a playback source by calling {@link SetSource},
     * call {@link Play} to start playback again after {@link Prepare} is called.
     *
     * @return Returns {@link MSERR_OK} if the playback is reset; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Reset() = 0;

    /**
     * @brief Releases player resources
     *
     * @return Returns {@link MSERR_OK} if the playback is released; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Release() = 0;

    /**
     * @brief Sets the volume of the player.
     *
     * This function can be used during playback or pause. The value <b>0</b> indicates no sound,
     * and <b>1</b> indicates the original volume. If no audio device is started or no audio
     * stream exists, the value <b>-1</b> is returned.
     *
     * @param leftVolume Indicates the target volume of the left audio channel to set,
     *        ranging from 0 to 1. each step is 0.01.
     * @param rightVolume Indicates the target volume of the right audio channel to set,
     *        ranging from 0 to 1. each step is 0.01.
     * @return Returns {@link MSERR_OK} if the volume is set; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetVolume(float leftVolume, float rightVolume) = 0;

    /**
     * @brief Changes the playback position.
     *
     * This function can be used during play or pause.
     *
     * @param mSeconds Indicates the target playback position, accurate to milliseconds.
     * @param mode Indicates the player seek mode. For details, see {@link PlayerSeekMode}.
     * @return Returns {@link MSERR_OK} if the seek is done; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
    */
    virtual int32_t Seek(int32_t mSeconds, PlayerSeekMode mode) = 0;

    /**
     * @brief Obtains the playback position, accurate to millisecond.
     *
     * @param currentTime Indicates the playback position.
     * @return Returns {@link MSERR_OK} if the current position is get; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t GetCurrentTime(int32_t &currentTime) = 0;

    /**
     * @brief Obtains the total duration of media files, accurate to milliseconds.
     *
     * @param duration Indicates the total duration of media files.
     * @return Returns {@link MSERR_OK} if the current duration is get; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t GetDuration(int32_t &duration) = 0;

    /**
     * @brief set the player playback rate
     *
     * @param mode the rate mode {@link PlaybackRateMode} which can set.
     * @return Returns {@link MSERR_OK} if the playback rate is set successfull; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetPlaybackSpeed(PlaybackRateMode mode) = 0;

    /**
     * @brief get the current player playback rate
     *
     * @param mode the rate mode {@link PlaybackRateMode} which can get.
     * @return Returns {@link MSERR_OK} if the current player playback rate is get; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t GetPlaybackSpeed(PlaybackRateMode &mode) = 0;

    /**
     * @brief Method to set the surface.
     *
     * @param surface pointer of the surface.
     * @return Returns {@link MSERR_OK} if the surface is set; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetVideoSurface(sptr<Surface> surface) = 0;

    /**
     * @brief Checks whether the player is playing.
     *
     * @return Returns true if the playback is playing; false otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual bool IsPlaying() = 0;

    /**
     * @brief Returns the value whether single looping is enabled or not .
     *
     * @return Returns true if the playback is single looping; false otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual bool IsLooping() = 0;

    /**
     * @brief Enables single looping of the media playback.
     *
     * @return Returns {@link MSERR_OK} if the single looping is set; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetLooping(bool loop) = 0;

    /**
     * @brief Method to set player callback.
     *
     * @param callback object pointer.
     * @return Returns {@link MSERR_OK} if the playercallback is set; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetPlayerCallback(const std::shared_ptr<PlayerCallback> &callback) = 0;
};

class __attribute__((visibility("default"))) PlayerFactory {
public:
    static std::shared_ptr<Player> CreatePlayer();

private:
    PlayerFactory() = default;
    ~PlayerFactory() = default;
};
__attribute__((visibility("default"))) std::string PlayerErrorTypeToString(PlayerErrorType type);
} // Media
} // OHOS
#endif
