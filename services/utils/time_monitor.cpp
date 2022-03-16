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
#include "time_monitor.h"
#include <cinttypes>
#include "media_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "TimeMonitor"};
}

namespace OHOS {
namespace Media {
TimeMonitor::TimeMonitor(const std::string &objectName)
    : objectName_(objectName)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

TimeMonitor::~TimeMonitor()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void TimeMonitor::StartTime()
{
    int ret = gettimeofday(&startTime_, nullptr);
    if (ret == -1) {
        MEDIA_LOGE("get current time failed!");
    }
    isStart_ = true;
}

void TimeMonitor::FinishTime()
{
    if (!isStart_) {
        return;
    }
    int ret = gettimeofday(&finishTime_, nullptr);
    if (ret == -1) {
        MEDIA_LOGE("get current time failed!");
    }
    MEDIA_LOGD("%{public}s: elapsed time = %{public}" PRId64 " ms", objectName_.c_str(),
               (Timeval2Sec(finishTime_, TIME_VAL_MS) - Timeval2Sec(startTime_, TIME_VAL_MS)));
    isStart_ = false;
}

int64_t TimeMonitor::Timeval2Sec(const timeval &tv, TimeValType valType) const
{
    if ((valType == TIME_VAL_MS) || (valType == TIME_VAL_US) || (valType == TIME_VAL_NS)) {
        if ((static_cast<int64_t>(tv.tv_sec) < (LLONG_MAX / valType)) &&
            (static_cast<int64_t>(tv.tv_usec) <= TIME_VAL_US)) {
            return static_cast<int64_t>(tv.tv_sec) * valType + static_cast<int64_t>(tv.tv_usec) * valType / TIME_VAL_US;
        } else {
            MEDIA_LOGW("time overflow");
        }
    }
    return -1;
}
} // namespace Media
} // namespace OHOS
