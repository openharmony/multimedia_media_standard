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

#include <media_dfx.h>
#include <unistd.h>
#include "securec.h"
#include "media_log.h"
#include "media_errors.h"
#include "hitrace_meter.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaDFX"};
    constexpr uint32_t MAX_STRING_SIZE = 256;
}

namespace OHOS {
namespace Media {
bool MediaEvent::CreateMsg(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char msg[MAX_STRING_SIZE] = {0};
    if (vsnprintf_s(msg, sizeof(msg), sizeof(msg) - 1, format, args) < 0) {
        MEDIA_LOGE("failed to call vsnprintf_s");
        va_end(args);
        return false;
    }
    va_end(args);
    msg_ = msg;
    return true;
}

void MediaEvent::EventWrite(std::string eventName, OHOS::HiviewDFX::HiSysEvent::EventType type,
    std::string module)
{
    int32_t pid = getpid();
    uint32_t uid = getuid();
    OHOS::HiviewDFX::HiSysEvent::Write("MULTIMEDIA", eventName, type,
        "PID", pid,
        "UID", uid,
        "MODULE", module,
        "MSG", msg_);
}

void BehaviorEventWrite(std::string status, std::string moudle)
{
    MediaEvent event;
    if (event.CreateMsg("%s, current state is: %s", "state change", status.c_str())) {
        event.EventWrite("PLAYER_STATE", OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR, moudle);
    } else {
        MEDIA_LOGW("Failed to call CreateMsg");
    }
}

void FaultEventWrite(std::string msg, std::string moudle)
{
    MediaEvent event;
    if (event.CreateMsg("%s", msg.c_str())) {
        event.EventWrite("PLAYER_ERR", OHOS::HiviewDFX::HiSysEvent::EventType::FAULT, moudle);
    } else {
        MEDIA_LOGW("Failed to call CreateMsg");
    }
}

MediaTrace::MediaTrace(const std::string &funcName)
{
    StartTrace(BYTRACE_TAG_ZMEDIA, funcName);
    isSync_ = true;
}

void MediaTrace::TraceBegin(const std::string &funcName, int32_t taskId)
{
    StartAsyncTrace(BYTRACE_TAG_ZMEDIA, funcName, taskId);
}

void MediaTrace::TraceEnd(const std::string &funcName, int32_t taskId)
{
    FinishAsyncTrace(BYTRACE_TAG_ZMEDIA, funcName, taskId);
}

MediaTrace::~MediaTrace()
{
    if (isSync_) {
        FinishTrace(BYTRACE_TAG_ZMEDIA);
    }
}
} // namespace Media
} // namespace OHOS
