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

#include "format.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "Format"};
}

namespace OHOS {
namespace Media {
bool Format::PutIntValue(const std::string &key, int32_t value)
{
    FormatData data;
    data.type = FORMAT_TYPE_INT32;
    data.val.int32Val = value;
    auto ret = formatMap_.insert(std::make_pair(key, data));
    return ret.second;
}

bool Format::PutLongValue(const std::string &key, int64_t value)
{
    FormatData data;
    data.type = FORMAT_TYPE_INT64;
    data.val.int64Val = value;
    auto ret = formatMap_.insert(std::make_pair(key, data));
    return ret.second;
}

bool Format::PutFloatValue(const std::string &key, float value)
{
    FormatData data;
    data.type = FORMAT_TYPE_FLOAT;
    data.val.floatVal = value;
    auto ret = formatMap_.insert(std::make_pair(key, data));
    return ret.second;
}

bool Format::PutDoubleValue(const std::string &key, double value)
{
    FormatData data;
    data.type = FORMAT_TYPE_DOUBLE;
    data.val.doubleVal = value;
    auto ret = formatMap_.insert(std::make_pair(key, data));
    return ret.second;
}

bool Format::PutStringValue(const std::string &key, const std::string &value)
{
    FormatData data;
    data.type = FORMAT_TYPE_STRING;
    data.stringVal = value;
    auto ret = formatMap_.insert(std::make_pair(key, data));
    return ret.second;
}

bool Format::GetStringValue(const std::string &key, std::string &value) const
{
    auto iter = formatMap_.find(key);
    if (iter == formatMap_.end() || iter->second.type != FORMAT_TYPE_STRING) {
        MEDIA_LOGE("Format::GetFormat failed. Key: %{public}s", key.c_str());
        return false;
    }
    value = iter->second.stringVal;
    return true;
}

bool Format::GetIntValue(const std::string &key, int32_t &value) const
{
    auto iter = formatMap_.find(key);
    if (iter == formatMap_.end() || iter->second.type != FORMAT_TYPE_INT32) {
        MEDIA_LOGE("Format::GetFormat failed. Key: %{public}s", key.c_str());
        return false;
    }
    value = iter->second.val.int32Val;
    return true;
}

bool Format::GetLongValue(const std::string &key, int64_t &value) const
{
    auto iter = formatMap_.find(key);
    if (iter == formatMap_.end() || iter->second.type != FORMAT_TYPE_INT64) {
        MEDIA_LOGE("Format::GetFormat failed. Key: %{public}s", key.c_str());
        return false;
    }
    value = iter->second.val.int64Val;
    return true;
}

bool Format::GetFloatValue(const std::string &key, float &value) const
{
    auto iter = formatMap_.find(key);
    if (iter == formatMap_.end() || iter->second.type != FORMAT_TYPE_FLOAT) {
        MEDIA_LOGE("Format::GetFormat failed. Key: %{public}s", key.c_str());
        return false;
    }
    value = iter->second.val.floatVal;
    return true;
}

bool Format::GetDoubleValue(const std::string &key, double &value) const
{
    auto iter = formatMap_.find(key);
    if (iter == formatMap_.end() || iter->second.type != FORMAT_TYPE_DOUBLE) {
        MEDIA_LOGE("Format::GetFormat failed. Key: %{public}s", key.c_str());
        return false;
    }
    value = iter->second.val.doubleVal;
    return true;
}

const std::map<std::string, FormatData> &Format::GetFormatMap() const
{
    return formatMap_;
}
}
}