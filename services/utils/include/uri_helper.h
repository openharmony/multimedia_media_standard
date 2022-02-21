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

#ifndef URI_HELPER_H
#define URI_HELPER_H

#include <cstdint>
#include <string>
#include <string_view>
#include <map>
#include <unistd.h>
#include <fcntl.h>

namespace OHOS {
namespace Media {
/**
 * The simple utility is designed to facilitate the uri processing.
 *
 * Multi-thread Unsafe and Only Stack Local Avaiable
 */
class __attribute__((visibility("default"))) UriHelper {
public:
    enum UriType : uint8_t {
        URI_TYPE_FILE,
        URI_TYPE_FD,
        URI_TYPE_HTTP,
        URI_TYPE_UNKNOWN,
    };

    enum UriAccessMode : uint8_t {
        URI_READ = 1 << 0,
        URI_WRITE = 1 << 1,
    };

    UriHelper(std::string_view uri) : uri_(uri) {}
    ~UriHelper() = default;

    UriHelper(UriHelper &&rhs) noexcept;
    UriHelper &operator=(UriHelper &&rhs) noexcept;
    UriHelper(const UriHelper &rhs) = default;
    UriHelper &operator=(const UriHelper &rhs) = default;

    UriHelper &FormatMe();
    uint8_t UriType() const;
    std::string FormattedUri() const;
    bool AccessCheck(uint8_t flag) const;
    static std::string FormatFdToUri(int32_t fd, int64_t offset, int64_t size);

private:
    int GetFdFromUri(std::string rawUri) const;

    std::string_view uri_;
    std::string formattedUri_ = "";
    uint8_t type_ = 0;
};
}
}

#endif