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

/**
 * @addtogroup MultiMedia_Player
 * @{
 *
 * @brief Defines the <b>Player</b> class and provides functions related to media playback.
 *
 *
 * @since 1.0
 * @version 1.0
 */

/**
 * @file player.h
 *
 * @brief Declares the <b>Player</b> class, which is used to implement player-related operations.
 *
 *
 * @since 1.0
 * @version 1.0
 */

#ifndef PLAYER_H
#define PLAYER_H

#include <surface.h>
#include <stdint.h>
#include <string>
#include <memory>

namespace OHOS {
namespace Media {
static const std::string SURFACE_WIDTH = "SURFACE_WIDTH";
static const std::string SURFACE_HEIGHT = "SURFACE_HEIGHT";
static const std::string SURFACE_STRIDE_ALIGNMENT = "SURFACE_STRIDE_ALIGNMENT";
static const std::string SURFACE_FORMAT = "SURFACE_FORMAT";

static const int MILLI = 1000;
static const int MICRO = MILLI * 1000;
static const int NANO = MICRO * 1000;
static const int KHZ = 1000;
static const int MONO = 1;

/** Player return values */
/** Player operation success */
const int32_t PLAYER_SUCCESS = 0;
/** Player operation failed */
const int32_t PLAYER_FAIL = -1;
/** Player set volume failed */
const int32_t PLAYER_SET_VOLUME_ERR = -1;
/** Player prepare failed */
const int32_t PLAYER_PREPARE_ERR = -1;
/** Player stop failed */
const int32_t PLAYER_STOP_ERR = -1;
/** Player invalid argument error */
const int32_t PLAYER_INVALID_ARGUMENT_ERR = 1;
/** Player request surface buffer failed */
const int32_t PLAYER_SAMPLE_EXTRACT_ERR = 2;
/** Player extract buffer failed */
const int32_t PLAYER_BUFFER_EXTRACT_ERR = 3;
/** Player request surface buffer failed */
const int32_t PLAYER_REQUEST_BUFFER_ERR = 4;

/**
 * @brief Provides functions for playing local stream
 *
 * @since 1.0
 * @version 1.0
 */
class Player : public std::enable_shared_from_this<Player> {
public:
    virtual ~Player();

    enum State {
        PLAYER_STATE_STOPPED,
        PLAYER_STATE_BUFFERING,
        PLAYER_STATE_PAUSED,
        PLAYER_STATE_PLAYING
    };

    enum ErrorType {
        PLAYER_ERROR = 1,
        PLAYER_IO_ERROR,
        PLAYER_STREAM_ERROR
    };

    enum PlayerError {
        PLAYER_INTERNAL_ERROR = 1,
        PLAYER_ERROR_SEEK,
        PLAYER_ERROR_STATE_CHANGE,
        PLAYER_FORMAT_NOT_SUPPORTED_ERROR
    };

    enum MessageType {
        PLAYER_INFO_UNKNOWN = 1,
        PLAYER_INFO_VIDEO_RENDERING_START,
        PLAYER_INFO_BUFFERING_START,
        PLAYER_INFO_BUFFERING_END
    };

    /**
     * Abstract class to be inherited from to receive notifications.
     *
     * Those method implementation will be called from undefined threads,
     * possibly concurrently.
     */
    struct PlayerCallback {
        virtual ~PlayerCallback();
        /**
         * Called when an error occurred.
         *
         * @param errorType Error type.
         * @param errorCode Error code.
         * @param player A shared pointer to the caller Player object.
         */
        virtual void OnError(int32_t errorType, int32_t errorCode,
                             const std::shared_ptr<Player> &player) const {}

        /**
         * Called when no more output buffers will be produced.
         *
         * @param player A shared pointer to the caller Player object.
         */
        virtual void OnEndOfStream(const std::shared_ptr<Player> &player) const {}

        /**
         * Called when state changes.
         *
         * @param state The new State.
         * @param player A shared pointer to the caller Player object.
         */
        virtual void OnStateChanged(State state, const std::shared_ptr<Player> &player) const {}

        /**
         * Called when the media position changed.
         *
         * This is called regularly and can be used to update the playback
         * progress in the UI.
         *
         * @param player A shared pointer to the caller Player object.
         * @param position The new position in nanoseconds
         */
        virtual void OnPositionUpdated(const std::shared_ptr<Player> &player,
                                       uint64_t position) const {}

        /**
         * Called when audio, video or subtitle tracks change.
         *
         * @param player A shared pointer to the caller Player object.
         */
        virtual void OnMediaTracksChanged(const std::shared_ptr<Player> &player) const {}

        /**
         * Called when Player::SeekTo() completed.
         *
         * Seek operation is asynchronous, this virtual method will be called
         * when playback resumed at the new position.
         *
         * @param player A shared pointer to the caller Player object.
         */
        virtual void OnSeekDone(const std::shared_ptr<Player> &player) const {}

        /**
         * Called when a player message or alarm is received.
         *
         * @param type Indicates the message type.
         * @param extra Indicates the extra code for the message type.
         * @param player A shared pointer to the caller Player object.
         */
        virtual void OnMessage(int32_t type, int32_t extra,
                               const std::shared_ptr<Player> &player) const {}
    };

    /**
     * Creates an instance for media playback.
     *
     * @return a new Player shared pointer.
     */
    static std::shared_ptr<Player> Create(const std::string &audioType,
                                            const std::string &videoType);

    /**
     * @brief Sets the playback source for the player. The corresponding source can be the file
     * descriptor (FD) of the local file, local file URI, network URI, or media stream.
     *
     * @param uri Indicates the playback source. Currently, only local file URIs are supported.
     * @return Returns <b>0</b> if the setting is successful; returns <b>errno</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetSource(const std::string &uri) = 0;

    /**
     * @brief Prepares the playback environment and buffers media data.
     *
     * This function must be called after {@link SetSource}.
     *
     * @return Returns <b>0</b> if the playback environment is prepared and media data is buffered;
     * returns <b>-1</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Prepare() = 0;

    /**
     * @brief Starts or resumes playback.
     *
     * This function must be called after {@link Prepare}. If the player state is <b>Prepared</b>,
     * this function is called to start playback. If the player state is <b>Playback paused</b>,
     * this function is called to resume playback. If the media is playing in an abnormal speed,
     * this function is called to restore the playback speed to normal.
     *
     * @return Returns <b>0</b> if the playback starts or resumes; returns <b>-1</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Play() const = 0;

    /**
     * @brief Checks whether the player is playing.
     *
     * @return Returns <b>true</b> if the player is playing; returns <b>false</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual bool IsPlaying() const = 0;

    /**
     * @brief Pauses/Resume playback.
     *
     * @return Returns <b>0</b> if the playback is paused; returns <b>-1</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Pause() const = 0;

    /**
     * @brief Stops playback.
     *
     * @return Returns <b>0</b> if the playback is stopped; returns <b>-1</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Stop() const = 0;

    /**
     * @brief Changes the playback position.
     *
     * This function can be used during playback or pause.
     *
     * @param mSeconds Indicates the target playback position, accurate to second.
     * @param mode Indicates the player seek mode. For details, see {@link PlayerSeekMode}.
     * @return Returns <b>0</b> if the playback position is changed; returns <b>-1</b> otherwise.
     * @since 1.0
     * @version 1.0
    */
    virtual int32_t Rewind(int64_t mSeconds, int32_t mode) const = 0;

    /**
     * @brief Sets the volume of the player.
     *
     * This function can be used during playback or pause. The value <b>0</b> indicates no sound,
     * and <b>100</b> indicates the original volume. If no audio device is started or no audio
     * stream exists, the value <b>-1</b> is returned.
     *
     * @param leftVolume Indicates the target volume of the left audio channel to set,
     *        ranging from 0 to 300.
     * @param rightVolume Indicates the target volume of the right audio channel to set,
     *        ranging from 0 to 300.
     * @return Returns <b>0</b> if the setting is successful; returns <b>-1</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetVolume(float leftVolume, float rightVolume) const = 0;

    /**
     * @brief Obtains the playback position, accurate to millisecond.
     *
     * @param time Indicates the playback position.
     * @return Returns <b>0</b> if the playback position is obtained; returns <b>-1</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t GetCurrentTime(int64_t &time) const = 0;

    /**
     * @brief Obtains the total duration of media files, in milliseconds.
     *
     * @param duration Indicates the total duration of media files.
     * @return Returns <b>0</b> if the total duration is obtained; returns <b>-1</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t GetDuration(int64_t &duration) const = 0;

    /**
     * @brief Restores the player to the initial state.
     *
     * @return Returns <b>0</b> if the player is restored; returns <b>-1</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Reset() const = 0;

    /**
     * @brief Releases player resources.
     *
     * @return Returns <b>0</b> if player resources are released; returns <b>-1</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Release() const = 0;

    /**
     * @brief Enables single looping of the media playback.
     *
     * @return Returns <b>0</b> single looping is enabled; returns <b>-1</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t EnableSingleLooping(bool loop) = 0;

    /**
     * @brief Returns the value whether single looping is enabled or not .
     *
     * @return Returns true if single looping is enabled; returns false otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual bool IsSingleLooping() const = 0;

    /**
     * @brief Method to set player callback.
     *
     * @param callback object pointer.
     * @since 1.0
     * @version 1.0
     */
    virtual void SetPlayerCallback(const PlayerCallback &cb) = 0;

    /**
     * @brief Method to set the surface.
     *
     * @param surface pointer.
     * @return Returns <b>0</b> video surface is set; returns <b>-1</b> otherwise.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetVideoSurface(sptr<Surface> surface) = 0;
};
}  // namespace Media
}  // namespace OHOS
#endif  // PLAYER_H
