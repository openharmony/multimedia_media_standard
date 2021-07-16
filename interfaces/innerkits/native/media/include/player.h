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

namespace OHOS {
namespace Media {
enum PlayerErrorType : int32_t {
    PLAYER_ERROR = 1,
    PLAYER_IO_ERROR,
    PLAYER_STREAM_ERROR,
    /* the service process is dead. */
    PLAYER_ERROR_SERVICE_DIED,
    /** Unknown error */
    PLAYER_ERROR_UNKNOWN
};

enum PlayerErrorCode : int32_t {
    PLAYER_INTERNAL_ERROR = 1,
    PLAYER_ERROR_SEEK,
    PLAYER_ERROR_STATE_CHANGE,
    PLAYER_FORMAT_NOT_SUPPORTED_ERROR
};

enum PlayerMessageType : int32_t {
    PLAYER_INFO_UNKNOWN = 1,
    PLAYER_INFO_VIDEO_RENDERING_START,
    PLAYER_INFO_BUFFERING_START,
    PLAYER_INFO_BUFFERING_END
};

enum PlayerStates : int32_t {
    PLAYER_STATE_ERROR = 0,
    PLAYER_IDLE = 1,
    PLAYER_INITIALIZED = 2,
    PLAYER_PREPARING = 3,
    PLAYER_PREPARED = 4,
    PLAYER_STARTED = 5,
    PLAYER_PAUSED = 6,
    PLAYER_STOPPED = 7,                  // equivalent to PAUSED
    PLAYER_PLAYBACK_COMPLETE = 8
};

enum PlayerSeekMode : int32_t {
    SEEK_PREVIOUS_SYNC = 0,
    SEEK_NEXT_SYNC,
    SEEK_CLOSEST_SYNC,
    SEEK_CLOSEST
};

enum PlaybackRateMode : int32_t {
    SPEED_FORWARD_0_75_X = 0,           // Video playback at 0.75x normal speed
    SPEED_FORWARD_1_00_X,               // Video playback at normal speed
    SPEED_FORWARD_1_25_X,               // Video playback at 1.25x normal speed
    SPEED_FORWARD_1_75_X,               // Video playback at 1.75x normal speed
    SPEED_FORWARD_2_00_X,               // Video playback at 2.0x normal speed
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
    virtual void OnError(int32_t errorType, int32_t errorCode) = 0;

    /**
     * Called when seek completed. accurate to millseconds.
     *
     * Seek operation is asynchronous, this virtual method will be called
     * when playback resumed at the new position.
     *
     * @param currentPositon the time when seek compelte.
     */
    virtual void OnSeekDone(uint64_t currentPositon) = 0;

    /**
     * Called when no more output buffers will be produced.
     *
     * @param isLooping A boolean to show whether the player will looping.
     */
    virtual void OnEndOfStream(bool isLooping) = 0;

    /**
     * Called when state changes.
     *
     * @param state The new State.
     */
    virtual void OnStateChanged(PlayerStates state) = 0;

    /**
     * Called when the media position changed. accurate to millseconds.
     *
     * This is called regularly and can be used to update the playback
     * progress in the UI.
     *
     * @param position The new position in nanoseconds
     */
    virtual void OnPositionUpdated(uint64_t postion) = 0;

    /**
     * Called when a player message or alarm is received.
     *
     * @param type Indicates the information type. For details, see {@link PlayerMessageType}.
     * @param extra Indicates other information, for example, the start time position of a playing file.
     */
    virtual void OnMessage(int32_t type, int32_t extra) = 0;
};

class Player {
public:
    virtual ~Player() = default;

    /**
     * @brief Sets the playback source for the player. The corresponding source can be l
     * ocal file URI
     *
     * @param uri Indicates the playback source. Currently, only local file URIs are supported.
     * @return Returns {@link SUCCESS} if the uri is set successfully; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetSource(const std::string &uri) = 0;

    /**
     * @brief Start playback.
     *
     * This function must be called after {@link Prepare}. If the player state is <b>Prepared</b>,
     * this function is called to start playback.
     *
     * @return Returns {@link SUCCESS} if the playback is started; otherwise returns an error code defined
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
     * @return Returns {@link SUCCESS} if the playback is prepared; returns an error code defined
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
     * @return Returns {@link SUCCESS} if the playback is preparing; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t PrepareAsync() = 0;

    /**
     * @brief Pauses playback.
     *
     * @return Returns {@link SUCCESS} if the playback is paused; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Pause() = 0;

    /**
     * @brief Stop playback.
     *
     * @return Returns {@link SUCCESS} if the playback is stopped; returns an error code defined
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
     * @return Returns {@link SUCCESS} if the playback is reset; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Reset() = 0;

    /**
     * @brief Releases player resources
     *
     * @return Returns {@link SUCCESS} if the playback is released; returns an error code defined
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
     * @return Returns {@link SUCCESS} if the volume is set; returns an error code defined
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
     * @return Returns {@link SUCCESS} if the seek is done; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
    */
    virtual int32_t Seek(uint64_t mSeconds, int32_t mode) = 0;

    /**
     * @brief Obtains the playback position, accurate to millisecond.
     *
     * @param currentTime Indicates the playback position.
     * @return Returns {@link SUCCESS} if the current position is get; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t GetCurrentTime(uint64_t &currentTime) = 0;

    /**
     * @brief Obtains the total duration of media files, accurate to milliseconds.
     *
     * @param duration Indicates the total duration of media files.
     * @return Returns {@link SUCCESS} if the current duration is get; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t GetDuration(uint64_t &duration) = 0;

    /**
     * @brief set the player playback rate
     *
     * @param mode the rate mode {@link PlaybackRateMode} which can set.
     * @return Returns {@link SUCCESS} if the playback rate is set successfull; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetPlaybackSpeed(int32_t mode) = 0;

    /**
     * @brief get the current player playback rate
     *
     * @param mode the rate mode {@link PlaybackRateMode} which can get.
     * @return Returns {@link SUCCESS} if the current player playback rate is get; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t GetPlaybackSpeed(int32_t &mode) = 0;

    /**
     * @brief Method to set the surface.
     *
     * @param surface pointer of the surface.
     * @return Returns {@link SUCCESS} if the surface is set; returns an error code defined
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
     * @return Returns {@link SUCCESS} if the single looping is set; returns an error code defined
     * in {@link media_errors.h} otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetLooping(bool loop) = 0;

    /**
     * @brief Method to set player callback.
     *
     * @param callback object pointer.
     * @return Returns {@link SUCCESS} if the playercallback is set; returns an error code defined
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
} // Media
} // OHOS
#endif