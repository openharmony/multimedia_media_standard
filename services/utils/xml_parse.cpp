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

#include "xml_parse.h"
#include "media_errors.h"
#include "media_log.h"
#include "string_ex.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "XmlParser"};
}

namespace OHOS {
namespace Media {
XmlParser::~XmlParser()
{
    Destroy();
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

bool XmlParser::LoadConfiguration(const char *xmlPath)
{
    mDoc_ = xmlReadFile(xmlPath, NULL, 0);
    if (mDoc_ == NULL) {
        MEDIA_LOGE("XmlParser xmlReadFile failed");
        return false;
    }
    return true;
}

bool XmlParser::Parse()
{
    xmlNode *root = xmlDocGetRootElement(mDoc_);
    if (root == NULL) {
        MEDIA_LOGE("XmlParser xmlDocGetRootElement failed");
        return false;
    }
    return ParseInternal(root);
}

void XmlParser::Destroy()
{
    if (mDoc_ != NULL) {
        xmlFreeDoc(mDoc_);
    }
    return;
}

bool XmlParser::IsNumberArray(const std::vector<std::string> &strArray) const
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

bool XmlParser::TransStrAsRange(const std::string &str, Range &range) const
{
    if (str == "null" || str == "") {
        MEDIA_LOGD("str is null");
        return false;
    }
    size_t pos = str.find("-");
    if (pos != str.npos && pos + 1 < str.size()) {
        std::string head = str.substr(0, pos);
        std::string tail = str.substr(pos + 1);
        if (!StrToInt(head, range.minVal)) {
            MEDIA_LOGE("call StrToInt func false, input str is: %{public}s", head.c_str());
            return false;
        }
        if (!StrToInt(tail, range.maxVal)) {
            MEDIA_LOGE("call StrToInt func false, input str is: %{public}s", tail.c_str());
            return false;
        }
    } else {
        MEDIA_LOGD("Can not find the delimiter of \"-\" in : %{public}s", str.c_str());
        return false;
    }
    return true;
}

std::vector<int32_t> XmlParser::TransMapAsIntegerArray(
    const std::unordered_map<std::string, int> &capabilityMap,
    const std::vector<std::string> &spilt) const
{
    std::vector<int32_t> res;
    for (auto iter = spilt.begin(); iter != spilt.end(); iter++) {
        if (capabilityMap.find(*iter) != capabilityMap.end()) {
            res.emplace_back(capabilityMap.at(*iter));
        } else {
            MEDIA_LOGD("can not find %{public}s in capabilityMap", iter->c_str());
        }
    }
    return res;
}

std::vector<int32_t> XmlParser::TransStrAsIntegerArray(const std::vector<std::string> &spilt) const
{
    std::vector<int32_t> array;
    for (auto iter = spilt.begin(); iter != spilt.end(); iter++) {
        int32_t num = -1;
        if (!StrToInt(*iter, num)) {
            MEDIA_LOGE("call StrToInt func false, input str is: %{public}s", iter->c_str());
            return array;
        }
        array.push_back(num);
    }
    return array;
}

bool XmlParser::SpiltKeyList(const std::string &str, const std::string &delim,
    std::vector<std::string> &spilt) const
{
    if (str == "") {
        return false;
    }
    std::string strAddDelim = str;
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

bool XmlParser::SetCapabilityStringData(std::unordered_map<std::string, std::string&> dataMap,
    const std::string &capabilityKey, const std::string &capabilityValue) const
{
    dataMap.at(capabilityKey) = capabilityValue;
    return true;
}

bool XmlParser::SetCapabilityBoolData(std::unordered_map<std::string, bool&> dataMap,
    const std::string &capabilityKey, const std::string &capabilityValue) const
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

bool XmlParser::SetCapabilityRangeData(std::unordered_map<std::string, Range&> dataMap,
    const std::string &capabilityKey, const std::string &capabilityValue) const
{
    Range range;
    bool ret = TransStrAsRange(capabilityValue, range);
    CHECK_AND_RETURN_RET_LOG(ret != false, false, "failed:can not trans %{public}s", capabilityValue.c_str());
    dataMap.at(capabilityKey) = range;
    return true;
}
}  // namespace Media
}  // namespace OHOS