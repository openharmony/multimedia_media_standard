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

#ifndef MEDIA_TIME_MONITOR_H
#define MEDIA_TIME_MONITOR_H

#include <string>
#include <mutex>
#include <map>
#include <sys/time.h>

namespace OHOS {
namespace Media {
class __attribute__((visibility("default"))) TimeMonitor {
public:
    explicit TimeMonitor(const std::string &objectName);
    ~TimeMonitor();
    void StartTime();
    void FinishTime();

private:
    enum TimeValType {
        TIME_VAL_MS = 1000,         /* sec to ms */
        TIME_VAL_US = 1000000,      /* sec to us */
        TIME_VAL_NS = 1000000000,   /* sec to ns */
    };
    int64_t Timeval2Sec(const timeval &tv, TimeValType valType) const;
    std::string objectName_ = "Unknown";
    struct timeval startTime_ = {};
    struct timeval finishTime_ = {};
    bool isStart_ = false;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_TIME_MONITOR_H
