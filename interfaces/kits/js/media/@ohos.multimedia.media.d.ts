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
  /**
   * Creates an audio decoder instance by name.
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @param name decoder name.
   * @param callback Callback used to return audio decoder instance if the operation is successful; returns null otherwise.
   */
  function createAudioDecoderByName(name: string, callback: AsyncCallback<AudioDecodeProcessor>):void;
  /**
   * Creates a audio encoder instance by codec name.
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @param name decoder name.
   * @return A Promise instance used to return audio decoder instance if the operation is successful; returns null otherwise.
   */
  function createAudioDecoderByName(name: string): Promise<AudioDecodeProcessor>;
  /**
   * Creates an audio decoder instance by codec MIME.
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @param codecMime codec MIME, see @CodecMimeType .
   * @param callback Callback used to return auddo decoder instance if the operation is successful; returns null otherwise.
   */
  function createAudioDecoderByMime(codecMime: string, callback: AsyncCallback<AudioDecodeProcessor>):void;
  /**
   * Creates an audio decoder instance by codec MIME.
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @param codecMime codec MIME, see @CodecMimeType .
   * @return A Promise instance used to return audio decoder instance if the operation is successful; returns null otherwise.
   */
   function createAudioDecoderByMime(codecMime: string): Promise<AudioDecodeProcessor>;
  /**
   * Creates an audio encoder instance by name.
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @param name encoder name.
   * @param callback Callback used to return auddo encoder instance if the operation is successful; returns null otherwise.
   */
  function createAudioEncoderByName(name: string, callback: AsyncCallback<AudioEncodeProcessor>):void;
  /**
   * Creates an audio encoder instance by name.
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @param name encoder name.
   * @return A Promise instance used to return audio encoder instance if the operation is successful; returns null otherwise.
   */
   function createAudioEncoderByName(name: string): Promise<AudioEncodeProcessor>;
  /**
   * Creates an audio encoder instance by code MIME.
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @param codecMime codec MIME, see @CodecMimeType .
   * @param callback Callback used to return auddo encoder instance if the operation is successful; returns null otherwise.
   */
  function createAudioEncoderByMime(codecMime: string, callback: AsyncCallback<AudioEncodeProcessor>):void;
  /**
   * Creates an audio encoder instance by code MIME.
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @param codecMime codec MIME, see @CodecMimeType .
   * @return A Promise instance used to return audio encoder instance if the operation is successful; returns null otherwise.
   */
  function createAudioEncoderByMime(codecMime: string): Promise<AudioEncodeProcessor>;
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
     * an IO error occured.
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
    readonly currentTime: number;

    /**
     * Playback duration. When the data source does not support seek, it returns - 1, such as a live broadcast scenario.
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
   * Enumerates audio encoding formats.
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
   * Enumerates audio output formats.
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

  enum MediaType {
    /**
     * track is audio.
     */
    MEDIA_TYPE_AUD = 0,
    /**
     * track is video.
     */
    MEDIA_TYPE_VID = 1,
    /**
     * track is subtitle.
     */
    MEDIA_TYPE_SUBTITLE = 2,
  }
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
     * key for max input size, value type is number, unit is Byte.
     */
    MD_KEY_MAX_INPUT_SIZE = "max_input_size",

    /**
     * key for max video encoder fps, value type is number, The value is 100 times the actual frame rate.
     * example: The real frame rate is 23.99, so this value is 2399.
     */
    MD_KEY_MAX_ENCODER_FPS = "max_encoder_fps",

    /**
     * key for video width, value type is number.
     */
    MD_KEY_WIDTH = "width",

    /**
     * key for video height, value type is number.
     */
    MD_KEY_HEIGHT = "height",

    /**
     * key for video pixerformat, value type is number.
     */
    MD_KEY_PIXEL_FORMAT = "pixel_format",

    /**
     * key for audio raw format, value type is number.
     */
    MD_KEY_AUDIO_RAW_FORMAT = "audio_raw_format",

    /**
     * key for video framerate, value type is number,The value is 100 times the actual frame rate.
     * example: The real frame rate is 23.99, so this value is 2399.
     */
    MD_KEY_FRAME_RATE = "frame_rate",

    /**
     * key for video framerate, value type is number,The value is 100 times the actual frame rate.
     * example: The real frame rate is 23.99, so this value is 2399.
     */
    MD_KEY_CAPTURE_RATE = "capture_rate",

    /**
     * key for key frame interval. value type is number, the unit is milliseconds.
     */
    MD_KEY_I_FRAME_INTERVAL = "i_frame_interval",
    /**
     * key for request a I frame imediately. value type is number.
     */
    MD_KEY_REQUEST_I_FRAME = "req_i_frame",
    /**
     * repeat endcode the previous frame after the pts in milliseconds. the value type is number.
     */
    MD_KEY_REPEAT_FRAME_AFTER = "repeat_frame_after",
    /**
     * suspend input surface data. the value type is number, 0:not suspend, 1:suspend.
     */
    MD_KEY_SUSPEND_INPUT_SURFACE = "suspend_input_surface",
    /**
     * video encode bitrate mode, the value type is number, see @VideoEncodeBitrateMode .
     */
    MD_KEY_VIDEO_ENCODE_BITRATE_MODE = "video_encode_bitrate_mode",
    /**
     * encode profile, the value type is number. see the codec's profile definition.
     */
    MD_KEY_PROFILE = "codec_profile",
    /**
     * encode quality, the value type is number.
     */
    MD_KEY_QUALITY = "codec_quality",
    /**
     * true video picture top position in the buffer, the value type is number.
     */
    MD_KEY_RECT_TOP = "rect_top",
    /**
     * true video picture bottom position in the buffer, the value type is number.
     */
    MD_KEY_RECT_BOTTOM = "rect_bottom",
    /**
     * true video picture left position in the buffer, the value type is number.
     */
    MD_KEY_RECT_LEFT = "rect_left",
    /**
    * true video picture right position in the buffer, the value type is number.
    */
    MD_KEY_RECT_RIGHT = "rect_right",
    /**
     * video raw data color standard. the value type is number, see @VideoColorStandard .
     */
    MD_KEY_COLOR_STANDARD = "color_standard",

    /**
     * key for audio channel count, value type is number
     */
    MD_KEY_AUD_CHANNEL_COUNT = "channel_count",

    /**
     * key for audio sample rate, value type is number
     */
    MD_KEY_AUD_SAMPLE_RATE = "sample_rate",

    /**
     * custom key, the value type is an ArrayBuffer, it will be passthrough to HDI.
     */
    MD_KEY_CUSTOM = "vendor.custom",
  }
export default media;
  interface MediaDescription {
    /**
     * key:value pair, key see @MediaDescriptionKey .
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    [key : string]: any;
  }
  /**
   * Enumerates frame flags.
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @devices phone, tablet, tv, wearable, car
   */
  enum FrameFlags {
    /**
     * end of stream flags.
     */
    EOS_FRAME = 1 << 0,
    /**
     * the sample is a sync frame
     */
    SYNC_FRAME = 1 << 1,

    /**
     * sample is partial frame.
     */
    PARTIAL_FRAME = 1 << 2 ,
    /**
    * sample is the codec data.
    */
    CODEC_DATA = 1 << 3,
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
     */
    VIDEO_H263 = 'video/h263',
    /**
     * H.264 codec MIME type.
     */
    VIDEO_AVC = 'video/avc',
    /**
     * MPEG2 codec MIME type.
     */
    VIDEO_MPEG2 = 'video/mpeg2',

    /**
     * H265 codec MIME type.
     */
    VIDEO_HEVC = 'video/hevc',

    /**
     * MPEG4 codec MIME type
     */
    VIDEO_MPEG4 = 'video/mp4v-es',

    /**
     * VP8 codec MIME type
     */
    VIDEO_VP8 = 'video/x-vnd.on2.vp8',

    /**
     * VP9 codec MIME type
     */
    VIDEO_VP9 = 'video/x-vnd.on2.vp9',

    /**
     * AMR-NB codec MIME type
     */
    AUDIO_AMR_NB = 'audio/3gpp',

    /**
     * AMR-WB codec MIME type
     */
    AUDIO_AMR_WB = 'audio/amr-wb',

    /**
     * MPEG-1 audio codec MIME type.
     */
    AUDIO_MPEG = 'audio/mpeg',

    /**
     * AAC codec MIME type.
     */
    AUDIO_AAC = 'audio/mp4a-latm',

    /**
     * vorbis codec MIME type.
     */
    AUDIO_VORBIS = 'audio/vorbis',

    /**
     * opus codec MIME type.
     */
    AUDIO_OPUS = 'audio/opus',

    /**
     * flac codec MIME type.
     */
    AUDIO_FLAC = 'audio/flac',

    /**
     * PCM MIME type.
     */
    AUDIO_RAW = 'audio/raw',
  }
  /**
   * Enumerates AVCodecType.
   *
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @devices phone, tablet, tv, wearable, car
   */
  enum AVCodecType{
    /**
     * video encoder.
     */
    AVCODEC_TYPE_VIDEO_ENCODER = 0,
    /**
     * video decoder.
     */
    AVCODEC_TYPE_VIDEO_DECODER = 1,
    /**
     * audio encoder.
     */
    AVCODEC_TYPE_AUDIO_ENCODER = 2,
    /**
     * audio decoder.
     */
    AVCODEC_TYPE_AUDIO_DECODER = 3,
  }
  interface AVCodecInfo {
    /**
     * name of this codec, used to create the codec instance.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    name: string;
    /**
     * codec type, see @AVCodecType .
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    type: AVCodecType;
    /**
     * MIME type of this codec, see @CodecMimeType .
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    mimeType: string;
    /**
     * is the codec hardware accelerated, ture accelerated, or false.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    isHardwareAccelerated: boolean;
    /**
     * is the codec software implemented. true software or false.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    isSoftwareOnly: boolean;
    /**
     * is the codec vendor provide. true vendor or false.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    isVendor: boolean;
  }

  /**
   * Enumerates audio raw format type.
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @devices phone, tablet, tv, wearable, car
   */
  enum AudioRawFormat {
    /**
     * signed 8 bits.
     */
    AUDIO_PCM_S8 = 1,
    /**
     * unsigned 8 bits.
     */
    AUDIO_PCM_8 = 2,
    /**
     * signed 16 bits in big endian.
     */
    AUDIO_PCM_S16_BE = 3,
    /**
     * signed 16 bits in little endian.
     */
    AUDIO_PCM_S16_LE = 4,
    /**
     * unsigned 16 bits in big endian.
     */
    AUDIO_PCM_16_BE = 5,
    /**
     * unsigned 16 bits in little endian.
     */
    AUDIO_PCM_16_LE = 6,
    /**
     * signed 24 bits in big endian.
     */
    AUDIO_PCM_S24_BE = 7,
    /**
     * signed 24 bits in little endian.
     */
    AUDIO_PCM_S24_LE = 8,
    /**
     * unsigned 24 bits in big endian.
     */
    AUDIO_PCM_24_BE = 9,
    /**
     * unsigned 24 bits in little endian.
     */
    AUDIO_PCM_24_LE = 10,
    /**
     * signed 32 bits in big endian.
     */
    AUDIO_PCM_S32_BE = 11,
    /**
     * signed 32 bits in little endian.
     */
    AUDIO_PCM_S32_LE = 12,
    /**
     * unsigned 32 bits in big endian.
     */
    AUDIO_PCM_32_BE = 13,
    /**
     * unsigned 32 bits in little endian.
     */
    AUDIO_PCM_32_LE = 14,
    /**
     * float 32 bits in big endian.
     */
    AUDIO_PCM_F32_BE = 15,
    /**
     * float 32 bits in little endian.
     */
    AUDIO_PCM_F32_LE = 16,
  }
  /**
   * Enumerates AAC profile.
   * @since 8
   * @SysCap SystemCapability.Multimedia.Media
   * @import import media from '@ohos.multimedia.media'
   * @devices phone, tablet, tv, wearable, car
   */
   enum AACProfile {
    /**
     * profile LC
    */
    AAC_PROFILE_LC = 0,
    /**
     * profile ELD
    */
    AAC_PROFILE_ELD = 1,
    /**
     * profile ERLC
    */
    AAC_PROFILE_ERLC = 2,
    /**
     * profile HE
    */
    AAC_PROFILE_HE = 3,
    /**
     * profile HE_V2
    */
    AAC_PROFILE_HE_V2 = 4,
    /**
     * profile LD
    */
    AAC_PROFILE_LD = 5,
    /**
     * profile MAIN
    */
    AAC_PROFILE_MAIN = 6,
  }

  interface Range {
    /**
     * minimum value of the range.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    min: number;
    /**
     * maximum value of the range.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    max: number;
  }

  interface AudioCaps {
    /**
     * codec info, see @AVCodecInfo
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    readonly codecInfo: AVCodecInfo;
    /**
     * supported bitrate range.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    readonly supportedBitrate: Range;
    /**
     * supported channel range.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    readonly supportedChannel: Range;
    /**
     * supported audio raw format. @AudioRawFormat .
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    readonly supportedFormats: Array<AudioRawFormat>;
    /**
     * supported sample rate array.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    readonly supportedSampleRates: Array<number>;
    /**
     * supported codec profile number
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    readonly supportedProfiles: Array<number>;
    /**
     * supported codec level array.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    readonly supportedLevels: Array<number>;
    /**
     * supported encode complexity range.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    readonly supportedComplexity: Range;
  }
  interface CodecError extends Error {
    code: MediaErrorCode;
  }
  interface CodecBuffer {
    /**
     * buffer index in fwk
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    index: number;
    /**
     * frame data buffer.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    data: ArrayBuffer;
    /**
     * offset of ArrayBuffer data.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    offset: number;
    /**
     * valid data length of Aarrybuffer.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    length: number;
    /**
     * size of Aary
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    flags: FrameFlags;
    /**
     * timestamp in milliseconds.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     */
    timeMs:number;
  }
  interface AudioDecodeProcessor {
    /**
     * configure audio decoder using MediaDescription.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param desc configure Description, see @MediaDescription .
     * @param callback A callback instance used to return when configure completed.
     */
    configure(desc: MediaDescription, callback: AsyncCallback<void>): void;
     /**
      * configure audio decoder using MediaDescription.
      * @devices phone, tablet, tv, wearable
      * @since 8
      * @SysCap SystemCapability.Multimedia.Media
      * @param desc configure Description, see @MediaDescription .
      * @return A Promise instance used to return when configure completed.
      */
    configure(desc: MediaDescription): Promise<void>;
    /**
     * prepare audio decoder, it will request the resource.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param callback A callback instance used to return when prepare completed.
     */
    prepare(callback: AsyncCallback<void>): void;
     /**
      * prepare audio decoder, it will request the resource.
      * @devices phone, tablet, tv, wearable
      * @since 8
      * @SysCap SystemCapability.Multimedia.Media
      * @return A Promise instance used to return when prepare completed.
      */
    prepare(): Promise<void>;
    /**
     * start audio decoder, after start, app can input audio data.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param callback A callback instance used to return when start completed.
     */
    start(callback: AsyncCallback<void>): void;
     /**
      * start audio decoder, after start, app can input audio data.
      * @devices phone, tablet, tv, wearable
      * @since 8
      * @SysCap SystemCapability.Multimedia.Media
      * @return A Promise instance used to return when start completed.
      */
    start(): Promise<void>;
    /**
     * stop decode.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param callback A callback instance used to return when stop completed.
     */
    stop(callback: AsyncCallback<void>): void;
     /**
      * stop decode.
      * @devices phone, tablet, tv, wearable
      * @since 8
      * @SysCap SystemCapability.Multimedia.Media
      * @return A Promise instance used to return when stop completed.
      */
    stop(): Promise<void>;
    /**
     * flush audio coder cached buffer.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param callback A callback instance used to return when flush completed.
     */
    flush(callback: AsyncCallback<void>): void;
     /**
      * flush audio coder cached buffer.
      * @devices phone, tablet, tv, wearable
      * @since 8
      * @SysCap SystemCapability.Multimedia.Media
      * @return A Promise instance used to return when flush completed.
      */
    flush(): Promise<void>;
    /**
     * reset audio decoder, it will release the resource.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param callback A callback instance used to return when reset completed.
     */
    reset(callback: AsyncCallback<void>): void;
     /**
      * reset audio decoder, it will release the resource.
      * @devices phone, tablet, tv, wearable
      * @since 8
      * @SysCap SystemCapability.Multimedia.Media
      * @return A Promise instance used to return when reset completed.
      */
    reset(): Promise<void>;
    /**
     * input frame data after start.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param buffer the input audio buffer.
     * @param callback Callback used to return when input buffer completed.
     */
    queueInput(buffer: CodecBuffer, callback: AsyncCallback<void>): void;
    /**
     * input frame data after start.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param buffer the input audio buffer.
     * @return A Promise instance used to return when input buffer completed.
     */
    queueInput(buffer: CodecBuffer): Promise<void>;
    /**
     * release outputbuffer
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param buffer the outout audio buffer.
     * @param isRender true need render the buffer,
     * @param callback Callback used to return when output buffer release completed.
     */
    releaseOutput(buffer: CodecBuffer, callback: AsyncCallback<void>): void;
    /**
     * release outputbuffer
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param buffer the outout audio buffer.
     * @return A Promise instance used to return when release output buffer completed.
     */
    releaseOutput(buffer: CodecBuffer): Promise<void>;
    /**
     * set parameter to audio decoder.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param desc parameter Description, see @MediaDescription .
     * @param callback Callback used to return when set completed.
     */
    setParameter(desc: MediaDescription, callback: AsyncCallback<void>): void;
    /**
     * set parameter to audio decoder.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param desc  parameter Description, see @MediaDescription .
     * @return A Promise instance used to return when set completed.
     */
    setParameter(desc: MediaDescription): Promise<void>;
    /**
     * get output description to audio decoder, can call after "outputAvailable".
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param desc parameter Description, see @MediaDescription .
     * @param callback Callback used to return output MediaDescription.
     */
    getOutputMediaDescription(callback: AsyncCallback<MediaDescription>): void;
     /**
     * set parameter to audio decoder, can call after "outputAvailable".
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param desc  parameter Description, see @MediaDescription .
     * @return A Promise instance used to return output MediaDescription.
     */
    getOutputMediaDescription(): Promise<MediaDescription>;
    /**
    * Listens for audio decoder error events.
    * @devices phone, tablet, tv, wearable
    * @since 6
    * @SysCap SystemCapability.Multimedia.Media
    * @param type Type of the audio decoder error event to listen for.
    * @param callback Callback used to listen for the audio decoder error event, errorcode see @CodecError .
    */
    on(type: 'error', callback: ErrorCallback<CodecError>): void;
    /**
    * Listens for audio decoder outputformat changed or first frame decoded format event. the new output info will callback.
    * @devices phone, tablet, tv, wearable
    * @since 6
    * @SysCap SystemCapability.Multimedia.Media
    * @param type Type of the audio decoder event to listen for.
    * @param callback Callback used to listen for the audio decoder event, new info see @MediaDescription.
    * usualy contain the width and height.
    */
    on(type: 'outputFormatChanged', callback: Callback<MediaDescription>): void;
    /**
    * Listens for audio decoder input buffer.
    * @devices phone, tablet, tv, wearable
    * @since 6
    * @SysCap SystemCapability.Multimedia.Media
    * @param type Type of the audio decoder event to listen for.
    * @param callback Callback used to listen for the audio decoder event, return the input codecbuffer.
    * usualy contain the width and height.
    */
    on(type: 'inputBufferAvailable', callback: Callback<CodecBuffer>): void;
    /**
    * Listens for audio decoder output buffer.
    * @devices phone, tablet, tv, wearable
    * @since 6
    * @SysCap SystemCapability.Multimedia.Media
    * @param type Type of the audio decoder event to listen for.
    * @param callback Callback used to listen for the audio decoder event, return the output codecbuffer.
    */
    on(type: 'outputBufferAvailable', callback: Callback<CodecBuffer>): void;
    /**
     * get the supported audio decoder capabilities.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param callback Callback used to return the supported audio decoder capability.
     */
    getAudioDecoderCaps(callback: AsyncCallback<AudioCaps>): void;
     /**
      * get the supported audio decoder capabilities.
      * @devices phone, tablet, tv, wearable, car
      * @since 8
      * @SysCap SystemCapability.Multimedia.Media
      * @return A Promise instance used to return the supported audio decoder capability.
      */
    getAudioDecoderCaps(): Promise<AudioCaps>;
  }

  interface AudioEncodeProcessor {
    /**
     * configure audio encoder using MediaDescription.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param desc configure Description, see @MediaDescription .
     * @param callback A callback instance used to return when configure completed.
     */
    configure(desc: MediaDescription, callback: AsyncCallback<void>): void;
     /**
      * configure audio encoder using MediaDescription.
      * @devices phone, tablet, tv, wearable
      * @since 8
      * @SysCap SystemCapability.Multimedia.Media
      * @param desc configure Description, see @MediaDescription .
      * @return A Promise instance used to return when configure completed.
      */
    configure(desc: MediaDescription): Promise<void>;
    /**
     * prepare audio encoder, it will request the resource.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param callback A callback instance used to return when prepare completed.
     */
    prepare(callback: AsyncCallback<void>): void;
     /**
      * prepare audio encoder, it will request the resource.
      * @devices phone, tablet, tv, wearable
      * @since 8
      * @SysCap SystemCapability.Multimedia.Media
      * @return A Promise instance used to return when prepare completed.
      */
    prepare(): Promise<void>;
    /**
     * start audio encode, after start, app can input audio data.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param callback A callback instance used to return when start completed.
     */
    start(callback: AsyncCallback<void>): void;
     /**
      * start audio encode, after start, app can input audio data.
      * @devices phone, tablet, tv, wearable
      * @since 8
      * @SysCap SystemCapability.Multimedia.Media
      * @return A Promise instance used to return when start completed.
      */
    start(): Promise<void>;
    /**
     * stop audio encode.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param callback A callback instance used to return when stop completed.
     */
    stop(callback: AsyncCallback<void>): void;
     /**
      * stop audio encode.
      * @devices phone, tablet, tv, wearable
      * @since 8
      * @SysCap SystemCapability.Multimedia.Media
      * @return A Promise instance used to return when stop completed.
      */
    stop(): Promise<void>;
    /**
     * flush audio encoder cached buffer.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param callback A callback instance used to return when flush completed.
     */
    flush(callback: AsyncCallback<void>): void;
     /**
      * flush audio encoder cached buffer.
      * @devices phone, tablet, tv, wearable
      * @since 8
      * @SysCap SystemCapability.Multimedia.Media
      * @return A Promise instance used to return when flush completed.
      */
    flush(): Promise<void>;
    /**
     * reset audio encoder, it will release the resource.
     * @devices phone, tablet, tv, wearable
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param callback A callback instance used to return when reset completed.
     */
    reset(callback: AsyncCallback<void>): void;
     /**
      * reset audio encoder, it will release the resource.
      * @devices phone, tablet, tv, wearable
      * @since 8
      * @SysCap SystemCapability.Multimedia.Media
      * @return A Promise instance used to return when reset completed.
      */
    reset(): Promise<void>;
    /**
     * input frame data after start, this API is used in bytebuffer input mode.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param buffer the input audio buffer.
     * @param callback Callback used to return when input buffer completed.
     */
    queueInput(buffer: CodecBuffer, callback: AsyncCallback<void>): void;
    /**
     * input frame data after start, this API is used in bytebuffer input mode.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param buffer the input audio buffer.
     * @return A Promise instance used to return when input buffer completed.
     */
    queueInput(buffer: CodecBuffer): Promise<void>;
    /**
     * release outputbuffer
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param buffer the outout audio buffer.
     * @param callback Callback used to return when output buffer release completed.
     */
    releaseOutput(buffer: CodecBuffer, callback: AsyncCallback<void>): void;
    /**
     * release outputbuffer
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param buffer the outout audio buffer.
     * @return A Promise instance used to return when release output buffer completed.
     */
    releaseOutput(buffer: CodecBuffer): Promise<void>;
    /**
     * set parameter to audio encoder.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param desc parameter Description, see @MediaDescription .
     * @param callback Callback used to return when set completed.
     */
    setParameter(desc: MediaDescription, callback: AsyncCallback<void>): void;
    /**
     * set parameter to audio encoder.
    * @devices phone, tablet, tv, wearable, car
    * @since 8
    * @SysCap SystemCapability.Multimedia.Media
    * @param desc  parameter Description, see @MediaDescription .
    * @return A Promise instance used to return when set completed.
    */
    setParameter(desc: MediaDescription): Promise<void>;
    /**
     * get output description to audio encoder, can call after "outputAvailable".
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param desc parameter Description, see @MediaDescription .
     * @param callback Callback used to return output MediaDescription.
     */
    getOutputMediaDescription(callback: AsyncCallback<MediaDescription>): void;
     /**
     * set parameter to audio encoder, can call after "outputAvailable".
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param desc  parameter Description, see @MediaDescription .
     * @return A Promise instance used to return output MediaDescription.
     */
    getOutputMediaDescription(): Promise<MediaDescription>;
    /**
    * Listens for audio encoder outputformat changed or first frame encoded format event. the new output info will callback.
    * @devices phone, tablet, tv, wearable
    * @since 6
    * @SysCap SystemCapability.Multimedia.Media
    * @param type Type of the audio encoder event to listen for.
    * @param callback Callback used to listen for the audio encoder event, new info see @MediaDescription.
    * usualy contain the width and height.
    */
    on(type: 'outputFormatChanged', callback: Callback<MediaDescription>): void;
    /**
     * Listens for audio encoder error events.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     * @param type Type of the audio encoder error event to listen for.
     * @param callback Callback used to listen for the audio encoder error event, errorcode see @CodecError .
     */
    on(type: 'error', callback: ErrorCallback<CodecError>): void;
    /**
     * Listens for audio encoder input buffer.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     * @param type Type of the audio encoder event to listen for.
     * @param callback Callback used to listen for the audio encoder event, return the input codecbuffer.
     * usualy contain the width and height.
     */
    on(type: 'inputAvailable', callback: Callback<CodecBuffer>): void;
    /**
     * Listens for audio encoder output buffer.
     * @devices phone, tablet, tv, wearable
     * @since 6
     * @SysCap SystemCapability.Multimedia.Media
     * @param type Type of the audio encoder event to listen for.
     * @param callback Callback used to listen for the audio encoder event, return the output codecbuffer.
     */
    on(type: 'outputAvailable', callback: Callback<CodecBuffer>): void;
    /**
     * get the supported audio encoder capabilities.
     * @devices phone, tablet, tv, wearable, car
     * @since 8
     * @SysCap SystemCapability.Multimedia.Media
     * @param callback Callback used to return the supported audio encoder capability.
     */
    getAudioEncoderCaps(callback: AsyncCallback<AudioCaps>): void;
     /**
      * get the supported audio encoder capabilities.
      * @devices phone, tablet, tv, wearable, car
      * @since 8
      * @SysCap SystemCapability.Multimedia.Media
      * @return A Promise instance used to return the supported audio encoder capability.
      */
    getAudioEncoderCaps(): Promise<AudioCaps>;
  }
}
export default media;