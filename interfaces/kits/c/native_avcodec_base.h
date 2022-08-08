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
typedef void (*AVCodecOnError)(AVCodec *codec, int32_t errorCode, void *userData);

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
typedef void (*AVCodecOnStreamChanged)(AVCodec *codec, AVFormat *format, void *userData);

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
typedef void (*AVCodecOnNeedInputData)(AVCodec *codec, uint32_t index, AVMemory *data, void *userData);

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
typedef void (*AVCodecOnNewOutputData)(AVCodec *codec, uint32_t index, AVMemory *data,
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
 * @brief Enumerates the MIME types of audio and video codecs
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
extern const char *AVCODEC_MIME_TYPE_VIDEO_AVC;
extern const char *AVCODEC_MIME_TYPE_AUDIO_AAC;

/**
 * @brief The extra data's key of surface Buffer
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
/* Key for timeStamp in surface's extraData, value type is int64 */
extern const char *ED_KEY_TIME_STAMP;
/* Key for endOfStream in surface's extraData, value type is bool */
extern const char *ED_KEY_END_OF_STREAM;

/**
 * @brief Provides the uniform container for storing the media description.
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
/* Key for track type, value type is uint8_t, see @OHMediaType. */
extern const char *MD_KEY_TRACK_TYPE;
/* Key for codec mime type, value type is string. */
extern const char *MD_KEY_CODEC_MIME;
/* Key for duration, value type is int64_t. */
extern const char *MD_KEY_DURATION;
/* Key for bitrate, value type is uint32_t. */
extern const char *MD_KEY_BITRATE;
/* Key for max input size, value type is uint32_t */
extern const char *MD_KEY_MAX_INPUT_SIZE;
/* Key for video width, value type is uint32_t */
extern const char *MD_KEY_WIDTH;
/* Key for video height, value type is uint32_t */
extern const char *MD_KEY_HEIGHT;
/* Key for video pixel format, value type is int32_t, see @AVPixelFormat */
extern const char *MD_KEY_PIXEL_FORMAT;
/* key for audio raw format, value type is uint32_t , see @AudioSampleFormat */
extern const char *MD_KEY_AUDIO_SAMPLE_FORMAT;
/* Key for video frame rate, value type is double. */
extern const char *MD_KEY_FRAME_RATE;
/* video encode bitrate mode, the value type is int32_t, see @OHVideoEncodeBitrateMode */
extern const char *MD_KEY_VIDEO_ENCODE_BITRATE_MODE;
/* encode profile, the value type is number. see @OHAVCProfile, OHAACProfile. */
extern const char *MD_KEY_PROFILE;
/* Key for audio channel count, value type is uint32_t */
extern const char *MD_KEY_AUD_CHANNEL_COUNT;
/* Key for audio sample rate, value type is uint32_t */
extern const char *MD_KEY_AUD_SAMPLE_RATE;

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
} OHMediaType;

/**
 * @brief AVC Profile
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
typedef enum OHAVCProfile {
    AVC_PROFILE_BASELINE = 0,
    AVC_PROFILE_HIGH = 4,
    AVC_PROFILE_MAIN = 8,
} OHAVCProfile;

/**
 * @brief AAC Profile
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
typedef enum OHAACProfile {
    AAC_PROFILE_LC = 0,
} OHAACProfile;

#ifdef __cplusplus
}
#endif

#endif // NATIVE_AVCODEC_BASE_H
