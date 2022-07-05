/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "test_params_config.h"
#include <iomanip>
#include <sstream>
#include "unittest_log.h"
#include "securec.h"
namespace OHOS {
namespace Media {
namespace PlayerTestParam {
} // namespace PlayerTestParam
namespace AVMetadataTestParam {
AVMetadataTestBase::AVMetadataTestBase()
{
}

AVMetadataTestBase::~AVMetadataTestBase()
{
}

bool AVMetadataTestBase::StrToInt64(const std::string &str, int64_t &value)
{
    if (str.empty() || (!isdigit(str.front()) && (str.front() != '-'))) {
        return false;
    }

    char *end = nullptr;
    errno = 0;
    auto addr = str.c_str();
    auto result = strtoll(addr, &end, 10); /* 10 means decimal */
    if (result == 0) {
        return false;
    }
    if ((end == addr) || (end[0] != '\0') || (errno == ERANGE)) {
        UNITTEST_INFO_LOG("call StrToInt func false,  input str is: %s!", str.c_str());
        return false;
    }

    value = result;
    return true;
}

bool AVMetadataTestBase::CompareMetadata(int32_t key, const std::string &result, const std::string &expected)
{
    std::string keyStr = (AVMETA_KEY_TO_STRING_MAP.count(key) == 0) ?
        std::string(AVMETA_KEY_TO_STRING_MAP.at(key)) : std::to_string(key);

    do {
        if (key == AV_KEY_DURATION) {
            int64_t resultDuration = 0;
            int64_t expectedDuration = 0;
            if (result.compare(expected) == 0) {
                return true;
            }
            if (!StrToInt64(result, resultDuration) || !StrToInt64(expected, expectedDuration)) {
                break;
            }
            if (std::abs(resultDuration - expectedDuration) > 100) { // max allowed time margin is 100ms
                break;
            }
        } else {
            if (result.compare(expected) != 0) {
                break;
            }
        }
        return true;
    } while (0);

    UNITTEST_INFO_LOG(">>>>>>>>>>>>>>>>>>>>>>>>>>[resolve failed] key = %s, result = %s, expected = %s",
        keyStr.c_str(), result.c_str(), expected.c_str());
    return false;
}

bool AVMetadataTestBase::CompareMetadata(const std::unordered_map<int32_t, std::string> &result,
    const std::unordered_map<int32_t, std::string> &expected)
{
    std::string resultValue;
    bool success = true;

    for (const auto &[key, expectedValue] : expected) {
        if (result.count(key) == 0) {
            resultValue = "";
        } else {
            resultValue = result.at(key);
        }

        success = success && CompareMetadata(key, resultValue, expectedValue);
    }

    return success;
}

std::string AVMetadataTestBase::GetPrettyDuration(int64_t duration) // ms
{
    static const int32_t msPerSec = 1000;
    static const int32_t msPerMin = 60 * msPerSec;
    static const int32_t msPerHour = 60 * msPerMin;

    int64_t hour = duration / msPerHour;
    int64_t min = (duration % msPerHour) / msPerMin;
    int64_t sec = (duration % msPerMin) / msPerSec;
    int64_t milliSec = duration % msPerSec;

    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(2) << hour << ":"
        << std::setw(2) << min << ":"
        << std::setw(2) << sec << "."
        << std::setw(3) << milliSec;

    return oss.str();
}
} // namespace AVMetadataTestParam
} // namespace Media
} // namespace OHOS