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
#include "securec.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "Format"};
}

namespace OHOS {
namespace Media {
void CopyFormatDataMap(const Format::FormatDataMap &from, Format::FormatDataMap &to)
{
    for (auto it = to.begin(); it != to.end(); ++it) {
        if (it->second.type == FORMAT_TYPE_ADDR && it->second.addr != nullptr) {
            free(it->second.addr);
            it->second.addr = nullptr;
        }
    }

    to = from;

    for (auto it = to.begin(); it != to.end();) {
        if (it->second.type != FORMAT_TYPE_ADDR || it->second.addr == nullptr) {
            ++it;
            continue;
        }

        it->second.addr = reinterpret_cast<uint8_t *>(malloc(it->second.size));
        if (it->second.addr == nullptr) {
            MEDIA_LOGE("malloc addr failed. Key: %{public}s", it->first.c_str());
            it = to.erase(it);
            continue;
        }

        errno_t err = memcpy_s(reinterpret_cast<void *>(it->second.addr),
            it->second.size, reinterpret_cast<const void *>(from.at(it->first).addr), it->second.size);
        if (err != EOK) {
            MEDIA_LOGE("memcpy addr failed. Key: %{public}s", it->first.c_str());
            free(it->second.addr);
            it->second.addr = nullptr;
            it = to.erase(it);
            continue;
        }
        ++it;
    }
}

Format::~Format()
{
    for (auto it = formatMap_.begin(); it != formatMap_.end(); ++it) {
        if (it->second.type == FORMAT_TYPE_ADDR && it->second.addr != nullptr) {
            free(it->second.addr);
            it->second.addr = nullptr;
        }
    }
}

Format::Format(const Format &rhs)
{
    if (&rhs == this) {
        return;
    }

    CopyFormatDataMap(rhs.formatMap_, formatMap_);
}

Format::Format(Format &&rhs) noexcept
{
    std::swap(formatMap_, rhs.formatMap_);
}

Format &Format::operator=(const Format &rhs)
{
    if (&rhs == this) {
        return *this;
    }

    CopyFormatDataMap(rhs.formatMap_, this->formatMap_);
    return *this;
}

Format &Format::operator=(Format &&rhs) noexcept
{
    if (&rhs == this) {
        return *this;
    }

    std::swap(this->formatMap_, rhs.formatMap_);
    return *this;
}

bool Format::PutIntValue(const std::string_view &key, int32_t value)
{
    FormatData data;
    data.type = FORMAT_TYPE_INT32;
    data.val.int32Val = value;
    auto ret = formatMap_.insert(std::make_pair(key, data));
    return ret.second;
}

bool Format::PutLongValue(const std::string_view &key, int64_t value)
{
    FormatData data;
    data.type = FORMAT_TYPE_INT64;
    data.val.int64Val = value;
    auto ret = formatMap_.insert(std::make_pair(key, data));
    return ret.second;
}

bool Format::PutFloatValue(const std::string_view &key, float value)
{
    FormatData data;
    data.type = FORMAT_TYPE_FLOAT;
    data.val.floatVal = value;
    auto ret = formatMap_.insert(std::make_pair(key, data));
    return ret.second;
}

bool Format::PutDoubleValue(const std::string_view &key, double value)
{
    FormatData data;
    data.type = FORMAT_TYPE_DOUBLE;
    data.val.doubleVal = value;
    auto ret = formatMap_.insert(std::make_pair(key, data));
    return ret.second;
}

bool Format::PutStringValue(const std::string_view &key, const std::string_view &value)
{
    FormatData data;
    data.type = FORMAT_TYPE_STRING;
    data.stringVal = value;
    auto ret = formatMap_.insert(std::make_pair(key, data));
    return ret.second;
}

bool Format::GetStringValue(const std::string_view &key, std::string &value) const
{
    auto iter = formatMap_.find(key);
    if (iter == formatMap_.end() || iter->second.type != FORMAT_TYPE_STRING) {
        MEDIA_LOGE("Format::GetFormat failed. Key: %{public}s", key.data());
        return false;
    }
    value = iter->second.stringVal;
    return true;
}

bool Format::GetIntValue(const std::string_view &key, int32_t &value) const
{
    auto iter = formatMap_.find(key);
    if (iter == formatMap_.end() || iter->second.type != FORMAT_TYPE_INT32) {
        MEDIA_LOGE("Format::GetFormat failed. Key: %{public}s", key.data());
        return false;
    }
    value = iter->second.val.int32Val;
    return true;
}

bool Format::GetLongValue(const std::string_view &key, int64_t &value) const
{
    auto iter = formatMap_.find(key);
    if (iter == formatMap_.end() || iter->second.type != FORMAT_TYPE_INT64) {
        MEDIA_LOGE("Format::GetFormat failed. Key: %{public}s", key.data());
        return false;
    }
    value = iter->second.val.int64Val;
    return true;
}

bool Format::GetFloatValue(const std::string_view &key, float &value) const
{
    auto iter = formatMap_.find(key);
    if (iter == formatMap_.end() || iter->second.type != FORMAT_TYPE_FLOAT) {
        MEDIA_LOGE("Format::GetFormat failed. Key: %{public}s", key.data());
        return false;
    }
    value = iter->second.val.floatVal;
    return true;
}

bool Format::GetDoubleValue(const std::string_view &key, double &value) const
{
    auto iter = formatMap_.find(key);
    if (iter == formatMap_.end() || iter->second.type != FORMAT_TYPE_DOUBLE) {
        MEDIA_LOGE("Format::GetFormat failed. Key: %{public}s", key.data());
        return false;
    }
    value = iter->second.val.doubleVal;
    return true;
}

bool Format::PutBuffer(const std::string_view &key, const uint8_t *addr, size_t size)
{
    constexpr size_t sizeMax = 1 * 1024 * 1024;
    if (size > sizeMax) {
        MEDIA_LOGE("PutBuffer input size failed. Key: %{public}s", key.data());
        return false;
    }

    FormatData data;
    data.type = FORMAT_TYPE_ADDR;
    data.addr = reinterpret_cast<uint8_t *>(malloc(size));
    if (data.addr == nullptr) {
        MEDIA_LOGE("malloc addr failed. Key: %{public}s", key.data());
        return false;
    }

    errno_t err = memcpy_s(reinterpret_cast<void *>(data.addr), size, reinterpret_cast<const void *>(addr), size);
    if (err != EOK) {
        MEDIA_LOGE("PutBuffer memcpy addr failed. Key: %{public}s", key.data());
        free(data.addr);
        return false;
    }

    RemoveKey(key);

    data.size = size;
    auto ret = formatMap_.insert(std::make_pair(key, data));
    return ret.second;
}

bool Format::GetBuffer(const std::string_view &key, uint8_t **addr, size_t &size) const
{
    auto iter = formatMap_.find(key);
    if (iter == formatMap_.end() || iter->second.type != FORMAT_TYPE_ADDR) {
        MEDIA_LOGE("Format::GetBuffer failed. Key: %{public}s", key.data());
        return false;
    }
    *addr = iter->second.addr;
    size = iter->second.size;
    return true;
}

bool Format::ContainKey(const std::string_view &key) const
{
    auto iter = formatMap_.find(key);
    if (iter != formatMap_.end()) {
        return true;
    }

    return false;
}

FormatDataType Format::GetValueType(const std::string_view &key) const
{
    auto iter = formatMap_.find(key);
    if (iter == formatMap_.end()) {
        return FORMAT_TYPE_NONE;
    }

    return iter->second.type;
}

void Format::RemoveKey(const std::string_view &key)
{
    auto iter = formatMap_.find(key);
    if (iter != formatMap_.end()) {
        if (iter->second.type == FORMAT_TYPE_ADDR && iter->second.addr != nullptr) {
            free(iter->second.addr);
            iter->second.addr = nullptr;
        }
        formatMap_.erase(iter);
    }
}

const Format::FormatDataMap &Format::GetFormatMap() const
{
    return formatMap_;
}
} // namespace Media
} // namespace OHOS