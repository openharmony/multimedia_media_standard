/*
* Copyright (C) 2021 Huawei Device Co., Ltd.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

import { ErrorCallback, AsyncCallback, Callback } from './basic';

/**
 * @name media
 * @since 6
 * @SysCap SystemCapability.Multimedia.Media
 * @import import media from '@ohos.multimedia.media'
 * @devices phone, tablet, tv, wearable
 */
declare namespace media {
  /**
   * Creates an AudioPlayer instance.
   * @since 6
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @return Returns an AudioPlayer instance if the operation is successful; returns null otherwise.
   */
  function createAudioPlayer(): AudioPlayer;

  /**
   * Creates an AudioRecorder instance.
   * @since 6
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @return Returns an AudioRecorder instance if the operation is successful; returns null otherwise.
   */
  function createAudioRecorder(): AudioRecorder;

  /**
   * Creates an VideoPlayer instance.
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @param callback Callback used to return AudioPlayer instance if the operation is successful; returns null otherwise.
   */
  function createVideoPlayer(callback: AsyncCallback<VideoPlayer>): void;
  /**
   * Creates an VideoPlayer instance.
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @return A Promise instance used to return VideoPlayer instance if the operation is successful; returns null otherwise.
   */
  function createVideoPlayer() : Promise<VideoPlayer>;

  /**
   * Creates an VideoRecorder instance.
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @param callback Callback used to return AudioPlayer instance if the operation is successful; returns null otherwise.
   */
  function createVideoRecorder(callback: AsyncCallback<VideoRecorder>): void;
  /**
   * Creates an VideoRecorder instance.
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @return A Promise instance used to return VideoRecorder instance if the operation is successful; returns null otherwise.
   */
  function createVideoRecorder(): Promise<VideoRecorder>;

  /**
   * Enumerates ErrorCode types, return in BusinessError::code
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @devices phone, tablet, tv, wearable, car
   */
  enum MediaErrorCode {
    /**
     * operation success.
     */
    MSERR_OK = 0,

    /**
     * malloc or new memory failed. maybe system have no memory.
     */
    MSERR_NO_MEMORY = 1,

    /**
     * no permission for the operation.
     */
    MSERR_OPERATION_NOT_PERMIT = 2,

    /**
     * invalid argument.
     */
    MSERR_INVALID_VAL = 3,

    /**
     * an IO error occurred.
     */
    MSERR_IO = 4,

    /**
     * operation time out.
     */
    MSERR_TIMEOUT = 5,

    /**
     * unknown error.
     */
    MSERR_UNKNOWN = 6,

    /**
     * media service died.
     */
    MSERR_SERVICE_DIED = 7,

    /**
     * operation is not permit in current state.
     */
    MSERR_INVALID_STATE = 8,

    /**
     * operation is not supported in current version.
     */
    MSERR_UNSUPPORTED = 9,
  }

  /**
   * Enumerates buffering info type, for network playback.
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @devices phone, tablet, tv, wearable, car
   */
  enum BufferingInfoType {
    /* begin to buffering*/
    BUFFERING_START = 1,

    /* end to buffering*/
    BUFFERING_END = 2,

    /* buffering percent*/
    BUFFERING_PERCENT = 3,

    /* cached duration in milliseconds*/
    CACHED_DURATION = 4,
  }

  /**
   * Describes audio playback states.
   */
  type AudioState = 'idle' | 'playing' | 'paused' | 'stopped' | 'error';

  /**
   * Manages and plays audio. Before calling an AudioPlayer method, you must use createAudioPlayer()
   * or createAudioPlayerAsync() to create an AudioPlayer instance.
   */
  interface AudioPlayer {
    /**
     * Starts audio playback.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     */
    play(): void;

    /**
     * Pauses audio playback.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     */
    pause(): void;

    /**
     * Stops audio playback.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     */
    stop(): void;

    /**
     * Resets audio playback.
     * @devices phone, tablet, tv, wearable
     * @since 7
     * @SysCap SystemCapability.Multimedia.Media
     */
     reset(): void;

    /**
     * Jumps to the specified playback position.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     * @param timeMs Playback position to jump
     */
    seek(timeMs: number): void;

    /**
     * Sets the volume.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     * @param vol Relative volume. The value ranges from 0.00 to 1.00. The value 1 indicates the maximum volume (100%).
     */
    setVolume(vol: number): void;

    /**
     * Releases resources used for audio playback.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     */
    release(): void;
    /**
    * get all track infos in MediaDescription, should be called after data loaded callback.
    * @devices phone, tablet, tv, wearable, car
    * @since 8
    * @SysCap SystemCapability.Multimedia.Media
    * @param callback async callback return track info in MediaDescription.
    */
    getTrackDescription(callback: AsyncCallback<Array<MediaDescription>>): void;

    /**
    * get all track infos in MediaDescription, should be called after data loaded callback..
    * @devices phone, tablet, tv, wearable, car
    * @since 8
    * @SysCap SystemCapability.Multimedia.Media
    * @param index  track index.
    * @return A Promise instance used to return the track info in MediaDescription.
    */
    getTrackDescription() : Promise<Array<MediaDescription>>;

    /**
     * Listens for audio playback buffering events.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param type Type of the playback buffering update event to listen for.
     * @param callback Callback used to listen for the buffering update event, return BufferingInfoType and the value.
     */
    on(type: 'bufferingUpdate', callback: (infoType: BufferingInfoType, value: number) => void): void;
    /**
     * Audio media URI. Mainstream audio formats are supported.
     * local:fd://XXX, file://XXX. network:http://xxx
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     */
    src: string;

    /**
     * Whether to loop audio playback. The value true means to loop playback.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     */
    loop: boolean;

    /**
     * Current playback position.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     */
    readonly currentTime: number;

    /**
     * Playback duration, When the data source does not support seek, it returns - 1, such as a live broadcast scenario.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     */
    readonly duration: number;

    /**
     * Playback state.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     */
    readonly state: AudioState;

    /**
     * Listens for audio playback events.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event.
     */
    on(type: 'play' | 'pause' | 'stop' | 'reset' | 'dataLoad' | 'finish' | 'volumeChange', callback: () => void): void;

    /**
     * Listens for audio playback events.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event.
     */
    on(type: 'timeUpdate', callback: Callback<number>): void;

    /**
     * Listens for playback error events.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     * @param type Type of the playback error event to listen for.
     * @param callback Callback used to listen for the playback error event.
     */
    on(type: 'error', callback: ErrorCallback): void;
  }

  /**
   * Enumerates audio encoding formats, it will be deprecated after API8, use @CodecMimeType to instead of.
   * @since 6
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @devices phone, tablet, tv, wearable
   */
  enum AudioEncoder {
    /**
     * Advanced Audio Coding Low Complexity (AAC-LC).
     */
    AAC_LC = 3,
  }

  /**
   * Enumerates audio output formats, it will be deprecated after API8, use @ContainerFormatType to instead of.
   * @since 6
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @devices phone, tablet, tv, wearable
   */
  enum AudioOutputFormat {
    /**
     * Indicates the Moving Picture Experts Group-4 (MPEG4) media format.
     */
    MPEG_4 = 2,

    /**
     * Audio Data Transport Stream (ADTS), a transmission stream format of Advanced Audio Coding (AAC) audio.
     */
    AAC_ADTS = 6
  }

  interface Location {
    /**
     * Latitude.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     */
    latitude: number;

    /**
     * Longitude.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     */
    longitude: number;
  }

  interface AudioRecorderConfig {
    /**
     * Audio encoding format. The default value is DEFAULT, it will be deprecated after API8.
     * use "audioEncoderMime" instead.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     */
    audioEncoder?: AudioEncoder;

    /**
     * Audio encoding bit rate.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     */
    audioEncodeBitrate?: number;

    /**
     * Audio sampling rate.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     */
    audioSampleRate?: number;

    /**
     * Number of audio channels.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     */
    numberOfChannels?: number;

    /**
     * Audio output format. The default value is DEFAULT, it will be deprecated after API8.
     * it will be use "fileFormat" to instead of.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     */
    format?: AudioOutputFormat;

    /**
     * Audio output uri.support two kind of uri now.
     * format like: scheme + "://" + "context".
     * file:  file://path
     * fd:    fd://fd
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     */
    uri: string;

    /**
     * Geographical location information.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     */
    location?: Location;

    /**
     * audio encoding format MIME. it used to instead of audioEncoder.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    audioEncoderMime?: CodecMimeType;
    /**
     * output file format. see @ContainerFormatType , it used to instead of "format".
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    fileFormat?: ContainerFormatType;
  }

  interface AudioRecorder {
    /**
     * Prepares for recording.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     * @param config Recording parameters.
     */
    prepare(config: AudioRecorderConfig): void;

    /**
     * Starts audio recording.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     */
    start(): void;

    /**
     * Pauses audio recording.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     */
    pause(): void;

    /**
     * Resumes audio recording.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     */
    resume(): void;

    /**
     * Stops audio recording.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     */
    stop(): void;

    /**
     * Releases resources used for audio recording.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     */
    release(): void;

    /**
     * Resets audio recording.
     * Before resetting audio recording, you must call stop() to stop recording. After audio recording is reset,
     * you must call prepare() to set the recording configurations for another recording.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     */
    reset(): void;

    /**
     * Listens for audio recording events.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     * @param type Type of the audio recording event to listen for.
     * @param callback Callback used to listen for the audio recording event.
     */
    on(type: 'prepare' | 'start' | 'pause' | 'resume' | 'stop' | 'release' | 'reset', callback: () => void): void;

    /**
     * Listens for audio recording error events.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     * @param type Type of the audio recording error event to listen for.
     * @param callback Callback used to listen for the audio recording error event.
     */
    on(type: 'error', callback: ErrorCallback): void;
  }

  /**
  * Describes video recorder states.
  */
  type VideoRecordState = 'idle' | 'prepared' | 'playing' | 'paused' | 'stopped' | 'error';

  interface VideoRecorder {
    /**
     * Prepares for recording.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param config Recording parameters.
     * @param callback A callback instance used to return when prepare completed.
     */
    prepare(config: VideoRecorderConfig, callback: AsyncCallback<void>): void;
    /**
     * Prepares for recording.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param config Recording parameters.
     * @return A Promise instance used to return when prepare completed.
     */
    prepare(config: VideoRecorderConfig): Promise<void>;
    /**
     * get input surface.it must be called between prepare completed and start.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param callback Callback used to return the input surface id in string.
     */
    getInputSurface(callback: AsyncCallback<string>): void;
    /**
     * get input surface. it must be called between prepare completed and start.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @return A Promise instance used to return the input surface id in string.
     */
    getInputSurface(): Promise<string>;
    /**
     * Starts video recording.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param callback A callback instance used to return when start completed.
     */
    start(callback: AsyncCallback<void>): void;
    /**
     * Starts video recording.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @return A Promise instance used to return when start completed.
     */
    start(): Promise<void>;
    /**
     * Pauses video recording.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param callback A callback instance used to return when pause completed.
     */
    pause(callback: AsyncCallback<void>): void;
    /**
     * Pauses video recording.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @return A Promise instance used to return when pause completed.
     */
    pause(): Promise<void>;
    /**
     * Resumes video recording.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param callback A callback instance used to return when resume completed.
     */
    resume(callback: AsyncCallback<void>): void;
    /**
     * Resumes video recording.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @return A Promise instance used to return when resume completed.
     */
    resume(): Promise<void>;
    /**
     * Stops video recording.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param callback A callback instance used to return when stop completed.
     */
    stop(callback: AsyncCallback<void>): void;
    /**
     * Stops video recording.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @return A Promise instance used to return when stop completed.
     */
    stop(): Promise<void>;
    /**
     * Releases resources used for video recording.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param callback A callback instance used to return when release completed.
     */
    release(callback: AsyncCallback<void>): void;
    /**
      * Releases resources used for video recording.
      * @devices phone, tablet, tv, wearable
      * @since 8
      * @SysCap SystemCapability.Multimedia.Media
      * @return A Promise instance used to return when release completed.
      */
    release(): Promise<void>;
    /**
     * Resets video recording.
     * Before resetting video recording, you must call stop() to stop recording. After video recording is reset,
     * you must call prepare() to set the recording configurations for another recording.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param callback A callback instance used to return when reset completed.
     */
    reset(callback: AsyncCallback<void>): void;
     /**
      * Resets video recording.
      * Before resetting video recording, you must call stop() to stop recording. After video recording is reset,
      * you must call prepare() to set the recording configurations for another recording.
      * @devices phone, tablet, tv, wearable
      * @since 8
      * @SysCap SystemCapability.Multimedia.Media
      * @return A Promise instance used to return when reset completed.
      */
    reset(): Promise<void>;
    /**
     * Listens for video recording error events.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param type Type of the video recording error event to listen for.
     * @param callback Callback used to listen for the video recording error event.
     */
    on(type: 'error', callback: ErrorCallback): void;

    /**
     * video recorder state.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
     readonly state: VideoRecordState;
  }

  /**
   * Describes video playback states.
   */
  type VideoPlayState = 'idle' | 'prepared' | 'playing' | 'paused' | 'stopped' | 'error';

  /**
   * Enumerates playback speed.
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @devices phone, tablet, tv, wearable, car
   */
  enum PlaybackSpeed {
    /* playback at 0.75x normal speed */
    SPEED_FORWARD_0_75_X = 0,
    /* playback at normal speed */
    SPEED_FORWARD_1_00_X = 1,
    /* playback at 1.25x normal speed */
    SPEED_FORWARD_1_25_X = 2,
    /* playback at 1.75x normal speed */
    SPEED_FORWARD_1_75_X = 3,
    /* playback at 2.0x normal speed */
    SPEED_FORWARD_2_00_X = 4,
  }

  /**
   * Manages and plays video. Before calling an video method, you must use createVideoPlayer() to create an VideoPlayer
   * instance.
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @devices phone, tablet, tv, wearable, car
   */
 interface VideoPlayer {
    /**
     * set display surface.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param surfaceId surface id, video player will use this id get a surface instance.
     * @return A Promise instance used to return when release output buffer completed.
     */
    setDisplaySurface(surfaceId: string, callback: AsyncCallback<void>): void;
    /**
    * set display surface.
    * @devices phone, tablet, tv, wearable, car
    * @since 8
    * @SysCap SystemCapability.Multimedia.Media
    * @param surfaceId surface id, video player will use this id get a surface instance.
    * @return A Promise instance used to return when release output buffer completed.
    */
    setDisplaySurface(surfaceId: string): Promise<void>;
    /**
     * prepare video playback, it will request resource for playing.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param callback A callback instance used to return when prepare completed.
     */
    prepare(callback: AsyncCallback<void>): void;
     /**
      * prepare video playback, it will request resource for playing.
      * @devices phone, tablet, tv, wearable
      * @since 8
      * @SysCap SystemCapability.Multimedia.Media
      * @return A Promise instance used to return when prepare completed.
      */
    prepare(): Promise<void>;
    /**
     * Starts video playback.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param callback A callback instance used to return when start completed.
     */
    play(callback: AsyncCallback<void>): void;
     /**
      * Starts video playback.
      * @devices phone, tablet, tv, wearable
      * @since 8
      * @SysCap SystemCapability.Multimedia.Media
      * @return A Promise instance used to return when start completed.
      */
    play(): Promise<void>;
    /**
     * Pauses video playback.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param callback A callback instance used to return when pause completed.
     */
    pause(callback: AsyncCallback<void>): void;
     /**
      * Pauses video playback.
      * @devices phone, tablet, tv, wearable
      * @since 8
      * @SysCap SystemCapability.Multimedia.Media
      * @return A Promise instance used to return when pause completed.
      */
    pause(): Promise<void>;
    /**
     * Stops video playback.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param callback A callback instance used to return when stop completed.
     */
    stop(callback: AsyncCallback<void>): void;
     /**
      * Stops video playback.
      * @devices phone, tablet, tv, wearable
      * @since 8
      * @SysCap SystemCapability.Multimedia.Media
      * @return A Promise instance used to return when stop completed.
      */
    stop(): Promise<void>;
    /**
     * Resets video playback, it will release the resource.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param callback A callback instance used to return when reset completed.
     */
    reset(callback: AsyncCallback<void>): void;
     /**
      * Resets video playback, it will release the resource.
      * @devices phone, tablet, tv, wearable
      * @since 8
      * @SysCap SystemCapability.Multimedia.Media
      * @return A Promise instance used to return when reset completed.
      */
    reset(): Promise<void>;
    /**
     * Jumps to the specified playback position by default SeekMode(SEEK_CLOSEST),
     * the performance may be not the best.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param timeMs Playback position to jump
     * @param callback A callback instance used to return when seek completed
     * and return the seeking position result.
     */
    seek(timeMs: number, callback: AsyncCallback<number>): void;
    /**
     * Jumps to the specified playback position.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param timeMs Playback position to jump
     * @param mode seek mode, see @SeekMode .
     * @param callback A callback instance used to return when seek completed
     * and return the seeking position result.
     */
     seek(timeMs: number, mode:SeekMode, callback: AsyncCallback<number>): void;
     /**
      * Jumps to the specified playback position.
      * @devices phone, tablet, tv, wearable
      * @since 8
      * @SysCap SystemCapability.Multimedia.Media
      * @param timeMs Playback position to jump
      * @param mode seek mode, see @SeekMode .
      * @return A Promise instance used to return when seek completed
      * and return the seeking position result.
      */
    seek(timeMs: number, mode?:SeekMode): Promise<number>;
    /**
     * Sets the volume.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param vol Relative volume. The value ranges from 0.00 to 1.00. The value 1 indicates the maximum volume (100%).
     * @param callback A callback instance used to return when set volume completed.
     */
    setVolume(vol: number, callback: AsyncCallback<void>): void;
     /**
      * Sets the volume.
      * @devices phone, tablet, tv, wearable
      * @since 8
      * @SysCap SystemCapability.Multimedia.Media
      * @param vol Relative volume. The value ranges from 0.00 to 1.00. The value 1 indicates the maximum volume (100%).
      * @return A Promise instance used to return when set volume completed.
      */
    setVolume(vol: number): Promise<void>;
    /**
     * Releases resources used for video playback.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param callback A callback instance used to return when release completed.
     */
    release(callback: AsyncCallback<void>): void;
     /**
      * Releases resources used for video playback.
      * @devices phone, tablet, tv, wearable
      * @since 8
      * @SysCap SystemCapability.Multimedia.Media
      * @return A Promise instance used to return when release completed.
      */
    release(): Promise<void>;
    /**
    * get all track infos in MediaDescription, should be called after data loaded callback.
    * @devices phone, tablet, tv, wearable, car
    * @since 8
    * @SysCap SystemCapability.Multimedia.Media
    * @param callback async callback return track info in MediaDescription.
    */
    getTrackDescription(callback: AsyncCallback<Array<MediaDescription>>): void;

    /**
    * get all track infos in MediaDescription, should be called after data loaded callback..
    * @devices phone, tablet, tv, wearable, car
    * @since 8
    * @SysCap SystemCapability.Multimedia.Media
    * @param index  track index.
    * @return A Promise instance used to return the track info in MediaDescription.
    */
    getTrackDescription() : Promise<Array<MediaDescription>>;

    /**
     * media url. Mainstream video formats are supported.
     * local:fd://XXX, file://XXX. network:http://xxx
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    url: string;

    /**
     * Whether to loop video playback. The value true means to loop playback.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    loop: boolean;

    /**
     * Current playback position.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    readonly currentTime: number;

    /**
     * Playback duration, if -1 means cannot seek.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    readonly duration: number;

    /**
     * Playback state.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    readonly state: VideoPlayState;

    /**
     * video width, valid after prepared.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    readonly width: number;

    /**
     * video height, valid after prepared.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    readonly height: number;

    /**
     * set payback speed.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param speed playback speed, see @PlaybackSpeed .
     * @param callback Callback used to return actually speed.
     */
    setSpeed(speed:number, callback: AsyncCallback<number>): void;
    /**
     * set output surface.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param speed playback speed, see @PlaybackSpeed .
     * @return A Promise instance used to return actually speed.
     */
    setSpeed(speed:number): Promise<number>;

    /**
     * Listens for video playback completed events.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event return .
     */
    on(type: 'playbackCompleted', callback: Callback<void>): void;

    /**
     * Listens for video playback buffering events.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param type Type of the playback buffering update event to listen for.
     * @param callback Callback used to listen for the buffering update event, return BufferingInfoType and the value.
     */
    on(type: 'bufferingUpdate', callback: (infoType: BufferingInfoType, value: number) => void): void;

    /**
     * Listens for start render video frame events.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event return .
     */
    on(type: 'startRenderFrame', callback: Callback<void>): void;

    /**
     * Listens for video size changed event.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param type Type of the playback event to listen for.
     * @param callback Callback used to listen for the playback event return video size.
     */
    on(type: 'videoSizeChanged', callback: (width: number, height: number) => void): void;

    /**
     * Listens for playback error events.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param type Type of the playback error event to listen for.
     * @param callback Callback used to listen for the playback error event.
     */
    on(type: 'error', callback: ErrorCallback): void;
  }

  /**
   * Enumerates container format type(The abbreviation for 'container format type' is CFT).
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @devices phone, tablet, tv, wearable, car
   */
  enum ContainerFormatType {
    /**
     * A video container format type mp4.
     */
    CFT_MPEG_4 = "mp4",

    /**
     * A audio container format type m4a.
     */
    CFT_MPEG_4A = "m4a",
  }

  /**
   * Enumerates media data type.
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @devices phone, tablet, tv, wearable, car
   */
  enum MediaType {
    /**
     * track is audio.
     */
    MEDIA_TYPE_AUD = 0,
    /**
     * track is video.
     */
    MEDIA_TYPE_VID = 1,
  }

  /**
   * Enumerates media description key.
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @devices phone, tablet, tv, wearable, car
   */
  enum MediaDescriptionKey {
    /**
     * key for track index, value type is number.
     */
    MD_KEY_TRACK_INDEX = "track_index",

    /**
     * key for track type, value type is number, see @MediaType .
     */
    MD_KEY_TRACK_TYPE = "track_type",

    /**
     * key for codec mime type, value type is string.
     */
    MD_KEY_CODEC_MIME = "codec_mime",

    /**
     * key for duration, value type is number.
     */
    MD_KEY_DURATION = "duration",

    /**
     * key for bitrate, value type is number.
     */
    MD_KEY_BITRATE = "bitrate",

    /**
     * key for video width, value type is number.
     */
    MD_KEY_WIDTH = "width",

    /**
     * key for video height, value type is number.
     */
    MD_KEY_HEIGHT = "height",

    /**
     * key for video frame rate, value type is number.
     */
    MD_KEY_FRAME_RATE = "frame_rate",

    /**
     * key for audio channel count, value type is number
     */
    MD_KEY_AUD_CHANNEL_COUNT = "channel_count",

    /**
     * key for audio sample rate, value type is number
     */
    MD_KEY_AUD_SAMPLE_RATE = "sample_rate",
  }

  interface VideoRecorderProfile {
    /**
     * Indicates the audio bit rate.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    readonly audioBitrate: number;

    /**
     * Indicates the number of audio channels.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    readonly audioChannels: number;

    /**
     * Indicates the audio encoding format.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    readonly audioCodec: CodecMimeType;

    /**
     * Indicates the audio sampling rate.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    readonly audioSampleRate: number;

    /**
     * Indicates the output file format.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    readonly fileFormat: ContainerFormatType;

    /**
     * Indicates the video bit rate.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    readonly videoBitrate: number;

    /**
     * Indicates the video encoding format.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    readonly videoCodec: CodecMimeType;

    /**
     * Indicates the video width.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    readonly videoFrameWidth: number;

    /**
     * Indicates the video height.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    readonly videoFrameHeight: number;

    /**
     * Indicates the video frame rate.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    readonly videoFrameRate: number;
  }

  /**
   * Enumerates audio source type for recorder.
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @devices phone, tablet, tv, wearable, car
   */
  enum AudioSourceType {
    /**
     * default audio source type.
    */
    AUDIO_SOURCE_TYPE_DEFAULT = 0,
    /**
     * source type mic.
    */
    AUDIO_SOURCE_TYPE_MIC = 1,
  }

  /**
   * Enumerates video source type for recorder.
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @devices phone, tablet, tv, wearable, car
   */
  enum VideoSourceType {
    /**
     * surface raw data.
    */
    VIDEO_SOURCE_TYPE_SURFACE_YUV = 0,
    /**
     * surface ES data.
    */
    VIDEO_SOURCE_TYPE_SURFACE_ES = 1,
  }

  interface VideoRecorderConfig {
    /**
     * audio source type, details see @AudioSourceType .
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    audioSourceType: AudioSourceType;
    /**
     * video source type, details see @VideoSourceType .
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    videoSourceType: VideoSourceType;
    /**
     * video recorder profile, can get by "getVideoRecorderProfile", details see @VideoRecorderProfile .
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    profile:VideoRecorderProfile;
    /**
     * video output uri.support two kind of uri now.
     * format like: scheme + "://" + "context".
     * file:  file://path
     * fd:    fd://fd
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    url: string;
    /**
     * Sets the video rotation angle in output file, and for the file to playback. mp4 support.
     * the range of rotation angle should be {0, 90, 180, 270}, default is 0.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    rotation?: number;
    /**
     * geographical location information.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    location?: Location;
  }

  interface MediaDescription {
    /**
     * key:value pair, key see @MediaDescriptionKey .
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    [key : string]: Object;
  }

  /**
   * Enumerates seek mode.
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @devices phone, tablet, tv, wearable, car
   */
  enum SeekMode {
    /**
     * seek to the next sync frame of the given timestamp
     * @since 8
     */
    SEEK_NEXT_SYNC = 0,
    /**
     * seek to the previous sync frame of the given timestamp
     * @since 8
     */
    SEEK_PREV_SYNC = 1,
  }

  /**
   * Enumerates Codec MIME types.
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @devices phone, tablet, tv, wearable, car
   */
   enum CodecMimeType {
    /**
     * H.263 codec MIME type.
     * @since 8
     */
    VIDEO_H263 = 'video/h263',
    /**
     * H.264 codec MIME type.
     * @since 8
     */
    VIDEO_AVC = 'video/avc',
    /**
     * MPEG2 codec MIME type.
     * @since 8
     */
    VIDEO_MPEG2 = 'video/mpeg2',
    /**
     * MPEG4 codec MIME type
     * @since 8
     */
    VIDEO_MPEG4 = 'video/mp4v-es',

    /**
     * VP8 codec MIME type
     * @since 8
     */
    VIDEO_VP8 = 'video/x-vnd.on2.vp8',

    /**
     * AAC codec MIME type.
     * @since 8
     */
    AUDIO_AAC = 'audio/mp4a-latm',

    /**
     * vorbis codec MIME type.
     * @since 8
     */
    AUDIO_VORBIS = 'audio/vorbis',

    /**
     * flac codec MIME type.
     * @since 8
     */
    AUDIO_FLAC = 'audio/flac',
  }
}
export default media;
