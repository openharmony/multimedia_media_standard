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

#include "avformat_native_mock.h"

namespace OHOS {
namespace Media {
bool AVFormatNativeMock::PutIntValue(const std::string_view &key, int32_t value)
{
    return format_.PutIntValue(key, value);
}

bool AVFormatNativeMock::GetIntValue(const std::string_view &key, int32_t &value)
{
    return format_.GetIntValue(key, value);
}

bool PutStringValue(const std::string_view &key, const std::string_view &value)
{
    return format_.PutStringValue(key, value);
}

bool GetStringValue(const std::string_view &key, std::string &value)
{
    return format_.GetStringValue(key, value);
}

Format &AVFormatNativeMock::GetFormat()
{
    return format_;
}
} // Media
} // OHOS