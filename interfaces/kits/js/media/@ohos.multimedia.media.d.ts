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
}

export default media;