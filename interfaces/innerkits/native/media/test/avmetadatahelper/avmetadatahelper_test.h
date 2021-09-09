
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

#ifndef AVMETADATAHELPER_TEST_H
#define AVMETADATAHELPER_TEST_H

#include "avmetadatahelper.h"

namespace OHOS {
namespace Media {
class AVMetadataHelperTest {
public:
    AVMetadataHelperTest() = default;
    ~AVMetadataHelperTest() = default;
    void TestCase(const std::string &pathOuter);

private:
    void GetMetadata(const std::string cmd);
    void DoNext();
    std::shared_ptr<AVMetadataHelper> avMetadataHelper_;
};
}
}
#endif