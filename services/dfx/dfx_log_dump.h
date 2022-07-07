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
#ifndef DFX_LOG_DUMP_H
#define DFX_LOG_DUMP_H

#include <string>
#include <hilog/log.h>
#include <thread>
#include <mutex>

namespace OHOS {
namespace Media {
class __attribute__((visibility("default"))) DfxLogDump {
public:
    static DfxLogDump &GetInstance();
    void SaveLog(const char *level, const OHOS::HiviewDFX::HiLogLabel &label, const char *fmt, ...);
    void DumpLog();
private:
    DfxLogDump();
    ~DfxLogDump();
    void UpdateCheckEnable();
    int32_t fileCount_ = 0;
    int32_t lineCount_ = 0;
    std::unique_ptr<std::thread> thread_;
    void TaskProcessor();
    std::mutex mutex_;
    std::condition_variable cond_;
    std::string logString_;
    bool isDump_ = false;
    bool isExit_ = false;
    bool isEnable_ = false;
    bool isNewFile_ = true;
};
} // namespace Media
} // namespace OHOS

#endif