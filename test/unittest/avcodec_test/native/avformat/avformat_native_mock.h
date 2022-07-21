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

#ifndef AVFORMAT_NATIVE_MOCK_H
#define AVFORMAT_NATIVE_MOCK_H

#include "avcodec_mock.h"
#include "format.h"

namespace OHOS {
namespace Media {
class AVFormatNativeMock : public FormatMock {
public:
    explicit AVFormatNativeMock(const Format &format) : format_(format) {}
    AVFormatNativeMock() = default;
    bool PutIntValue(const std::string_view &key, int32_t value) override;
    bool GetIntValue(const std::string_view &key, int32_t &value) override;
    bool PutStringValue(const std::string_view &key, const std::string_view &value) override;
    bool GetStringValue(const std::string_view &key, std::string &value) override;
    Format &GetFormat();

private:
    Format format_;
};
} // Media
} // OHOS
#endif // AVFORMAT_NATIVE_MOCK_H