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

#include "dfx_log_dump.h"
#include <fstream>
#include <unistd.h>
#include <sys/time.h>
#include "securec.h"

namespace {
constexpr int32_t FILE_MAX = 100;
constexpr int32_t FILE_LINE_MAX = 50000;
}
namespace OHOS {
namespace Media {
DfxLogDump &DfxLogDump::GetInstance()
{
    static DfxLogDump dfxLogDump;
    return dfxLogDump;
}

DfxLogDump::DfxLogDump()
{
    thread_ = std::make_unique<std::thread>(&DfxLogDump::TaskProcessor, this);
}

DfxLogDump::~DfxLogDump()
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        isExit_ = true;
        cond_.notify_all();
    }
    if (thread_ != nullptr && thread_->joinable()) {
        thread_->join();
    }
}

void DfxLogDump::DumpLog()
{
    std::unique_lock<std::mutex> lock(mutex_);
    isDump_ = true;
    cond_.notify_all();
}

static void AddNewLog(std::string &logStr)
{
    struct timeval time = {};
    (void)gettimeofday(&time, nullptr);
    int64_t second = time.tv_sec % 60;
    int64_t allMinute = time.tv_sec / 60;
    int64_t minute = allMinute % 60;
    int64_t hour = allMinute / 60 % 24;
    int64_t mSecond = time.tv_usec / 1000;

    logStr += std::to_string(hour);
    logStr += ":";
    logStr += std::to_string(minute);
    logStr += ":";
    logStr += std::to_string(second);
    logStr += ":";
    logStr += std::to_string(mSecond);
    logStr += " ";
    logStr += level;
    logStr += " pid:";
    logStr += std::to_string(getpid());
    logStr += " tid:";
    logStr += std::to_string(gettid());
    logStr += " ";
    logStr += label.tag;
    logStr += ":";
}

void DfxLogDump::SaveLog(const char *level, const OHOS::HiviewDFX::HiLogLabel &label, const char *fmt, ...)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (!isEnable_) {
        return;
    }
    std::string temp = "";
    std::string fmtStr = fmt;
    int32_t srcPos = 0;
    auto dtsPos = fmtStr.find("{public}", srcPos);
    const int32_t pubLen = 8;
    while (dtsPos != std::string::npos) {
        temp += fmtStr.substr(srcPos, dtsPos - srcPos);
        srcPos = dtsPos + pubLen;
        dtsPos = fmtStr.find("{public}", srcPos);
    }
    temp += fmtStr.substr(srcPos);

    va_list ap;
    va_start(ap, fmt);
    constexpr uint8_t maxLogLen = 255;
    char logBuf[maxLogLen];
    auto ret = vsnprintf_s(logBuf, maxLogLen, maxLogLen - 1, temp.c_str(), ap);
    va_end(ap);

    AddNewLog(logString_);
    if (ret < 0) {
        logString_ += "dump log error";
    } else {
        logString_ += logBuf;
    }
    logString_ += "\n";
    lineCount_++;
    if (lineCount_ >= FILE_LINE_MAX) {
        cond_.notify_all();
    }
}

void DfxLogDump::UpdateCheckEnable()
{
    std::string file = "/data/media/log/check.config";
    std::ofstream ofStream(file);
    if (!ofStream.is_open()) {
        isEnable_ = false;
    }
    ofStream.close();
    isEnable_ = true;
}

void DfxLogDump::TaskProcessor()
{
    while (true) {
        std::string temp;
        int32_t lineCount = 0;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (isExit_) {
                return;
            }
            static constexpr int32_t timeout = 60; // every 1 minute have a log
            cond_.wait_for(lock, std::chrono::seconds(timeout),
                [this] {
                    UpdateCheckEnable();
                    return isExit_ || isDump_ || lineCount_ >= FILE_LINE_MAX || !logString_.empty();
            });
            isDump_ = false;
            lineCount = lineCount_;
            lineCount_ = lineCount_ >= FILE_LINE_MAX ? 0 : lineCount_;
            swap(logString_, temp);
        }

        std::string file = "/data/media/log/";
        file += std::to_string(getpid());
        file += "_hilog_media.log";
        file += std::to_string(fileCount_);
        std::ofstream ofStream;
        if (isNewFile_) {
            ofStream.open(file, std::ios::out | std::ios::trunc);
        } else {
            ofStream.open(file, std::ios::out | std::ios::app);
        }
        if (!ofStream.is_open()) {
            return;
        }
        isNewFile_ = false;
        if (lineCount >= FILE_LINE_MAX) {
            isNewFile_ = true;
            fileCount_++;
            fileCount_ = fileCount_ > FILE_MAX ? 0 : fileCount_;
        }
        ofStream.write(temp.c_str(), temp.size());
        ofStream.close();
    }
}
} // namespace Media
} // namespace OHOS