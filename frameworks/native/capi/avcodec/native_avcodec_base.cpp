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

#include "native_avcodec_base.h"

#ifdef __cplusplus
extern "C" {
#endif

const char* AVCODEC_MIME_TYPE_VIDEO_AVC = "video/avc";
const char* AVCODEC_MIME_TYPE_AUDIO_AAC = "audio/mp4a-latm";
const char* ED_KEY_TIME_STAMP = "timeStamp";
const char* ED_KEY_END_OF_STREAM = "endOfStream";
const char* MD_KEY_TRACK_INDEX = "track_index";
const char* MD_KEY_TRACK_TYPE = "track_type";
const char* MD_KEY_CODEC_MIME = "codec_mime";
const char* MD_KEY_DURATION = "duration";
const char* MD_KEY_BITRATE = "bitrate";
const char* MD_KEY_MAX_INPUT_SIZE = "max_input_size";
const char* MD_KEY_MAX_ENCODER_FPS = "max_encoder_fps";
const char* MD_KEY_WIDTH = "width";
const char* MD_KEY_HEIGHT = "height";
const char* MD_KEY_PIXEL_FORMAT = "pixel_format";
const char* MD_KEY_AUDIO_SAMPLE_FORMAT = "audio_sample_format";
const char* MD_KEY_FRAME_RATE = "frame_rate";
const char* MD_KEY_CAPTURE_RATE = "capture_rate";
const char* MD_KEY_I_FRAME_INTERVAL = "i_frame_interval";
const char* MD_KEY_REQUEST_I_FRAME = "req_i_frame";
const char* MD_KEY_REPEAT_FRAME_AFTER = "repeat_frame_after";
const char* MD_KEY_SUSPEND_INPUT_SURFACE = "suspend_input_surface";
const char* MD_KEY_VIDEO_ENCODE_BITRATE_MODE = "video_encode_bitrate_mode";
const char* MD_KEY_PROFILE = "codec_profile";
const char* MD_KEY_QUALITY = "codec_quality";
const char* MD_KEY_RECT_TOP = "rect_top";
const char* MD_KEY_RECT_BOTTOM = "rect_bottom";
const char* MD_KEY_RECT_LEFT = "rect_left";
const char* MD_KEY_RECT_RIGHT = "rect_right";
const char* MD_KEY_COLOR_STANDARD = "color_standard";
const char* MD_KEY_AUD_CHANNEL_COUNT = "channel_count";
const char* MD_KEY_AUD_SAMPLE_RATE = "sample_rate";
const char* MD_KEY_CUSTOM = "vendor.custom";

#ifdef __cplusplus
}
#endif
