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

#include "avmuxer_util.h"
#include <tuple>
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMuxerUtil"};
}

namespace OHOS {
namespace Media {

struct MultiValue {
    explicit MultiValue(int32_t val) {
        val_.intVal = val;
    }
    explicit MultiValue(const char *val) {
        val_.stringVal = val;
    }
    union Val {
        int32_t    intVal;
        const char *stringVal;
    } val_;
};

std::map<CodecMimeType, std::vector<std::tuple<std::string, GType, MultiValue>>> optionCapsMap = {
    {CODEC_MIMIE_TYPE_VIDEO_AVC, {
        {"alignment", G_TYPE_STRING, MultiValue("nal")},
        {"stream-format", G_TYPE_STRING, MultiValue("byte-stream")}
    }},
    {CODEC_MIMIE_TYPE_VIDEO_H263, {
    }},
    {CODEC_MIMIE_TYPE_VIDEO_MPEG4, {
        {"mpegversion", G_TYPE_INT, MultiValue(4)},
        {"systemstream", G_TYPE_BOOLEAN, MultiValue(FALSE)}
    }},
    {CODEC_MIMIE_TYPE_AUDIO_AAC, {
        {"mpegversion", G_TYPE_INT, MultiValue(4)},
        {"stream-format", G_TYPE_STRING, MultiValue("adts")}
    }},
    {CODEC_MIMIE_TYPE_AUDIO_MPEG, {
        {"mpegversion", G_TYPE_INT, MultiValue(1)},
        {"layer", G_TYPE_INT, MultiValue(3)}
    }}
};

bool AVMuxerUtil::isVideo(CodecMimeType type) {
    if (type == CODEC_MIMIE_TYPE_VIDEO_H263 || type == CODEC_MIMIE_TYPE_VIDEO_AVC ||
        type ==CODEC_MIMIE_TYPE_VIDEO_MPEG2 || type ==CODEC_MIMIE_TYPE_VIDEO_HEVC || 
        type == CODEC_MIMIE_TYPE_VIDEO_MPEG4) {
        return true;
    }
    return false;
}

static int32_t parseParam(FormatParam &param, const MediaDescription &trackDesc, CodecMimeType type) {
    if (AVMuxerUtil::isVideo(type)) {
        CHECK_AND_RETURN_RET_LOG(trackDesc.GetIntValue(MD_KEY_WIDTH, param.width) == true,
            MSERR_INVALID_VAL, "Failed to get MD_KEY_WIDTH");
        CHECK_AND_RETURN_RET_LOG(trackDesc.GetIntValue(MD_KEY_HEIGHT, param.height) == true,
            MSERR_INVALID_VAL, "Failed to get MD_KEY_HEIGHT");
        CHECK_AND_RETURN_RET_LOG(trackDesc.GetIntValue(MD_KEY_FRAME_RATE, param.frameRate) == true,
            MSERR_INVALID_VAL, "Failed to get MD_KEY_FRAME_RATE");
        MEDIA_LOGD("width is: %{public}d, height is: %{public}d, frameRate is: %{public}d",
            param.width, param.height, param.frameRate);
    } else {
        CHECK_AND_RETURN_RET_LOG(trackDesc.GetIntValue(MD_KEY_CHANNEL_COUNT, param.channels) == true,
            MSERR_INVALID_VAL, "Failed to get MD_KEY_CHANNEL_COUNT");
        CHECK_AND_RETURN_RET_LOG(trackDesc.GetIntValue(MD_KEY_SAMPLE_RATE, param.rate) == true,
            MSERR_INVALID_VAL, "Failed to get MD_KEY_SAMPLE_RATE");
        MEDIA_LOGD("channels is: %{public}d, rate is: %{public}d", param.channels, param.rate);
    }

    return MSERR_OK;
}

static void AddOptionCaps(GstCaps *src_caps, CodecMimeType type)
{
    for (auto& elements : optionCapsMap[type]) {
        switch(std::get<1>(elements)) {
            case G_TYPE_BOOLEAN:
            case G_TYPE_INT:
                gst_caps_set_simple(src_caps,
                    std::get<0>(elements).c_str(), std::get<1>(elements), std::get<2>(elements).val_.intVal,
                    nullptr);
                break;
            case G_TYPE_STRING:
                gst_caps_set_simple(src_caps,
                    std::get<0>(elements).c_str(), std::get<1>(elements), std::get<2>(elements).val_.stringVal,
                    nullptr);
                break;
            default:
                break;
        }
    }
}

static void CreateCaps(FormatParam &param, const std::string &mimeType, GstCaps *src_caps, CodecMimeType type)
{
    if (AVMuxerUtil::isVideo(type)) {
        src_caps = gst_caps_new_simple(std::get<0>(MIME_MAP_TYPE.at(mimeType)).c_str(),
            "width", G_TYPE_INT, param.width,
            "height", G_TYPE_INT, param.height,
            "framerate", GST_TYPE_FRACTION, param.frameRate, 1,
            nullptr);
    } else {
        src_caps = gst_caps_new_simple(std::get<0>(MIME_MAP_TYPE.at(mimeType)).c_str(),
            "channels", G_TYPE_INT, param.channels,
            "rate", G_TYPE_INT, param.rate,
            nullptr);
    }
    AddOptionCaps(src_caps, type);
}

int32_t AVMuxerUtil::SetCaps(const MediaDescription &trackDesc, const std::string &mimeType,
    GstCaps *src_caps, CodecMimeType type)
{
    MEDIA_LOGD("Set %{public}s cpas", mimeType.c_str());
    bool ret;
    FormatParam param;
    ret = parseParam(param, trackDesc, type);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_VAL, "Failed to call parseParam");
    CreateCaps(param, mimeType, src_caps, type);

    return MSERR_OK;
}

int32_t PushCodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
    GstAppSrc *src, GstShMemWrapAllocator *allocator)
{
    GstMemory *mem = gst_shmem_wrap(GST_ALLOCATOR_CAST(allocator), sampleData);
    GstBuffer *buffer = gst_buffer_new();
    gst_buffer_append_memory(buffer, mem);
    GST_BUFFER_DTS(buffer) = static_cast<uint64_t>(sampleInfo.timeUs * 1000);
    GST_BUFFER_PTS(buffer) = static_cast<uint64_t>(sampleInfo.timeUs * 1000);
    if (sampleInfo.flags == AVCODEC_BUFFER_FLAG_SYNC_FRAME) {
        gst_buffer_set_flags(buffer, GST_BUFFER_FLAG_DELTA_UNIT);
    }

    GstFlowReturn ret = gst_app_src_push_buffer(src, buffer);
    CHECK_AND_RETURN_RET_LOG(ret == GST_FLOW_OK, MSERR_INVALID_OPERATION, "Failed to call gst_app_src_push_buffer");

    return MSERR_OK;
}

int32_t AVMuxerUtil::WriteData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
    GstAppSrc *src, std::map<int, TrackInfo>& trackInfo, GstShMemWrapAllocator *allocator)
{
    int32_t ret = PushCodecData(sampleData, sampleInfo, src, allocator);
    CHECK_AND_RETURN_RET_LOG(ret == GST_FLOW_OK, MSERR_INVALID_OPERATION, "Failed to call PushCodecData");
    return MSERR_OK;
}
}
}