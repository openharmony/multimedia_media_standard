/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef NATIVE_AVCODEC_BASE_H
#define NATIVE_AVCODEC_BASE_H

#include <stdint.h>
#include <stdio.h>
#include "native_averrors.h"
#include "native_avformat.h"
#include "native_avmemory.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NativeWindow NativeWindow;
typedef struct AVCodec AVCodec;

/**
 * @brief Enumerates the MIME types of audio and video codecs
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
extern const char* AVCODEC_MIME_TYPE_VIDEO_AVC;
extern const char* AVCODEC_MIME_TYPE_AUDIO_AAC;

/**
 * @brief Enumerate the categories of AVCodec's Buffer tags
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
typedef enum AVCodecBufferFlags {
    AVCODEC_BUFFER_FLAGS_NONE = 0,
    /* Indicates that the Buffer is an End-of-Stream frame */
    AVCODEC_BUFFER_FLAGS_EOS = 1 << 0,
    /* Indicates that the Buffer contains keyframes */
    AVCODEC_BUFFER_FLAGS_SYNC_FRAME = 1 << 1,
    /* Indicates that the data contained in the Buffer is only part of a frame */
    AVCODEC_BUFFER_FLAGS_INCOMPLETE_FRAME = 1 << 2,
    /* Indicates that the Buffer contains Codec-Specific-Data */
    AVCODEC_BUFFER_FLAGS_CODEC_DATA = 1 << 3,
} AVCodecBufferFlags;

/**
 * @brief Define the Buffer description information of AVCodec
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
typedef struct AVCodecBufferAttr {
    /* Presentation timestamp of this Buffer in microseconds */
    int64_t pts;
    /* The size of the data contained in the Buffer in bytes */
    int32_t size;
    /* The starting offset of valid data in this Buffer */
    int32_t offset;
    /* The flags this Buffer has, which is also a combination of multiple {@link AVCodecBufferFlags}. */
    uint32_t flags;
} AVCodecBufferAttr;

/**
 * @brief When an error occurs in the running of the AVCodec instance, the function pointer will be called
 * to report specific error information.
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @param codec AVCodec instance
 * @param errorCode specific error code
 * @param userData User specific data
 * @since 9
 * @version 1.0
 */
typedef void (* AVCodecOnError)(AVCodec *codec, int32_t errorCode, void *userData);

/**
 * @brief When the output stream changes, the function pointer will be called to report the new stream description
 * information. It should be noted that the life cycle of the AVFormat pointer
 * is only valid when the function pointer is called, and it is forbidden to continue to access after the call ends.
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @param codec AVCodec instance
 * @param format New output stream description information
 * @param userData User specific data
 * @since 9
 * @version 1.0
 */
typedef void (* AVCodecOnStreamChanged)(AVCodec *codec, AVFormat *format, void *userData);

/**
 * @brief When AVCodec needs new input data during the running process,
 * the function pointer will be called and carry an available Buffer to fill in the new input data.
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @param codec AVCodec instance
 * @param index The index corresponding to the newly available input buffer.
 * @param data New available input buffer.
 * @param userData User specific data
 * @since 9
 * @version 1.0
 */
typedef void (* AVCodecOnNeedInputData)(AVCodec *codec, uint32_t index, AVMemory *data, void *userData);

/**
 * @brief When new output data is generated during the operation of AVCodec, the function pointer will be
 * called and carry a Buffer containing the new output data. It should be noted that the life cycle of the
 * AVCodecBufferAttr pointer is only valid when the function pointer is called. , which prohibits continued
 * access after the call ends.
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @param codec AVCodec instance
 * @param index The index corresponding to the new output Buffer.
 * @param data Buffer containing the new output data
 * @param attr The description of the new output Buffer, please refer to {@link AVCodecBufferAttr}
 * @param userData specified data
 * @since 9
 * @version 1.0
 */
typedef void (* AVCodecOnNewOutputData)(AVCodec *codec, uint32_t index, AVMemory *data,
    AVCodecBufferAttr *attr, void *userData);

/**
 * @brief A collection of all asynchronous callback function pointers in AVCodec. Register an instance of this
 * structure to the AVCodec instance, and process the information reported through the callback to ensure the
 * normal operation of AVCodec.
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @param onError Monitor AVCodec operation errors, refer to {@link AVCodecOnError}
 * @param onStreamChanged Monitor codec stream information, refer to {@link AVCodecOnStreamChanged}
 * @param onNeedInputData Monitoring codec requires input data, refer to {@link AVCodecOnNeedInputData}
 * @param onNeedInputData Monitor codec to generate output data, refer to {@link onNeedInputData}
 * @since 9
 * @version 1.0
 */
typedef struct AVCodecAsyncCallback {
    AVCodecOnError onError;
    AVCodecOnStreamChanged onStreamChanged;
    AVCodecOnNeedInputData onNeedInputData;
    AVCodecOnNewOutputData onNeedOutputData;
} AVCodecAsyncCallback;

/**
 * @brief The extra data's key of surface Buffer 
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
typedef struct OHSurfaceBufferExtraDataKey {
    /* Key for timeStamp in surface's extraData, value type is int64 */
    const char* ED_KEY_TIME_STAMP = "timeStamp";

    /* Key for endOfStream in surface's extraData, value type is bool */
    const char* ED_KEY_END_OF_STREAM = "endOfStream";
} OHSurfaceBufferExtraDataKey;

/**
 * @brief Provides the uniform container for storing the media description.
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
typedef struct OHMediaDescriptionKey {
    /* Key for track index, value type is uint32_t. */
    const char* MD_KEY_TRACK_INDEX = "track_index";
    /* Key for track type, value type is uint8_t, see @OHMediaType. */
    const char* MD_KEY_TRACK_TYPE = "track_type";
    /* Key for codec mime type, value type is string. */
    const char* MD_KEY_CODEC_MIME = "codec_mime";
    /* Key for duration, value type is int64_t. */
    const char* MD_KEY_DURATION = "duration";
    /* Key for bitrate, value type is uint32_t. */
    const char* MD_KEY_BITRATE = "bitrate";
    /* Key for max input size, value type is uint32_t */
    const char* MD_KEY_MAX_INPUT_SIZE = "max_input_size";
    /* Key for max video encoder fps, value type is double */
    const char* MD_KEY_MAX_ENCODER_FPS = "max_encoder_fps";
    /* Key for video width, value type is uint32_t */
    const char* MD_KEY_WIDTH = "width";
    /* Key for video height, value type is uint32_t */
    const char* MD_KEY_HEIGHT = "height";
    /* Key for video pixel format, value type is int32_t, see @OHVideoPixelFormat */
    const char* MD_KEY_PIXEL_FORMAT = "pixel_format";
    /* key for audio raw format, value type is uint32_t , see @AudioSampleFormat */
    const char* MD_KEY_AUDIO_SAMPLE_FORMAT = "audio_sample_format";
    /* Key for video frame rate, value type is double. */
    const char* MD_KEY_FRAME_RATE = "frame_rate";
    /* Key for video capture rate, value type is double */
    const char* MD_KEY_CAPTURE_RATE = "capture_rate";
    /**
     * Key for the interval of key frame. value type is int32_t, the unit is milliseconds.
     * A negative value means no key frames are requested after the first frame. A zero
     * value means a stream containing all key frames is requested.
     */
    const char* MD_KEY_I_FRAME_INTERVAL = "i_frame_interval";
    /* Key for the request a I-Frame immediately. value type is boolean */
    const char* MD_KEY_REQUEST_I_FRAME = "req_i_frame";
    /* repeat encode the previous frame after the pts in milliseconds, value type is int32_t */
    const char* MD_KEY_REPEAT_FRAME_AFTER = "repeat_frame_after";
    /* suspend input surface data. the value type is int32_t, 0:not suspend, 1:suspend. */
    const char* MD_KEY_SUSPEND_INPUT_SURFACE = "suspend_input_surface";
    /* video encode bitrate mode, the value type is int32_t, see @OHVideoEncodeBitrateMode */
    const char* MD_KEY_VIDEO_ENCODE_BITRATE_MODE = "video_encode_bitrate_mode";
    /* encode profile, the value type is number.
     * see @OHAVCProfile, OHHEVCProfile, OHMPEG2Profile, OHMPEG4Profile, OHH263Profile, OHAACProfile
     */
    const char* MD_KEY_PROFILE = "codec_profile";
    /* encode quality, the value type is int32_t. */
    const char* MD_KEY_QUALITY = "codec_quality";
    /* true video picture top position in the buffer, the value type is int32_t. */
    const char* MD_KEY_RECT_TOP = "rect_top";
    /* true video picture bottom position in the buffer, the value type is int32_t. */
    const char* MD_KEY_RECT_BOTTOM = "rect_bottom";
    /* true video picture left position in the buffer, the value type is int32_t. */
    const char* MD_KEY_RECT_LEFT = "rect_left";
    /* true video picture right position in the buffer, the value type is int32_t. */
    const char* MD_KEY_RECT_RIGHT = "rect_right";
    /* video raw data color standard. the value type is int32_t. */
    const char* MD_KEY_COLOR_STANDARD = "color_standard";
    /* Key for audio channel count, value type is uint32_t */
    const char* MD_KEY_AUD_CHANNEL_COUNT = "channel_count";
    /* Key for audio sample rate, value type is uint32_t */
    const char* MD_KEY_AUD_SAMPLE_RATE = "sample_rate";
    /* custom key prefix, media service will pass through to HAL. */
    const char* MD_KEY_CUSTOM = "vendor.custom";
} OHMediaDescriptionKey;

/**
 * @brief Media type.
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
typedef enum OHMediaType {
    /* track is audio. */
    MEDIA_TYPE_AUD = 0,
    /* track is video. */
    MEDIA_TYPE_VID = 1,
    /* track is subtitle. */
    MEDIA_TYPE_SUBTITLE = 2,
} OHMediaType;

/**
 * @brief The format of video pixel.
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
typedef enum OHVideoPixelFormat {
    /* yuv 420 planar. */
    YUVI420 = 1,
    /* NV12. yuv 420 semiplanar. */
    NV12 = 2,
    /* NV21. yvu 420 semiplanar. */
    NV21 = 3,
    /* format from surface. */
    SURFACE_FORMAT = 4,
    /* RGBA. */
    RGBA = 5,
} OHVideoPixelFormat;

/**
 * @brief The bitrate mode of video encoder.
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
typedef enum OHVideoEncodeBitrateMode {
    /* constant bit rate mode. */
    CBR = 0,
    /* variable bit rate mode. */
    VBR = 1,
    /* constant quality mode. */
    CQ = 2,
} OHVideoEncodeBitrateMode;

/**
 * @brief AVC Profile
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
typedef enum OHAVCProfile {
    AVC_PROFILE_BASELINE = 0,
    AVC_PROFILE_CONSTRAINED_BASELINE = 1,
    AVC_PROFILE_CONSTRAINED_HIGH = 2,
    AVC_PROFILE_EXTENDED = 3,
    AVC_PROFILE_HIGH = 4,
    AVC_PROFILE_HIGH_10 = 5,
    AVC_PROFILE_HIGH_422 = 6,
    AVC_PROFILE_HIGH_444 = 7,
    AVC_PROFILE_MAIN = 8,
} OHAVCProfile;

/**
 * @brief HEVC Profile
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
typedef enum OHHEVCProfile {
    HEVC_PROFILE_MAIN = 0,
    HEVC_PROFILE_MAIN_10 = 1,
    HEVC_PROFILE_MAIN_STILL = 2,
    HEVC_PROFILE_MAIN_10_HDR10 = 3,
} OHHEVCProfile;

/**
 * @brief MPEG2 Profile
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
typedef enum OHMPEG2Profile {
    MPEG2_PROFILE_422 = 0,
    MPEG2_PROFILE_HIGH = 1,
    MPEG2_PROFILE_MAIN = 2,
    MPEG2_PROFILE_SNR = 3,
    MPEG2_PROFILE_SIMPLE = 4,
    MPEG2_PROFILE_SPATIAL = 5,
} OHMPEG2Profile;

/**
 * @brief MPEG4 Profile
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
typedef enum OHMPEG4Profile {
    MPEG4_PROFILE_ADVANCED_CODING = 0,
    MPEG4_PROFILE_ADVANCED_CORE = 1,
    MPEG4_PROFILE_ADVANCED_REAL_TIME = 2,
    MPEG4_PROFILE_ADVANCED_SCALABLE = 3,
    MPEG4_PROFILE_ADVANCED_SIMPLE = 4,
    MPEG4_PROFILE_BASIC_ANIMATED = 5,
    MPEG4_PROFILE_CORE = 6,
    MPEG4_PROFILE_CORE_SCALABLE = 7,
    MPEG4_PROFILE_HYBRID = 8,
    MPEG4_PROFILE_MAIN = 9,
    MPEG4_PROFILE_NBIT = 10,
    MPEG4_PROFILE_SCALABLE_TEXTURE = 11,
    MPEG4_PROFILE_SIMPLE = 12,
    MPEG4_PROFILE_SIMPLE_FBA = 13,
    MPEG4_PROFILE_SIMPLE_FACE = 14,
    MPEG4_PROFILE_SIMPLE_SCALABLE = 15,
} OHMPEG4Profile;

/**
 * @brief H263 Profile
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
typedef enum OHH263Profile {
    H263_PROFILE_BACKWARD_COMPATIBLE = 0,
    H263_PROFILE_BASELINE = 1,
    H263_PROFILE_H320_CODING = 2,
    H263_PROFILE_HIGH_COMPRESSION = 3,
    H263_PROFILE_HIGH_LATENCY = 4,
    H263_PROFILE_ISW_V2 = 5,
    H263_PROFILE_ISW_V3 = 6,
    H263_PROFILE_INTERLACE = 7,
    H263_PROFILE_INTERNET = 8,
} OHH263Profile;

/**
 * @brief AAC Profile
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
typedef enum OHAACProfile {
    AAC_PROFILE_LC = 0,
    AAC_PROFILE_ELD = 1,
    AAC_PROFILE_ERLC = 2,
    AAC_PROFILE_HE = 3,
    AAC_PROFILE_HE_V2 = 4,
    AAC_PROFILE_LD = 5,
    AAC_PROFILE_MAIN = 6,
} OHAACProfile;

#ifdef __cplusplus
}
#endif

#endif // NATIVE_AVCODEC_BASE_H
