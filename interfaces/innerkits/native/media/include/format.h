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

#ifndef FORMAT_H
#define FORMAT_H

#include <map>
#include <string>

namespace OHOS {
namespace Media {
enum FormatDataType : uint32_t {
    /** None */
    FORMAT_TYPE_NONE,
    /** Int8 */
    FORMAT_TYPE_INT8,
    /** Int16 */
    FORMAT_TYPE_INT16,
    /** Int32 */
    FORMAT_TYPE_INT32,
    /** Int64 */
    FORMAT_TYPE_INT64,
    /** UInt8 */
    FORMAT_TYPE_UINT8,
    /** UInt16 */
    FORMAT_TYPE_UINT16,
    /** UInt32 */
    FORMAT_TYPE_UINT32,
    /** UInt64 */
    FORMAT_TYPE_UINT64,
    /** Float */
    FORMAT_TYPE_FLOAT,
    /** Double */
    FORMAT_TYPE_DOUBLE,
    /** String */
    FORMAT_TYPE_STRING
};

class FormatData {
public:
    FormatData() = default;
    ~FormatData() = default;
};

class Format {
public:
    Format() = default;
    ~Format() = default;

    const std::map<std::string, FormatData *> &GetFormatMap() const;

private:
    // string: such as video_width
    // FormatData: such as int32_t 1080
    std::map<std::string, FormatData *> formatMap_;
};
} // namespace Media
} // namespace OHOS

#endif // FORMAT_H