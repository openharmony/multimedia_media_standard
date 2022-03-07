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
#include "time_perf.h"
#include <climits>
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaTimePerf"};
    static constexpr int32_t MICRO_SEC_PER_SEC = 1000000;
    static constexpr int64_t MAX_SEC = LLONG_MAX / MICRO_SEC_PER_SEC;
    static constexpr int32_t INVALID_TIME = -1;
}

namespace OHOS {
namespace Media {
void TimePerf::StartPerfRecord(uintptr_t obj, std::string_view tag)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto objIter = objPerfRecords_.find(obj);
    if (objIter == objPerfRecords_.end()) {
        auto ret = objPerfRecords_.emplace(obj, TagRecords{});
        objIter = ret.first;
    }

    auto &tagRecords = objIter->second;
    auto tagIter = tagRecords.find(tag);
    if (tagIter == tagRecords.end()) {
        auto ret = tagRecords.emplace(tag, PerfRecord {INVALID_TIME});
        tagIter = ret.first;
        tagIter->second.currStart = INVALID_TIME;
        tagIter->second.currStop = INVALID_TIME;
        tagIter->second.firstTime = INVALID_TIME;
        tagIter->second.peakTime = INVALID_TIME;
        tagIter->second.avgTime = INVALID_TIME;
        tagIter->second.count = 0;
    }

    auto &record = tagIter->second;
    if (record.currStart != INVALID_TIME) {
        MEDIA_LOGW("already start for obj: 0x%{public}06" PRIXPTR ", tag: %{public}s",
                   FAKE_POINTER(obj), tag.data());
        return;
    }

    struct timeval start {};
    int32_t ret = gettimeofday(&start, nullptr);
    if (ret != 0) {
        MEDIA_LOGW("get time of day failed");
        return;
    }
    TimeVal2USec(start, record.currStart);
}

void TimePerf::StopPerfRecord(uintptr_t obj, std::string_view tag)
{
    struct timeval stop {};
    int32_t getTimeRet = gettimeofday(&stop, nullptr);
    if (getTimeRet != 0) {
        MEDIA_LOGW("get time of day failed");
    }

    std::lock_guard<std::mutex> lock(mutex_);

    auto objIter = objPerfRecords_.find(obj);
    if (objIter == objPerfRecords_.end()) {
        MEDIA_LOGW("no record exits for obj: 0x%{public}06" PRIXPTR ", tag: %{public}s",
                   FAKE_POINTER(obj), tag.data());
        return;
    }

    auto &tagRecords = objIter->second;
    auto tagIter = tagRecords.find(tag);
    if (tagIter == tagRecords.end()) {
        MEDIA_LOGW("no record exits for obj: 0x%{public}06" PRIXPTR ", tag: %{public}s",
                   FAKE_POINTER(obj), tag.data());
        return;
    }

    auto &record = tagIter->second;
    if (record.currStart == INVALID_TIME) {
        MEDIA_LOGW("not start record for obj: 0x%{public}06" PRIXPTR ", tag: %{public}s",
                   FAKE_POINTER(obj), tag.data());
        return;
    }

    if (getTimeRet != 0) {
        record.currStart = INVALID_TIME;
        return;
    }

    TimeVal2USec(stop, record.currStop);
    int64_t currTime = record.currStop - record.currStart;
    if (currTime > record.peakTime || record.peakTime == INVALID_TIME) {
        record.peakTime = currTime;
    }
    if (record.firstTime == INVALID_TIME) {
        record.firstTime = currTime;
    }

    record.avgTime = (record.avgTime * record.count + currTime) / (record.count + 1);
    record.count += 1;
    record.currStart = INVALID_TIME;
    record.currStop = INVALID_TIME;

    MEDIA_LOGD("obj[0x%{public}06" PRIXPTR "] tag[%{public}s] "
               "current take time: %{public}" PRIi64 "us, first time: %{public}" PRIi64 ", "
               "peak time: %{public}" PRIi64 ", avg time: %{public}" PRIi64 ", "
               "count: %{public}" PRIi64 "",
               FAKE_POINTER(obj), tag.data(), currTime, record.firstTime,
               record.peakTime, record.avgTime, record.count);
}

void TimePerf::DumpObjectRecord(uintptr_t obj)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto objIter = objPerfRecords_.find(obj);
    if (objIter == objPerfRecords_.end()) {
        MEDIA_LOGW("no record exits for obj: 0x%{public}06" PRIXPTR, FAKE_POINTER(obj));
        return;
    }

    for (auto &[tag, record] : objIter->second) {
        if (record.count == 0) {
            continue;
        }
        MEDIA_LOGD("obj[0x%{public}06" PRIXPTR "] tag[%{public}s] "
                   ", first time: %{public}" PRIi64 ", peak time: %{public}" PRIi64 ", "
                   "avg time: %{public}" PRIi64 ", count: %{public}" PRIi64 "",
                   FAKE_POINTER(obj), tag.data(), record.firstTime,
                   record.peakTime, record.avgTime, record.count);
    }
}

void TimePerf::CleanObjectRecord(uintptr_t obj)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto objIter = objPerfRecords_.find(obj);
    if (objIter == objPerfRecords_.end()) {
        return;
    }
    (void)objPerfRecords_.erase(objIter);
}

void TimePerf::DumpAllRecord()
{
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto &[obj, tagRecords] : objPerfRecords_) {
        for (auto &[tag, record] : tagRecords) {
            if (record.count == 0) {
                continue;
            }
            MEDIA_LOGD("obj[0x%{public}06" PRIXPTR "] tag[%{public}s], first time: %{public}" PRIi64 ""
                ", peak time: %{public}" PRIi64 ", avg time: %{public}" PRIi64 ", count: %{public}" PRIi64 "",
                FAKE_POINTER(obj), tag.data(), record.firstTime,
                record.peakTime, record.avgTime, record.count);
        }
    }
}

void TimePerf::CleanAllRecord()
{
    std::lock_guard<std::mutex> lock(mutex_);
    objPerfRecords_.clear();
}

void TimePerf::TimeVal2USec(const struct timeval &time, int64_t &usec)
{
    if ((static_cast<int64_t>(time.tv_sec) > MAX_SEC) ||
        (static_cast<int64_t>(time.tv_sec) == MAX_SEC && time.tv_usec > 0) ||
        (time.tv_usec > MICRO_SEC_PER_SEC)) {
        MEDIA_LOGW("time overflow");
        usec = 0;
        return;
    }

    usec = static_cast<int64_t>(time.tv_sec) * MICRO_SEC_PER_SEC + time.tv_usec;
}
} // namespace Media
} // namespace OHOS
