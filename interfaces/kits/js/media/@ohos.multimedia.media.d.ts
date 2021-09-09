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
   * Create the VideoRecorder instance to manage audio recording.
   * @sysCap SystemCapability.Multimedia.Media
   * @devices
   */
  function createVideoRecorder(id: string): VideoRecorder;

  /**
   * Create the CodecDescriptionHelper instance to manage codec information.
   * @sysCap SystemCapability.Multimedia.Media
   * @devices
   */
  function getCodecDescriptionHelper(): CodecDescriptionHelper;
  /**
   * Create the MediaTranscode instance to manage transcode.
   * @sysCap SystemCapability.Multimedia.Media
   * @devices
   */
  function createMediaTranscode(): MediaTranscode;
  /**
   * Create the MediaComposer instance to manage compose.
   * @sysCap SystemCapability.Multimedia.Media
   * @devices
   */
  function createMediaComposer(): MediaComposer;
  /**
   * Create the MediaDecoder instance to manage decoder.
   * @sysCap SystemCapability.Multimedia.Media
   * @devices
   */
  function createMediaDecoder(): MediaDecoder;

  enum ErrorCode {
    /**
     * ok
     */
    MSERR_OK = 0,

    /**
     * opertation not be permitted
     */
    MSERR_OPERATE_NOT_PERMIT = 1,

    /**
     * IO error
     */
     MSERR_IO = 5,

    /**
     * no memory
     */
    MSERR_NO_MEMORY = 12,

    /**
     * invalid argument
     */
    MSERR_INVALID_VAL = 22,

    /**
     * network timeout
     */
    MSERR_TIMEOUT = 110,

    /**
     * unkown error
     */
    MSERR_UNKNOWN = 512,

    /**
     * meida service died
     */
    MSERR_SERVICE_DIED = 513,

    /**
     * the state is not support this operation
     */
    MSERR_INVALID_STATE = 514,

    /**
     * unsupport interface
     */
    MSERR_UNSUPPORT = 515,
  }

  /**
   * Create the VideoPlayer instance to manage video play
   * @since 6
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.Multimedia.media'
   * @param  videoplayer topic id
   * @permission N/A
   */
  function createVideoPlayer(id: string):VideoPlayer;

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

  /**
  * Describes playback status
  */
  type VideoState = 'playing' | 'paused' | 'stopped';

  interface VideoPlayer {
    /**
     * start to play video resource
     * @devices
     * @since 6
     * @sysCap SystemCapability.Multimedia.Media
     */
    play(): void;

    /**
     * pause playing
     * @devices
     * @since 6
     * @sysCap SystemCapability.Multimedia.Media
     */
    pause(): void;

    /**
     * stop playing
     * @devices
     * @since 6
     * @sysCap SystemCapability.Multimedia.Media
     */
    stop(): void;

    /**
     * prepare playing
     * @devices
     * @since 6
     * @sysCap SystemCapability.Multimedia.Media
     */
    prepare():void;

    /**
     * reset playing
     * @devices
     * @since 6
     * @sysCap SystemCapability.Multimedia.Media
     */
    reset(): void;

    /**
     * Goes to a specified playback position
     * @devices
     * @since 6
     * @sysCap SystemCapability.Multimedia.Media
     * @param time Target playback position
     */
    seek(timeMs: number): void;

    /**
     * Sets the specified volume.
     * @devices
     * @since 6
     * @sysCap SystemCapability.Multimedia.Media
     * @param volume Volume to set
     */
    setVolume(vol: number): void;

    /**
     * release video resource
     * @devices
     * @since 6
     * @sysCap SystemCapability.Multimedia.Media
     */
    release(): void;

    /**
     * video resource URI
     * @devices
     * @since 6
     * @sysCap SystemCapability.Multimedia.Media
     */
    src: string;

    /**
     * Whether to loop video playback
     * @devices
     * @since 6
     * @sysCap SystemCapability.Multimedia.Media
     * @option true "The video looping function is enabled."
     * @option false "The video looping function is disabled."
     */
    loop: boolean;

    /**
     * video playback speed
     * @devices
     * @since 6
     * @sysCap SystemCapability.Multimedia.Media
     */
    playbackSpeed: number;

    /**
     * video resource's height
     * @devices
     * @since 6
     * @sysCap SystemCapability.Multimedia.Media
     */
    readonly height: number;

    /**
     * video resource's width
     * @devices
     * @since 6
     * @sysCap SystemCapability.Multimedia.Media
     */
    readonly width: number;

     /**
     * Current playback position
     * @devices
     * @since 6
     * @sysCap SystemCapability.Multimedia.Media
     */
    readonly currenTime:number;

     /**
     * Playback duration
     * @devices
     * @since 6
     * @sysCap SystemCapability.Multimedia.Media
     */
    readonly duration: number;

     /**
     * Playback status
     * @devices
     * @since 6
     * @sysCap SystemCapability.Multimedia.Media
     */
    readonly state: VideoState;

    /**
     * called when play, pause, finish, stop, volumeChange event complete.
     * @devices
     * @since 6
     * @sysCap SystemCapability.Multimedia.Media
     */
    on(type: 'play' | 'pause' | 'finish' | 'volumeChange' | 'stop', callback: ()=>{}):void;

    /**
     * Called when an error has occurred.
     * @devices
     * @since 6
     * @sysCap SystemCapability.Multimedia.Media
     */
    on(type: 'error', callback: ErrorCallback):void;

    /**
     * Called when an prepare finished.
     * @devices
     * @since 6
     * @sysCap SystemCapability.Multimedia.Media
     */
    on(type: 'prepared', callback: Callback<number>):void;

    /**
     * Called when an seek event finished.
     * @devices
     * @since 6
     * @sysCap SystemCapability.Multimedia.Media
     */
    on(type: 'timeUpdate', callback: Callback<number>):void;
  }

  enum AudioSourceType {
    /**
     * Microphone
     */
     MIC = 1,
  }

  /**
   * Enumerates audio encoding formats.
   * @since 6
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @devices phone, tablet, tv, wearable
   */
  enum AudioEncoder {
    /**
     * Default audio encoding format, which is AMR-NB.
     */
    DEFAULT = 0,

    /**
     * Indicates the AMR-NB audio encoding format.
     */
    AMR_NB = 1,

    /**
     * Indicates the AMR-WB audio encoding format.
     */
    AMR_WB = 2,

    /**
     * Advanced Audio Coding Low Complexity (AAC-LC).
     */
    AAC_LC = 3,

    /**
     * High-Efficiency Advanced Audio Coding (HE-AAC).
     */
    HE_AAC = 4
  }

  /**
   * Enumerates audio output formats.
   * @since 6
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @devices phone, tablet, tv, wearable
   */
  enum AudioOutputFormat {
    /**
     * Default audio output format, which is Moving Pictures Expert Group 4 (MPEG-4).
     */
    DEFAULT = 0,

    /**
     * Indicates the Moving Picture Experts Group-4 (MPEG4) media format.
     */
    MPEG_4 = 2,

    /**
     * Indicates the Adaptive Multi-Rate Narrowband (AMR-NB) media format.
     */
    AMR_NB = 3,

    /**
     * Indicates the Adaptive Multi-Rate Wideband (AMR-WB) media format.
     */
    AMR_WB = 4,

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

  enum VideoEncoder {
    /**
     * H.264/MPEG-4 AVC
     */
    H264 = 1,
  }

  enum FileFormat {
    /**
     * MPEG4 format
     */
    MP4 = 1,

    /**
     * M4A format
     */
    M4A = 2,
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

  interface VideoRecorderConfig {
    /**
     * Audio encoding format.
     * @devices
     */
    audioEncoder?: AudioEncoder;

    /**
     * Audio encoding bit rate.
     * @devices
     */
    audioEncodeBitRate?: number;

    /**
     * Audio sampling rate.
     * @devices
     */
    audioSampleRate?: number;

    /**
     * Number of audio channels.
     * @devices
     */
    numberOfChannels?: number;

    /**
     * Video encoding format.
     * @devices
     */
    videoEncoder?: VideoEncoder;

    /**
     * Video encoding bit rate.
     * @devices
     */
    videoEncodeBitRate?: number;

    /**
     * Video frame height.
     * @devices
     */
    videoHeight?: number;

    /**
     * Video frame width.
     * @devices
     */
    videoWidth?: number;

    /**
     * Video frame rate.
     * @devices
     */
    videoFrameRate?: number;

    /**
     * Mute the voice.
     * @devices
     */
    muted?: boolean;

    /**
     * Output file format.
     * @devices
     */
    fileFormat?: FileFormat;

    /**
     * Output video file path.
     * @devices
     */
    videoPath: string;

    /**
     * Video file thumbnail path.
     * @devices
     */
    thumbPath: string;
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

  interface VideoRecorder {
    /**
     * Prepare recording.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    prepare(config: VideoRecorderConfig): void;

    /**
     * Start recording.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    start(): void;

    /**
     * Pause recording.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    pause(): void;

    /**
     * Resume recording.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    resume(): void;

    /**
     * Stop recording.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    stop(): void;

    /**
     * Release recorder resources.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    release(): void;

    /**
     * Reset recorder.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    reset(): void;

    /**
     * Called when prepare, start, pause, resume, stop, release or reset event complete.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    on(type: 'prepare' | 'start' | 'pause' | 'resume' | 'stop' | 'release' | 'reset', callback: ()=>{}): void;

    /**
     * Called when an error has occurred.
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    on(type: 'error', callback: ErrorCallback): void;
  }

  /**
   * codec information manager.
   * @sysCap SystemCapability.Multimedia.Media
   * @devices
   */
  interface CodecDescriptionHelper {
    /**
     * 获取支持的编解码器，callbak方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    getSupportedCodecs(callback: AsyncCallback<Array<CodecDescription>>): void;
    /**
     * 获取支持的编解码器，promise方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    getSupportedCodecs(): Promise<Array<CodecDescription>>;
    /**
     * 获取支持的解码器，callbak方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    getSupportedDecoders(callback: AsyncCallback<Array<CodecDescription>>): void;
    /**
     * 获取支持的解码器，promise方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    getSupportedDecoders(): Promise<Array<CodecDescription>>;
    /**
     * 获取支持的编码器，callbak方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    getSupportedEncoders(callback: AsyncCallback<Array<CodecDescription>>): void;
    /**
     * 获取支持的编码器，promise方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    getSupportedEncoders(): Promise<Array<CodecDescription>>;
    /**
     * 找到解码器，callbak方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    findDecoder(format: CodecFormat, callback: AsyncCallback<CodecDescription>): void;
    /**
     * 找到解码器，promise方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    findDecoder(format: CodecFormat): Promise<CodecDescription>;
    /**
     * 找到编码器，callbak方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    findEncoder(format: CodecFormat, callback: AsyncCallback<CodecDescription>): void;
    /**
     * 找到编码器，promise方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    findEncoder(format: CodecFormat): Promise<CodecDescription>;
    /**
     * 某个mime是否支持解码，callbak方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    isDecodeSupportedByMime(mime: String, callback: AsyncCallback<boolean>): void;
    /**
     * 某个mime是否支持解码，promise方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    isDecodeSupportedByMime(mime: String): Promise<boolean>;
    /**
     * 某个mime是否支持编码，callbak方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    isEncodeSupportedByMime(mime: String, callback: AsyncCallback<boolean>): void;
    /**
     * 某个mime是否支持编码，promise方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    isEncodeSupportedByMime(mime: String): Promise<boolean>;
    /**
     * 某个format是否支持解码，callbak方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    isDecoderSupportedByFormat(format: CodecFormat, callback: AsyncCallback<boolean>): void;
    /**
     * 某个format是否支持解码，promise方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    isDecoderSupportedByFormat(format: CodecFormat): Promise<boolean>;
    /**
     * 某个format是否支持编码，callbak方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    isEncoderSupportedByFormat(format: CodecFormat, callback: AsyncCallback<boolean>): void;
    /**
     * 某个format是否支持编码，promise方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    isEncoderSupportedByFormat(format: CodecFormat): Promise<boolean>;
    /**
     * 获取支持的mimes，callbak方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    getSupportedMimes(callback: AsyncCallback<Array<string>>): void;
    /**
     * 获取支持的mimes，promise方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    getSupportedMimes(): Promise<Array<string>>;
  }
  /**
   * 编解码信息。
   * @sysCap SystemCapability.Multimedia.Media
   * @devices
   */
  interface CodecDescription {
    /**
     * mime类型的队列
     * @devices
     */
    mimeTypes: Array<string>;
    /**
     * 名字
     * @devices
     */
    name: string;
    /**
     * 编解码类型
     * @devices
     */
    format: Array<CodecFormat>;
    /**
     * 是否是音频
     * @devices
     */
    isAudio: boolean;
    /**
     * 是否编码
     * @devices
     */
    isEncoder: boolean;
    /**
     * 是否软件
     * @devices
     */
    isSoftware: boolean;
  }
  /**
   * 编解码格式
   * @sysCap SystemCapability.Multimedia.Media
   * @devices
   */
  interface CodecFormat {
    /**
     * mime类型
     * @devices
     */
    mineType: string;
    /**
     * 比特率模式
     * @devices
     */
    bitrateMode: number;
    /**
     * 比特率
     * @devices
     */
    bitrate: number;
    /**
     * capture速率
     * @devices
     */
    captureRate: number;
    /**
     * 通道数
     * @devices
     */
    channelCount: number;
    /**
     * 颜色模式
     * @devices
     */
    colorModel: number;
    /**
     * 时长
     * @devices
     */
    duration: number;
    /**
     * 帧间隔
     * @devices
     */
    frameInterval: number;
    /**
     * 帧率
     * @devices
     */
    frameRate: number;
    /**
     * 高度
     * @devices
     */
    height: number;
    /**
     * 宽度
     * @devices
     */
    width: number;
    /**
     * 采样率
     * @devices
     */
    sampleRate: number;
  }
  /**
   * 媒体文件格式
   * @sysCap SystemCapability.Multimedia.Media
   * @devices
   */
  enum MediaFileFormat {
    /**
     * 3GPP
     */
    FORMAT_3GPP = 0,
    /**
     * HEIF
     */
    FORMAT_HEIF = 1,
    /**
     * MPEG4
     */
    FORMAT_MPEG4 = 2,
    /**
     * WEBM
     */
    FORMAT_WEBM = 3,
  }
  /**
   * 媒体合成器
   * @sysCap SystemCapability.Multimedia.Media
   * @devices
   */
  class MediaComposer {
    /**
     * 分解某个视频源，回调方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    extractSource(src: string, callback: AsyncCallback<ComposerTracks>): void;
    /**
     * 分解某个视频源，promise方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    extractSource(src: string):Promise<ComposerTracks>;
    /**
     * 添加视频轨道，回调方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    addTrack(track: ComposerTrack, callback: AsyncCallback<void>): void;
    /**
     * 添加视频轨道，promise方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    addTrack(track: ComposerTrack): Promise<void>;
    /**
     * 移除视频轨道，回调方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    removeTrack(track: ComposerTrack, callback: AsyncCallback<void>): void;
    /**
     * 移除视频轨道，promise方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    removeTrack(track: ComposerTrack): Promise<void>;
    /**
     * 合成视频轨道，回调方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    compose(filepath: string, callback: AsyncCallback<void>): void;
    /**
     * 合成视频轨道，promise方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    compose(filepath: string): Promise<void>;
    /**
     * 释放，回调方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    release(callback: AsyncCallback<void>): void;
    /**
     * 释放，promsise方式返回。
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    release():Promise<void>;
  }
  /**
   * 合成的轨道
   * @sysCap SystemCapability.Multimedia.Media
   * @devices
   */
  interface ComposerTrack {
    /**
     * 类型
     * @devices
     */
    readonly kind: string;
    /**
     * 时长
     * @devices
     */
    readonly duration: number;
    /**
     * 音量
     * @devices
     */
    volume: number;
  }
  /**
   * 合成的轨道列表
   * @sysCap SystemCapability.Multimedia.Media
   * @devices
   */
  interface ComposerTracks {
    /**
     * 轨道数
     * @devices
     */
    trackCount: number;
    /**
     * 轨道列表
     * @devices
     */
    tracks: Array<Readonly<ComposerTrack>>;
  }
  /**
   * seek方式
   * @sysCap SystemCapability.Multimedia.Media
   * @devices
   */
  enum SeekMode {
    /**
     * 最近I帧
     */
    REWIND_TO_CLOSEST_SYNC = 1,
    /**
     * 后一个I帧
     */
    REWIND_TO_NEXT_SYNC = 2,
    /**
     * 前一个I帧
     */
    REWIND_TO_PREVIOUS_SYNC = 3,
  }
  /**
   * 解码格式
   * @sysCap SystemCapability.Multimedia.Media
   * @devices
   */
  interface DecoderFormat {
    /**
     * 宽度
     * @devices
     */
    width: number;
    /**
     * 高度
     * @devices
     */
    height: number;
  }
  /**
   * 解码帧
   * @sysCap SystemCapability.Multimedia.Media
   * @devices
   */
  interface DecoderFrame {
    /**
     * 宽度
     * @devices
     */
    width: number;
    /**
     * 高度
     * @devices
     */
    height: number;
    /**
     * 解码buffer
     * @devices
     */
    buffer: ArrayBuffer;
    /**
     * pts
     * @devices
     */
    pts: number;
    /**
     * dts
     * @devices
     */
    dts: number;
  }
  /**
   * 媒体解码器
   * @sysCap SystemCapability.Multimedia.Media
   * @devices
   */
  class MediaDecoder {
    /**
     * 开始解码
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    start(uri: string): void;
    /**
     * seek到某个时间点
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    seek(timeMs: number, mode?: SeekMode): void;
    /**
     * 停止
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    stop(): void;
    /**
     * 释放
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    release(): void;
    /**
     * 监听开始的消息
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    on(type: 'start', callback: Callback<DecoderFormat>): void;
    /**
     * 监听停止，seek，释放的回调
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    on(type: 'stop' | 'seek' | 'release' , callback: Callback<void>): void;
    /**
     * 监听bufferchange消息
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    on(type: 'bufferChange', callback: Callback<DecoderFrame>): void;
    /**
     * 监听error消息
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    on(type: 'error', callback: ErrorCallback): void;
  }
  /**
   * 媒体transcode
   * @sysCap SystemCapability.Multimedia.Media
   * @devices
   */
  interface MediaTranscode {
    /**
     * 开始转码
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    start(params: MediaTranscodeParams);
    /**
     * 释放
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    release();
    /**
     * 监听开始的消息
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    on(type: 'start', callback: Callback<void>): void;
    /**
     * 监听完成的消息
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    on(type: 'complete', callback: Callback<void>): void;
    /**
     * 监听释放的消息
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    on(type: 'release', callback: Callback<void>): void;
    /**
     * 监听错误消息
     * @sysCap SystemCapability.Multimedia.Media
     * @devices
     */
    on(type: 'error', callback: ErrorCallback): void;
  }
  /**
   * 媒体transcode的参数
   * @sysCap SystemCapability.Multimedia.Media
   * @devices
   */
  interface MediaTranscodeParams {
    /**
     * uri
     * @devices
     */
    uri: string;
    /**
     * 音频目标格式
     * @devices
     */
    audioFormat: CodecFormat;
    /**
     * 视频目标格式
     * @devices
     */
    videoFormat: CodecFormat;
    /**
     * 文件目标格式
     * @devices
     */
    fileFormat: MediaFileFormat;
    /**
     * 合成后文件路径
     * @devices
     */
    filepath: string;
  }
}

export default media;
