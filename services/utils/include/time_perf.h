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

#ifndef TIME_PERF_H
#define TIME_PERF_H

#include <string_view>
#include <sys/time.h>
#include <unordered_map>
#include <mutex>
#include "nocopyable.h"

namespace OHOS {
namespace Media {
#define SPLICE_IMPL(a, b) a ## b
#define SPLICE(a, b) SPLICE_IMPL(a, b)

#define AUTO_PERF(obj, tag) \
    AutoPerf SPLICE(autoPerf, __LINE__)(reinterpret_cast<uintptr_t>(obj), tag)
#define ASYNC_PERF_START(obj, tag) \
    TimePerf::Inst().StartPerfRecord(reinterpret_cast<uintptr_t>(obj), tag)
#define ASYNC_PERF_STOP(obj, tag) \
    TimePerf::Inst().StopPerfRecord(reinterpret_cast<uintptr_t>(obj), tag)
#define CLEAN_PERF_RECORD(obj) \
    TimePerf::Inst().DumpObjectRecord(reinterpret_cast<uintptr_t>(obj)); \
    TimePerf::Inst().CleanObjectRecord(reinterpret_cast<uintptr_t>(obj))

class __attribute__((visibility("default"))) TimePerf {
public:
    static TimePerf &Inst()
    {
        static TimePerf inst;
        return inst;
    }

    void StartPerfRecord(uintptr_t obj, std::string_view tag);
    void StopPerfRecord(uintptr_t obj, std::string_view tag);
    void DumpObjectRecord(uintptr_t obj);
    void CleanObjectRecord(uintptr_t obj);
    void DumpAllRecord();
    void CleanAllRecord();

private:
    TimePerf() = default;
    ~TimePerf() = default;

    void TimeVal2USec(const struct timeval &time, int64_t &usec);

    struct PerfRecord {
        int64_t currStart; // microsecond
        int64_t currStop;  // microsecond
        int64_t peakTime;
        int64_t firstTime;
        int64_t avgTime;
        int64_t count;
    };

    using TagRecords = std::unordered_map<std::string_view, PerfRecord>;
    std::unordered_map<uintptr_t, TagRecords> objPerfRecords_;
    std::mutex mutex_;
};

struct __attribute__((visibility("default"))) AutoPerf : public NoCopyable {
    AutoPerf(uintptr_t obj, std::string_view tag) : obj_(obj), tag_(tag)
    {
        TimePerf::Inst().StartPerfRecord(obj, tag);
    }

    ~AutoPerf()
    {
        TimePerf::Inst().StopPerfRecord(obj_, tag_);
    }

    uintptr_t obj_;
    std::string_view tag_;
};
} // namespace Media
} // namespace OHOS

#endif
