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

#ifndef AVCODEC_XML_PARSER_H
#define AVCODEC_XML_PARSER_H

#include <vector>
#include <string>
#include <unordered_map>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "avcodec_info.h"

namespace OHOS {
namespace Media {
enum NodeName : int32_t {
    CODECS,
    AUDIO_CODECS,
    VIDEO_CODECS,
    AUDIO_DECODER,
    AUDIO_ENCODER,
    VIDEO_DECODER,
    VIDEO_ENCODER,
    UNKNOWN
};

class AVCodecXmlParser {
public:
    const char *AVCODEC_CONFIG_FILE = "/etc/codec/codec_caps.xml";
    AVCodecXmlParser();
    ~AVCodecXmlParser();
    bool LoadConfiguration();
    bool Parse();
    void Destroy();
    std::vector<CapabilityData> GetCapabilityDataArray();

private:
    bool IsNumberArray(const std::vector<std::string> &strArray);
    bool TransStrAsRange(const std::string &str, Range &range);
    bool TransStrAsSize(const std::string &str, ImgSize &size);
    std::vector<int32_t> TransMapAsIntegerArray(const std::unordered_map<std::string, int> &capabilityMap,
                                                std::vector<std::string> &spilt);
    std::vector<int32_t> TransStrAsIntegerArray(std::vector<std::string> &spilt);
    bool SpiltKeyList(const std::string &str, const std::string &delim, std::vector<std::string> &spilt);
    bool SetCapabilityStringData(std::unordered_map<std::string, std::string&> dataMap,
                                const std::string &capabilityKey, const std::string &capabilityValue);
    bool SetCapabilityIntData(std::unordered_map<std::string, int32_t&> dataMap,
                                const std::string &capabilityKey, const std::string &capabilityValue);
    bool SetCapabilityBoolData(std::unordered_map<std::string, bool&> dataMap,
                                const std::string &capabilityKey, const std::string &capabilityValue);
    bool SetCapabilityRangeData(std::unordered_map<std::string, Range&> dataMap,
                                const std::string &capabilityKey, const std::string &capabilityValue);
    bool SetCapabilityVectorData(std::unordered_map<std::string, std::vector<int32_t>&> dataMap,
                                const std::string &capabilityKey, const std::string &capabilityValue);
    bool SetCapabilityData(CapabilityData &data, const std::string &capabilityKey, const std::string &capabilityValue);
    bool SetCapabilitySizeData(std::unordered_map<std::string, ImgSize&> dataMap,
                                const std::string &capabilityKey, const std::string &capabilityValue);
    bool SetCapabilityHashRangeData(std::unordered_map<std::string, std::map<ImgSize, Range>&> dataMap,
                                                const std::string &capabilityKey, const std::string &capabilityValue);

    bool ParseInternal(xmlNode *node);
    NodeName GetNodeNameAsInt(xmlNode *node);
    bool ParseData(xmlNode *node);
    std::vector<CapabilityData> capabilityDataArray_;
    xmlDoc *mDoc_;
    std::string capabilityListVal_;
    std::vector<std::string> capabilityKeys_ = {
        "codecName",
        "codecType",
        "mimeType",
        "isVendor",
        "bitrate",
        "channels",
        "sampleRate",
        "format",
        "profiles",
        "complexity",
        "bitrateMode",
        "alignment",
        "width",
        "height",
        "frameRate",
        "encodeQuality",
        "quality",
        "levels",
        "blockPerFrame",
        "blockPerSecond",
        "blockSize",
        "profileLevelsMap",
        "measuredFrameRate",
        };
};
} // Medias
} // OHOS
#endif // AVCODEC_XML_PARSER_H
