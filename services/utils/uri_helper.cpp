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

#include "uri_helper.h"
#include <cstring>
#include <climits>
#include <sys/types.h>
#include <sys/stat.h>
#include <type_traits>
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "UriHelper"};
}

namespace OHOS {
namespace Media {
static const std::map<std::string_view, uint8_t> g_validUriTypes = {
    {"", UriHelper::UriType::URI_TYPE_FILE }, // empty uri head is treated as the file type uri.
    {"file", UriHelper::UriType::URI_TYPE_FILE},
    {"fd", UriHelper::UriType::URI_TYPE_FD},
    {"http", UriHelper::UriType::URI_TYPE_HTTP}
};

static bool PathToRealFileUrl(const std::string_view &path, std::string &realPath)
{
    if (path.empty()) {
        MEDIA_LOGE("path is empty!");
        return false;
    }

    if ((path.length() >= PATH_MAX)) {
        MEDIA_LOGE("path len is error, the len is: [%{public}zu]", path.length());
        return false;
    }

    char tmpPath[PATH_MAX] = {0};
    if (realpath(path.data(), tmpPath) == nullptr) {
        MEDIA_LOGE("path to realpath error, %{public}s", path.data());
        return false;
    }

    if (access(tmpPath, F_OK) != 0) {
        MEDIA_LOGE("check realpath (%{private}s) error", tmpPath);
        return false;
    }

    realPath = std::string("file://") + tmpPath;
    return true;
}

template<typename T, typename = std::enable_if_t<std::is_same_v<int64_t, T> || std::is_same_v<int32_t, T>>>
bool StrToInt(const std::string_view& str, T& value)
{
    if (str.empty() || (!isdigit(str.front()) && (str.front() != '-'))) {
        return false;
    }

    std::string valStr(str);
    char* end = nullptr;
    errno = 0;
    const char* addr = valStr.c_str();
    long long result = strtoll(addr, &end, 10); /* 10 means decimal */
    if ((end == addr) || (end[0] != '\0') || (errno == ERANGE) ||
            (result > LLONG_MAX) || (result < LLONG_MIN)) {
        MEDIA_LOGE("call StrToInt func false,  input str is: %{public}s!", valStr.c_str());
        return false;
    }

    if constexpr (std::is_same<int32_t, T>::value) {
        if ((result > INT_MAX) || (result < INT_MIN)) {
            MEDIA_LOGE("call StrToInt func false,  input str is: %{public}s!", valStr.c_str());
            return false;
        }
        value = static_cast<int32_t>(result);
        return true;
    }

    value = result;
    return true;
}

std::pair<std::string_view, std::string_view> SplitUriHeadAndBody(const std::string_view &str)
{
    std::string_view::size_type start = str.find_first_not_of(' ');
    std::string_view::size_type end = str.find_last_not_of(' ');
    std::pair<std::string_view, std::string_view> result;
    std::string_view noSpaceStr;

    if (end == std::string_view::npos) {
        noSpaceStr = str.substr(start);
    } else {
        noSpaceStr = str.substr(start, end - start + 1);
    }

    std::string_view delimiter = "://";
    std::string_view::size_type pos = noSpaceStr.find(delimiter);
    if (pos == std::string_view::npos) {
        result.first = "";
        result.second = noSpaceStr;
    } else {
        result.first = noSpaceStr.substr(0, pos);
        result.second = noSpaceStr.substr(pos + delimiter.size());
    }

    return result;
}

void UriHelper::Swap(UriHelper &&rhs) noexcept
{
    formattedUri_.swap(rhs.formattedUri_);
    type_ = rhs.type_;
    rawFileUri_ = rhs.rawFileUri_;
    std::swap(fd_, rhs.fd_);
    offset_ = rhs.offset_;
    size_ = rhs.size_;
}

void UriHelper::Copy(const UriHelper &rhs) noexcept
{
    formattedUri_ = rhs.formattedUri_;
    type_ = rhs.type_;
    if (type_ == UriHelper::URI_TYPE_FILE) {
        rawFileUri_ = formattedUri_;
        rawFileUri_ = rawFileUri_.substr(strlen("file://"));
    }
    if (rhs.fd_ > 0) {
        fd_ = ::dup(rhs.fd_);
    }
    offset_ = rhs.offset_;
    size_ = rhs.size_;
}

UriHelper::UriHelper(const std::string_view &uri)
{
    FormatMeForUri(uri);
}

UriHelper::UriHelper(int32_t fd, int64_t offset, int64_t size) : fd_(fd), offset_(offset), size_(size)
{
    FormatMeForFd();
}

UriHelper::~UriHelper()
{
    if (fd_ > 0) {
        (void)::close(fd_);
    }
}

UriHelper::UriHelper(const UriHelper &rhs)
{
    Copy(rhs);
}

UriHelper &UriHelper::operator=(const UriHelper &rhs)
{
    if (&rhs == this) {
        return *this;
    }

    Copy(rhs);
    return *this;
}

UriHelper::UriHelper(UriHelper &&rhs) noexcept
{
    Swap(std::forward<UriHelper &&>(rhs));
}

UriHelper &UriHelper::operator=(UriHelper &&rhs) noexcept
{
    if (&rhs == this) {
        return *this;
    }

    Swap(std::forward<UriHelper &&>(rhs));
    return *this;
}

void UriHelper::FormatMeForUri(const std::string_view &uri) noexcept
{
    if (!formattedUri_.empty()) {
        return;
    }

    auto [head, body] = SplitUriHeadAndBody(uri);
    if (g_validUriTypes.count(head) == 0) {
        return;
    }
    type_ = g_validUriTypes.at(head);

    // verify whether the uri is readable and generate the formatted uri.
    switch (type_) {
        case URI_TYPE_FILE: {
            if (!PathToRealFileUrl(body, formattedUri_)) {
                type_ = URI_TYPE_UNKNOWN;
                formattedUri_ = body;
            }
            rawFileUri_ = formattedUri_;
            rawFileUri_ = rawFileUri_.substr(strlen("file://"));
            break;
        }
        case URI_TYPE_FD: {
            if (!ParseFdUri(body)) {
                type_ = URI_TYPE_UNKNOWN;
                formattedUri_ = "";
            }
            break;
        }
        default:
            formattedUri_ = std::string(head);
            formattedUri_ += body;
            break;
    }

    MEDIA_LOGI("formatted uri: %{public}s", formattedUri_.c_str());
}

void UriHelper::FormatMeForFd() noexcept
{
    if (!formattedUri_.empty()) {
        return;
    }

    type_ = URI_TYPE_FD;
    (void)CorrectFdParam();
}

bool UriHelper::CorrectFdParam()
{
    int flags = fcntl(fd_, F_GETFL);
    CHECK_AND_RETURN_RET_LOG(flags != -1, false, "Fail to get File Status Flags");

    struct stat64 st;
    if (fstat64(fd_, &st) != 0) {
        MEDIA_LOGE("can not get file state");
        return false;
    }

    int64_t fdSize = static_cast<int64_t>(st.st_size);
    if (offset_ < 0 || offset_ > fdSize) {
        offset_ = 0;
    }

    if ((size_ <= 0) || (size_ > fdSize - offset_)) {
        size_ = fdSize - offset_;
    }

    fd_ = ::dup(fd_);
    formattedUri_ = std::string("fd://") + std::to_string(fd_) + "?offset=" +
        std::to_string(offset_) + "&size=" + std::to_string(size_);
    return true;
}

bool UriHelper::ParseFdUri(int32_t &fd, int64_t &offset, int64_t size)
{
    if (type_ != URI_TYPE_FD) {
        return false;
    }

    fd = fd_;
    offset = offset_;
    size = size_;
    return true;
}

uint8_t UriHelper::UriType() const
{
    return type_;
}

std::string UriHelper::FormattedUri() const
{
    return formattedUri_;
}

bool UriHelper::AccessCheck(uint8_t flag) const
{
    if (type_ == URI_TYPE_UNKNOWN) {
        return false;
    }

    if (type_ == URI_TYPE_FILE) {
        uint32_t mode = (flag & URI_READ) ? R_OK : 0;
        mode |= (flag & URI_WRITE) ? W_OK : 0;
        int ret = access(rawFileUri_.data(), static_cast<int>(mode));
        if (ret != 0) {
            MEDIA_LOGE("Fail to access path: %{public}s", rawFileUri_.data());
            return false;
        }
        return true;
    } else if (type_ == URI_TYPE_FD) {
        CHECK_AND_RETURN_RET_LOG(fd_ > 0, false, "Fail to get file descriptor from uri");

        int flags = fcntl(fd_, F_GETFL);
        CHECK_AND_RETURN_RET_LOG(flags != -1, false, "Fail to get File Status Flags");

        uint32_t mode = (flag & URI_WRITE) ? O_RDWR : O_RDONLY;
        if ((static_cast<unsigned int>(flags) & mode) != mode) {
            return false;
        }
        return true;
    }

    return true; // Not implemented, defaultly return true.
}

bool UriHelper::ParseFdUri(std::string_view uri)
{
    static constexpr std::string_view::size_type delim1Len = std::string_view("?offset=").size();
    static constexpr std::string_view::size_type delim2Len = std::string_view("&size=").size();
    std::string_view::size_type delim1 = uri.find("?");
    std::string_view::size_type delim2 = uri.find("&");

    if (delim1 == std::string_view::npos && delim1 == std::string_view::npos) {
        CHECK_AND_RETURN_RET_LOG(StrToInt(uri, fd_), false, "Invalid fd url");
    } else if (delim1 != std::string_view::npos && delim2 != std::string_view::npos) {
        std::string_view fdstr = uri.substr(0, delim1);
        int32_t fd = -1;
        CHECK_AND_RETURN_RET_LOG(StrToInt(fdstr, fd), false, "Invalid fd url");
        std::string_view offsetStr = uri.substr(delim1 + delim1Len, delim2 - delim1 - delim1Len);
        CHECK_AND_RETURN_RET_LOG(StrToInt(offsetStr, offset_), false, "Invalid fd url");
        std::string_view sizeStr = uri.substr(delim2 + delim2Len);
        CHECK_AND_RETURN_RET_LOG(StrToInt(sizeStr, size_), false, "Invalid fd url");
        fd_ = fd;
    } else {
        MEDIA_LOGE("invalid fd uri: %{public}s", uri.data());
        return false;
    }

    MEDIA_LOGD("parse fd uri, fd: %{public}d, offset: %{public}" PRIi64 ", size: %{public}" PRIi64,
               fd_, offset_, size_);

    return CorrectFdParam();
}
} // namespace Media
} // namespace OHOS
