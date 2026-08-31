/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef COMMON_NAPI_PARSE_MEDIA_SURFACE_ID_H
#define COMMON_NAPI_PARSE_MEDIA_SURFACE_ID_H

#include <charconv>
#include <cstdint>
#include <string>
#include <system_error>

namespace OHOS {
namespace Media {
/*
 * Parse a whole-token decimal surface id from JS NAPI / protocol text.
 * Reject empty, overflow, leading/trailing junk, '+', hex, floats, and negatives.
 * Valid in-range values keep the same numeric result as strtoull on digit-only input.
 */
inline bool ParseMediaSurfaceId(const std::string &text, uint64_t &out)
{
    if (text.empty()) {
        return false;
    }
    const char *first = text.data();
    const char *last = first + text.size();
    uint64_t value = 0;
    auto result = std::from_chars(first, last, value);
    if (result.ec != std::errc() || result.ptr != last) {
        return false;
    }
    out = value;
    return true;
}
} // namespace Media
} // namespace OHOS

#endif // COMMON_NAPI_PARSE_MEDIA_SURFACE_ID_H
