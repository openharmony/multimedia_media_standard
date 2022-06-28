/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef TEST_PARAM_COMMON_H
#define TEST_PARAM_COMMON_H

#include <cstdint>
#include "string"

namespace OHOS {
namespace Media {
namespace PlayerTestParam {
const static int32_t SEEK_TIME_5_SEC = 5000;
const static int32_t SEEK_TIME_2_SEC = 2000;
const static int32_t WAITSECOND = 6;
const static int32_t DELTA_TIME = 1000;
const std::string MEDIA_ROOT = "file://data/media/";
const std::string VIDEO_FILE1 = MEDIA_ROOT + "test_1920_1080_1.mp4";
} // namespace PlayerTestParam
} // namespace Media
} // namespace OHOS

#endif