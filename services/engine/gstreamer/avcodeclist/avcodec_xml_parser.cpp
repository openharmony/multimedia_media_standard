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
#include "avcodec_xml_parser.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecParser"};
}

namespace OHOS {
namespace Media {
const std::unordered_map<std::string, int> VIDEO_PROFILE_MAP = {
    // H263
    {"H263BackwardCompatible", H263_PROFILE_BACKWARD_COMPATIBLE},
    {"H263Baseline", H263_PROFILE_BASELINE},
    {"H263H320Coding", H263_PROFILE_H320_CODING},
    {"H263HighCompression", H263_PROFILE_HIGH_COMPRESSION},
    {"H263HighLatency", H263_PROFILE_HIGH_LATENCY},
    {"H263ISWV2", H263_PROFILE_ISW_V2},
    {"H263ISWV3", H263_PROFILE_ISW_V3},
    {"H263Interlace", H263_PROFILE_INTERLACE},
    {"H263Internet", H263_PROFILE_INTERNET},
    // H264
    {"AVCBaseline", AVC_PROFILE_BASELINE},
    {"AVCHighCompression", AVC_PROFILE_CONSTRAINED_BASELINE},
    {"AVCConstrainedHigh", AVC_PROFILE_CONSTRAINED_HIGH},
    {"AVCExtended", AVC_PROFILE_EXTENDED},
    {"AVCHigh", AVC_PROFILE_HIGH},
    {"AVCHigh10", AVC_PROFILE_HIGH_10},
    {"AVCHigh422", AVC_PROFILE_HIGH_422},
    {"AVCHigh444", AVC_PROFILE_HIGH_444},
    {"AVCMain", AVC_PROFILE_MAIN},
    // H265
    {"HEVCMain", HEVC_PROFILE_MAIN},
    {"HEVCMain10", HEVC_PROFILE_MAIN_10},
    {"HEVCMain10HDR10", HEVC_PROFILE_MAIN_10_HDR10},
    {"HEVCMain10HDR10Plus", HEVC_PROFILE_MAIN_10_HDR10_PLUS},
    {"HEVCMainStill", HEVC_PROFILE_MAIN_STILL},
    // MPEG2
    {"MPEG2_422", MPEG2_PROFILE_422},
    {"MPEG2High", MPEG2_PROFILE_HIGH},
    {"MPEG2Main", MPEG2_PROFILE_MAIN},
    {"MPEG2SNR", MPEG2_PROFILE_SNR},
    {"MPEG2Simple", MPEG2_PROFILE_SIMPLE},
    {"MPEG2Spatial", MPEG2_PROFILE_SPATIAL},
    // MPEG4
    {"MPEG4AdvancedCoding", MPEG4_PROFILE_ADVANCED_CODING},
    {"MPEG4AdvancedCore", MPEG4_PROFILE_ADVANCED_CORE},
    {"MPEG4AdvancedRealTime", MPEG4_PROFILE_ADVANCED_REAL_TIME},
    {"MPEG4AdvancedScalable", MPEG4_PROFILE_ADVANCED_SCALABLE},
    {"MPEG4AdvancedSimple", MPEG4_PROFILE_ADVANCED_SIMPLE},
    {"MPEG4BasicAnimated", MPEG4_PROFILE_BASIC_ANIMATED},
    {"MPEG4Core", MPEG4_PROFILE_CORE},
    {"MPEG4CoreScalable", MPEG4_PROFILE_CORE_SCALABLE},
    {"MPEG4Hybrid", MPEG4_PROFILE_HYBRID},
    {"MPEG4Main", MPEG4_PROFILE_MAIN},
    {"MPEG4Nbit", MPEG4_PROFILE_NBIT},
    {"MPEG4ScalableTexture", MPEG4_PROFILE_SCALABLE_TEXXTURE},
    {"MPEG4Simple", MPEG4_PROFILE_SIMPLE},
    {"MPEG4SimpleFBA", MPEG4_PROFILE_SIMPLE_FBA},
    {"MPEG4SimpleFace", MPEG4_PROFILE_SIMPLE_FACE},
    {"MPEG4SimpleScalable", MPEG4_PROFILE_SIMPLE_SCALABLE},
    // VP8
    {"VP8Main", VP8_PROFILE_MAIN},
};

const std::unordered_map<std::string, int> AUDIO_PROFILE_MAP = {
    {"AAC_LC", AAC_PROFILE_LC},
    {"AAC_ELD", AAC_PROFILE_ELD},
    {"AAC_ERLC", AAC_PROFILE_ERLC},
    {"AAC_HE", AAC_PROFILE_HE},
    {"AAC_HE_V2", AAC_PROFILE_HE_V2},
    {"AAC_LD", AAC_PROFILE_LD},
    {"AAC_Main", AAC_PROFILE_MAIN},
};

const std::unordered_map<std::string, int> VIDEO_FORMAT_MAP = {
    {"YUVI420", YUVI420},
    {"NV12", NV12},
    {"NV21", NV21},
};

const std::unordered_map<std::string, int> AUDIO_FORMAT_MAP = {
    {"S8", AUDIO_PCM_S8},
    {"8", AUDIO_PCM_8},
    {"S16BE", AUDIO_PCM_S16_BE},
    {"S16LE", AUDIO_PCM_S16_LE},
    {"16BE", AUDIO_PCM_16_BE},
    {"16LE", AUDIO_PCM_16_LE},
    {"S24BE", AUDIO_PCM_S24_BE},
    {"S24LE", AUDIO_PCM_S24_LE},
    {"24BE", AUDIO_PCM_24_BE},
    {"24LE", AUDIO_PCM_24_LE},
    {"S32BE", AUDIO_PCM_S32_BE},
    {"S32LE", AUDIO_PCM_S32_LE},
    {"32BE", AUDIO_PCM_32_BE},
    {"32LE", AUDIO_PCM_32_LE},
    {"F32BE", AUDIO_PCM_F32_BE},
    {"F32LE", AUDIO_PCM_F32_LE},
};

const std::unordered_map<std::string, int> BITRATE_MODE_MAP = {
    {"CBR", CBR},
    {"VBR", VBR},
    {"CQ", CQ},
};

const std::unordered_map<std::string, AVCodecType> CODEC_TYPE_MAP = {
    {"VIDEO_ENCODER", AVCODEC_TYPE_VIDEO_ENCODER},
    {"VIDEO_DECODER", AVCODEC_TYPE_VIDEO_DECODER},
    {"AUDIO_ENCODER", AVCODEC_TYPE_AUDIO_ENCODER},
    {"AUDIO_DECODER", AVCODEC_TYPE_AUDIO_DECODER},
};

AVCodecParser::AVCodecParser()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecParser::~AVCodecParser()
{
    Destroy();
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

bool AVCodecParser::LoadConfiguration()
{
    mDoc_ = xmlReadFile(AVCODEC_CONFIG_FILE, NULL, 0);
    if (mDoc_ == NULL) {
        MEDIA_LOGE("AVCodec xmlReadFile failed");
        return false;
    }
    return true;
}

bool AVCodecParser::Parse()
{
    xmlNode *root = xmlDocGetRootElement(mDoc_);
    if (root == NULL) {
        MEDIA_LOGE("AVCodec xmlDocGetRootElement failed");
        return false;
    }
    if (!ParseInternal(root)) {
        return false;
    }
    return true;
}

void AVCodecParser::Destroy()
{
    if (mDoc_ != NULL) {
        xmlFreeDoc(mDoc_);
    }
    return;
}

bool AVCodecParser::ParseInternal(xmlNode *node)
{
    xmlNode *currNode = node;
    for (; currNode; currNode = currNode->next) {
        if (XML_ELEMENT_NODE == currNode->type) {
            switch (GetNodeNameAsInt(currNode)) {
                case AUDIO_DECODER:
                case AUDIO_ENCODER:
                case VIDEO_DECODER:
                case VIDEO_ENCODER: {
                    ParseData(currNode);
                    break;
                }
                default:
                    ParseInternal(currNode->children);
                    break;
            }
        }
    }
    return true;
}

bool AVCodecParser::TransStrAsRange(const std::string &str, Range &range)
{
    if (str == "null" || str == "") {
        MEDIA_LOGD("str is null");
        return false;
    }
    size_t pos = str.find("-");
    if (pos != str.npos) {
        std::string head = str.substr(0, pos);
        std::string tail = str.substr(pos + 1);
        range.minVal = atoi(head.c_str());
        range.maxVal = atoi(tail.c_str());
    } else {
        MEDIA_LOGD("Can not find the delimiter of \"-\" in : %{public}s", str.c_str());
        return false;
    }
    return true;
}

std::vector<int32_t> AVCodecParser::TransStrAsIntegerArray(std::vector<std::string> &spilt)
{
    std::vector<int32_t> array;
    for (auto iter = spilt.begin(); iter != spilt.end(); iter++) {
        int32_t num = atoi(iter->c_str());
        array.push_back(num);
    }
    return array;
}

std::vector<int32_t> AVCodecParser::TransMapAsIntegerArray(
    const std::unordered_map<std::string, int> &capabilityMap,
    std::vector<std::string> &spilt)
{
    std::vector<int32_t> res;
    for (auto iter = spilt.begin(); iter != spilt.end(); iter++) {
        if (capabilityMap.find(*iter) != capabilityMap.end()) {
            res.push_back(capabilityMap.at(*iter));
        } else {
            MEDIA_LOGD("can not find %{public}s in capabilityMap", iter->c_str());
        }
    }
    return res;
}

bool AVCodecParser::SpiltKeyList(const std::string &str, const std::string &delim, std::vector<std::string> &spilt)
{
    if (str == "") {
        return false;
    }
    std::string strAddDelim  = str;
    if (str.back() != delim.back()) {
        strAddDelim = str + delim;
    }
    size_t pos = 0;
    size_t size = strAddDelim.size();
    for (size_t i = 0; i < size; ++i) {
        pos = strAddDelim.find(delim, i);
        if (pos != strAddDelim.npos) {
            std::string s = strAddDelim.substr(i, pos - i);
            spilt.push_back(s);
            i = pos + delim.size() - 1;
        }
    }
    return true;
}

bool AVCodecParser::SetCapabilityStringData(std::unordered_map<std::string, std::string&> dataMap,
                                            const std::string &capabilityKey, const std::string &capabilityValue)
{
    dataMap.at(capabilityKey) = capabilityValue;
    return true;
}

bool AVCodecParser::SetCapabilityIntData(std::unordered_map<std::string, int32_t&> dataMap,
                                         const std::string &capabilityKey, const std::string &capabilityValue)
{
    if (CODEC_TYPE_MAP.find(capabilityValue) != CODEC_TYPE_MAP.end()) {
        dataMap.at(capabilityKey) = CODEC_TYPE_MAP.at(capabilityValue);
    } else {
        MEDIA_LOGD("The value of %{public}s in the configuration file is incorrect.", capabilityValue.c_str());
        return false;
    }
    return true;
}

bool AVCodecParser::SetCapabilityBoolData(std::unordered_map<std::string, bool&> dataMap,
                                          const std::string &capabilityKey, const std::string &capabilityValue)
{
    if (capabilityValue == "true") {
        dataMap.at(capabilityKey) = true;
    } else if (capabilityValue == "false") {
        dataMap.at(capabilityKey) = false;
    } else {
        MEDIA_LOGD("The value of %{public}s in the configuration file is incorrect.", capabilityValue.c_str());
        return false;
    }
    return true;
}

bool AVCodecParser::SetCapabilityRangeData(std::unordered_map<std::string, Range&> dataMap,
                                           const std::string &capabilityKey, const std::string &capabilityValue)
{
    Range range;
    bool ret = TransStrAsRange(capabilityValue, range);
    CHECK_AND_RETURN_RET_LOG(ret != false, false, "failed:can not trans %{public}s", capabilityValue.c_str());
    dataMap.at(capabilityKey) = range;
    return true;
}

bool AVCodecParser::IsNumberArray(const std::vector<std::string> &strArray)
{
    for (auto iter = strArray.begin(); iter != strArray.end(); iter++) {
        for (char const &c : *iter) {
            if (std::isdigit(c) == 0) {
                return false;
            }
        }
    }
    return true;
}

bool AVCodecParser::SetCapabilityVectorData(std::unordered_map<std::string, std::vector<int32_t>&> dataMap,
                                            const std::string &capabilityKey, const std::string &capabilityValue)
{
    std::vector<std::string> spilt;
    std::vector<int32_t> array;
    bool ret = SpiltKeyList(capabilityValue, ",", spilt);
    CHECK_AND_RETURN_RET_LOG(ret != false, false, "failed:can not split %{public}s", capabilityValue.c_str());
    if (spilt.size() > 0) {
        std::string probe = spilt[0];
        if (VIDEO_PROFILE_MAP.find(probe) != VIDEO_PROFILE_MAP.end()) {
            array = TransMapAsIntegerArray(VIDEO_PROFILE_MAP, spilt);
        } else if (AUDIO_PROFILE_MAP.find(probe) != AUDIO_PROFILE_MAP.end()) {
            array = TransMapAsIntegerArray(AUDIO_PROFILE_MAP, spilt);
        } else if (VIDEO_FORMAT_MAP.find(probe) != VIDEO_FORMAT_MAP.end()) {
            array = TransMapAsIntegerArray(VIDEO_FORMAT_MAP, spilt);
        } else if (AUDIO_FORMAT_MAP.find(probe) != AUDIO_FORMAT_MAP.end()) {
            array = TransMapAsIntegerArray(AUDIO_FORMAT_MAP, spilt);
        } else if (BITRATE_MODE_MAP.find(probe) != BITRATE_MODE_MAP.end()) {
            array = TransMapAsIntegerArray(BITRATE_MODE_MAP, spilt);
        } else if (IsNumberArray(spilt)) {
            array = TransStrAsIntegerArray(spilt);
        } else {
            MEDIA_LOGD("The value of %{public}s in the configuration file is incorrect.", capabilityValue.c_str());
            return false;
        }
        dataMap.at(capabilityKey) = array;
    }
    return true;
}

bool AVCodecParser::SetCapabilityData(CapabilityData &data, const std::string &capabilityKey,
                                      const std::string &capabilityValue)
{
    std::unordered_map<std::string, std::string&> capabilityStringMap = {
        {"codecName", data.codecName},
        {"mimeType", data.mimeType}
    };

    std::unordered_map<std::string, int32_t&> capabilityIntMap = {
        {"codecType", data.codecType}
    };

    std::unordered_map<std::string, bool&> capabilityBoolMap = {
        {"isVendor", data.isVendor}
    };

    std::unordered_map<std::string, Range&> capabilityRangeMap = {
        {"bitrate", data.bitrate},
        {"channels", data.channels},
        {"complexity", data.complexity},
        {"alignment", data.alignment},
        {"width", data.width},
        {"height", data.height},
        {"frameRate", data.frameRate},
        {"encodeQuality", data.encodeQuality},
        {"quality", data.quality},
    };

    std::unordered_map<std::string, std::vector<int32_t>&> capabilityVectorMap = {
        {"sampleRate", data.sampleRate},
        {"format", data.format},
        {"profiles", data.profiles},
        {"bitrateMode", data.bitrateMode},
        {"levels", data.levels},
    };

    bool ret = false;
    if (capabilityStringMap.find(capabilityKey) != capabilityStringMap.end()) {
        ret = SetCapabilityStringData(capabilityStringMap, capabilityKey, capabilityValue);
        CHECK_AND_RETURN_RET_LOG(ret != false, false, "SetCapabilityStringData failed");
    } else if (capabilityIntMap.find(capabilityKey) != capabilityIntMap.end()) {
        ret = SetCapabilityIntData(capabilityIntMap, capabilityKey, capabilityValue);
        CHECK_AND_RETURN_RET_LOG(ret != false, false, "SetCapabilityIntData failed");
    } else if (capabilityBoolMap.find(capabilityKey) != capabilityBoolMap.end()) {
        ret = SetCapabilityBoolData(capabilityBoolMap, capabilityKey, capabilityValue);
        CHECK_AND_RETURN_RET_LOG(ret != false, false, "SetCapabilityBoolData failed");
    } else if (capabilityRangeMap.find(capabilityKey) != capabilityRangeMap.end()) {
        ret = SetCapabilityRangeData(capabilityRangeMap, capabilityKey, capabilityValue);
        CHECK_AND_RETURN_RET_LOG(ret != false, false, "SetCapabilityRangeData failed");
    } else if (capabilityVectorMap.find(capabilityKey) != capabilityVectorMap.end()) {
        ret = SetCapabilityVectorData(capabilityVectorMap, capabilityKey, capabilityValue);
        CHECK_AND_RETURN_RET_LOG(ret != false, false, "SetCapabilityVectorData failed");
    } else {
        CHECK_AND_RETURN_RET_LOG(ret != false, false, "can not find capabilityKey: %{public}s", capabilityKey.c_str());
    }
    return true;
}

bool AVCodecParser::ParseData(xmlNode *node)
{
    xmlNode *child = node->xmlChildrenNode;
    std::string capabilityValue;
    CapabilityData capabilityData;
    for (; child; child = child->next) {
        if (xmlStrcmp(child->name, reinterpret_cast<const xmlChar*>("Item"))) {
            continue;
        }
        for (auto it = capabilityKeys_.begin(); it != capabilityKeys_.end(); it++) {
            std::string capabilityKey = *it;

            if (xmlHasProp(child, reinterpret_cast<xmlChar*>(const_cast<char*>(capabilityKey.c_str())))) {
                capabilityValue = std::string(reinterpret_cast<char*>(xmlGetProp(child,
                    reinterpret_cast<xmlChar*>(const_cast<char*>(capabilityKey.c_str())))));
                bool ret = SetCapabilityData(capabilityData, capabilityKey, capabilityValue);
                CHECK_AND_RETURN_RET_LOG(ret != false, false, "SetCapabilityData failed");
                break;
            }
        }
    }
    capabilityDataArray_.push_back(capabilityData);
    return true;
}

NodeName AVCodecParser::GetNodeNameAsInt(xmlNode *node)
{
    if (!xmlStrcmp(node->name, reinterpret_cast<const xmlChar*>("Codecs"))) {
        return CODECS;
    } else if (!xmlStrcmp(node->name, reinterpret_cast<const xmlChar*>("AudioCodecs"))) {
        return AUDIO_CODECS;
    } else if (!xmlStrcmp(node->name, reinterpret_cast<const xmlChar*>("VideoCodecs"))) {
        return VIDEO_CODECS;
    } else if (!xmlStrcmp(node->name, reinterpret_cast<const xmlChar*>("AudioDecoder"))) {
        return AUDIO_DECODER;
    } else if (!xmlStrcmp(node->name, reinterpret_cast<const xmlChar*>("AudioEncoder"))) {
        return AUDIO_ENCODER;
    } else if (!xmlStrcmp(node->name, reinterpret_cast<const xmlChar*>("VideoDecoder"))) {
        return VIDEO_DECODER;
    } else if (!xmlStrcmp(node->name, reinterpret_cast<const xmlChar*>("VideoEncoder"))) {
        return VIDEO_ENCODER;
    } else {
        return UNKNOWN;
    }
}

std::vector<CapabilityData> AVCodecParser::GetCapabilityDataArray()
{
    return this->capabilityDataArray_;
}
} // Media
} // OHOS