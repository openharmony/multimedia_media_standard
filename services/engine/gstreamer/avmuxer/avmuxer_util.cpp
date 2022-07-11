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
    explicit MultiValue(int32_t val)
    {
        val_.intVal = val;
    }
    explicit MultiValue(const char *val)
    {
        val_.stringVal = val;
    }
    union Val {
        int32_t intVal;
        const char *stringVal;
    } val_ = {0};
};

struct FormatInfo {
    std::string mux_;
    std::set<std::string> mimeTypes_;
};

struct MimeInfo {
    std::string innerType_;
    std::string parse_;
    MediaType mediaType_;
};

struct CapsInfo {
    std::string key_;
    GType type_;
    MultiValue value_;
};

const std::map<std::string, FormatInfo> FORMAT_INFO = {
    {"mp4", {"mp4mux", {"video/avc", "video/mp4v-es", "video/h263", "audio/mp4a-latm", "audio/mpeg"}}},
    {"m4a", {"mp4mux", {"audio/mp4a-latm"}}},
};

const std::map<const std::string, MimeInfo> MIME_INFO = {
    {"video/avc", {"video/x-h264", "h264parse", MEDIA_TYPE_VID}},
    {"video/h263", {"video/x-h263", "", MEDIA_TYPE_VID}},
    {"video/mp4v-es", {"video/mpeg", "mpeg4videoparse", MEDIA_TYPE_VID}},
    {"audio/mp4a-latm", {"audio/mpeg", "aacparse", MEDIA_TYPE_AUD}},
    {"audio/mpeg", {"audio/mpeg", "", MEDIA_TYPE_AUD}}
};

std::map<std::string, std::vector<CapsInfo>> optionCapsMap = {
    {"video/avc", {
        {"alignment", G_TYPE_STRING, MultiValue("nal")},
        {"stream-format", G_TYPE_STRING, MultiValue("byte-stream")}
    }},
    {"video/h263", {
    }},
    {"video/mp4v-es", {
        {"mpegversion", G_TYPE_INT, MultiValue(4)},
        {"systemstream", G_TYPE_BOOLEAN, MultiValue(FALSE)}
    }},
    {"audio/mp4a-latm", {
        {"mpegversion", G_TYPE_INT, MultiValue(4)},
        {"stream-format", G_TYPE_STRING, MultiValue("adts")}
    }},
    {"audio/mpeg", {
        {"mpegversion", G_TYPE_INT, MultiValue(1)},
        {"layer", G_TYPE_INT, MultiValue(3)}
    }}
};

static int32_t parseParam(FormatParam &param, const MediaDescription &trackDesc, const std::string &mimeType)
{
    MediaType mediaType;
    CHECK_AND_RETURN_RET_LOG(AVMuxerUtil::FindMediaType(mimeType, mediaType), MSERR_INVALID_VAL, "Illegal mimeType");
    if (mediaType == MEDIA_TYPE_VID) {
        CHECK_AND_RETURN_RET_LOG(trackDesc.GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, param.width),
            MSERR_INVALID_VAL, "Failed to get MD_KEY_WIDTH");
        CHECK_AND_RETURN_RET_LOG(trackDesc.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, param.height),
            MSERR_INVALID_VAL, "Failed to get MD_KEY_HEIGHT");
        CHECK_AND_RETURN_RET_LOG(trackDesc.GetIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, param.frameRate),
            MSERR_INVALID_VAL, "Failed to get MD_KEY_FRAME_RATE");
        MEDIA_LOGD("width is: %{public}d, height is: %{public}d, frameRate is: %{public}d",
            param.width, param.height, param.frameRate);
    } else if (mediaType == MEDIA_TYPE_AUD) {
        CHECK_AND_RETURN_RET_LOG(
            trackDesc.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, param.channels),
            MSERR_INVALID_VAL, "Failed to get MD_KEY_CHANNEL_COUNT");
        CHECK_AND_RETURN_RET_LOG(trackDesc.GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, param.rate),
            MSERR_INVALID_VAL, "Failed to get MD_KEY_SAMPLE_RATE");
        MEDIA_LOGD("channels is: %{public}d, rate is: %{public}d", param.channels, param.rate);
    } else {
        MEDIA_LOGD("Faild to check track type");
        return MSERR_INVALID_VAL;
    }

    return MSERR_OK;
}

static void AddOptionCaps(GstCaps *src_caps, const std::string &mimeType)
{
    for (auto& elements : optionCapsMap[mimeType]) {
        switch (elements.type_) {
            case G_TYPE_BOOLEAN:
            case G_TYPE_INT:
                gst_caps_set_simple(src_caps,
                    elements.key_.c_str(),
                    elements.type_,
                    elements.value_.val_.intVal,
                    nullptr);
                break;
            case G_TYPE_STRING:
                gst_caps_set_simple(src_caps,
                    elements.key_.c_str(),
                    elements.type_,
                    elements.value_.val_.stringVal,
                    nullptr);
                break;
            default:
                break;
        }
    }
}

static GstCaps *CreateCaps(FormatParam &param, const std::string &mimeType)
{
    GstCaps *src_caps = nullptr;
    std::string innerType;
    MediaType mediaType;
    CHECK_AND_RETURN_RET_LOG(AVMuxerUtil::FindInnerTypes(mimeType, innerType), nullptr, "Illegal mimeType");
    CHECK_AND_RETURN_RET_LOG(AVMuxerUtil::FindMediaType(mimeType, mediaType), nullptr, "Illegal mimeType");
    if (mediaType == MEDIA_TYPE_VID) {
        src_caps = gst_caps_new_simple(innerType.c_str(),
            "width", G_TYPE_INT, param.width,
            "height", G_TYPE_INT, param.height,
            "framerate", GST_TYPE_FRACTION, param.frameRate, 1,
            nullptr);
    } else if (mediaType == MEDIA_TYPE_AUD) {
        src_caps = gst_caps_new_simple(innerType.c_str(),
            "channels", G_TYPE_INT, param.channels,
            "rate", G_TYPE_INT, param.rate,
            nullptr);
    } else {
        MEDIA_LOGE("Failed to check track type");
        return nullptr;
    }
    AddOptionCaps(src_caps, mimeType);

    return src_caps;
}

bool AVMuxerUtil::SetCaps(const MediaDescription &trackDesc, const std::string &mimeType,
    GstCaps **src_caps)
{
    MEDIA_LOGD("Set %{public}s caps", mimeType.c_str());
    CHECK_AND_RETURN_RET_LOG(src_caps != nullptr, false, "src_caps is nullptr");
    FormatParam param;
    bool ret = parseParam(param, trackDesc, mimeType);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, false, "Failed to call parseParam");
    *src_caps = CreateCaps(param, mimeType);

    return true;
}

std::vector<std::string> AVMuxerUtil::FindFormat()
{
    std::vector<std::string> formatList;
    for (auto& formats : FORMAT_INFO) {
        formatList.push_back(formats.first);
    }
    return formatList;
}

bool AVMuxerUtil::FindMux(const std::string &format, std::string &mux)
{
    if (FORMAT_INFO.find(format) == FORMAT_INFO.end()) {
        return false;
    }
    mux = FORMAT_INFO.at(format).mux_;
    return true;
}

bool AVMuxerUtil::FindMimeTypes(const std::string &format, std::set<std::string> &mimeTypes)
{
    if (FORMAT_INFO.find(format) == FORMAT_INFO.end()) {
        return false;
    }
    mimeTypes = FORMAT_INFO.at(format).mimeTypes_;
    return true;
}

bool AVMuxerUtil::FindInnerTypes(const std::string &mimeType, std::string &innerType)
{
    if (MIME_INFO.find(mimeType) == MIME_INFO.end()) {
        return false;
    }
    innerType = MIME_INFO.at(mimeType).innerType_;
    return true;
}

bool AVMuxerUtil::FindParse(const std::string &mimeType, std::string &parse)
{
    if (MIME_INFO.find(mimeType) == MIME_INFO.end()) {
        return false;
    }
    parse = MIME_INFO.at(mimeType).parse_;
    return true;
}

bool AVMuxerUtil::FindMediaType(const std::string &mimeType, MediaType &mediaType)
{
    if (MIME_INFO.find(mimeType) == MIME_INFO.end()) {
        return false;
    }
    mediaType = MIME_INFO.at(mimeType).mediaType_;
    return true;
}
}
}