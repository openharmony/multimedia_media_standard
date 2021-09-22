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

import {ErrorCallback, AsyncCallback, Callback} from './basic';

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
   * Describes audio playback states.
   */
  type AudioState = 'idle' | 'playing' | 'paused' | 'stopped';

  /**
   * Manages and plays audio. Before calling an AudioPlayer method, you must use createAudioPlayer() to create an AudioPlayer instance.
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
     * Reset resources used for audio playback.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     */
    reset(): void;

    /**
     * Releases resources used for audio playback.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     */
    release(): void;

    /**
     * Audio media URI. Mainstream audio formats are supported.
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
    readonly currentTime:number;

    /**
     * Playback duration.
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

  interface AudioRecorderConfig {
    /**
     * Audio source type.
     * @devices
     */
    audioSourceType: AudioSourceType;

    /**
     * Audio encoding format. The default value is DEFAULT.
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
    audioEncodeBitRate?: number;

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
     * Output file format.
     * @devices
     */
    fileFormat?: FileFormat;

    /**
     * Audio output format. The default value is DEFAULT.
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
     * Before resetting audio recording, you must call stop() to stop recording. After audio recording is reset, you must call prepare() to set the recording configurations for another recording.
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
}

export default media;
