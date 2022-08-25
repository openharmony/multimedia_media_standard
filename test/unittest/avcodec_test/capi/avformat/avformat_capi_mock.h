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

#ifndef AVFORMAT_CAPI_MOCK_H
#define AVFORMAT_CAPI_MOCK_H

#include "avcodec_mock.h"
#include "native_avformat.h"

namespace OHOS {
namespace Media {
class AVFormatCapiMock : public FormatMock {
public:
    explicit AVFormatCapiMock(OH_AVFormat *format) : format_(format) {}
    AVFormatCapiMock();
    ~AVFormatCapiMock();
    bool PutIntValue(const std::string_view &key, int32_t value) override;
    bool GetIntValue(const std::string_view &key, int32_t &value) override;
    bool PutStringValue(const std::string_view &key, const std::string_view &value) override;
    bool GetStringValue(const std::string_view &key, std::string &value) override;
    void Destroy() override;
    OH_AVFormat *GetFormat();

private:
    OH_AVFormat *format_;
};
} // Media
} // OHOS
#endif // AVFORMAT_CAPI_MOCK_H