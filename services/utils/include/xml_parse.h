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

#ifndef XML_PARSE_H
#define XML_PARSE_H

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <vector>
#include <string>
#include <unordered_map>
#include "avcodec_info.h"

namespace OHOS {
namespace Media {
class __attribute__((visibility("default"))) XmlParser {
public:
    virtual ~XmlParser();
    virtual bool LoadConfiguration(const char *xmlPath) final;
    virtual bool Parse() final;
    virtual void Destroy() final;

protected:
    xmlDoc *mDoc_ = nullptr;
    virtual bool ParseInternal(xmlNode *node) = 0;
    virtual bool SetCapabilityIntData(std::unordered_map<std::string, int32_t&> dataMap,
        const std::string &capabilityKey, const std::string &capabilityValue) const = 0;
    virtual bool SetCapabilityVectorData(std::unordered_map<std::string, std::vector<int32_t>&> dataMap,
        const std::string &capabilityKey, const std::string &capabilityValue) const = 0;
    virtual bool IsNumberArray(const std::vector<std::string> &strArray) const final;
    virtual bool TransStrAsRange(const std::string &str, Range &range) const final;
    virtual std::vector<int32_t> TransMapAsIntegerArray(
        const std::unordered_map<std::string, int> &capabilityMap, const std::vector<std::string> &spilt) const final;
    virtual std::vector<int32_t> TransStrAsIntegerArray(const std::vector<std::string> &spilt) const final;
    virtual bool SpiltKeyList(
        const std::string &str, const std::string &delim, std::vector<std::string> &spilt) const final;
    virtual bool SetCapabilityStringData(std::unordered_map<std::string, std::string&> dataMap,
        const std::string &capabilityKey, const std::string &capabilityValue) const final;
    virtual bool SetCapabilityBoolData(std::unordered_map<std::string, bool&> dataMap,
        const std::string &capabilityKey, const std::string &capabilityValue) const final;
    virtual bool SetCapabilityRangeData(std::unordered_map<std::string, Range&> dataMap,
        const std::string &capabilityKey, const std::string &capabilityValue) const final;
};
}  //  namespace Media
}  //  namespace OHOS

#endif  //  XML_PARSE_H