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

#ifndef OHOS_MEDIA_LOG_H
#define OHOS_MEDIA_LOG_H

#include <hilog/log.h>
#include <cinttypes>

namespace OHOS {
#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002B00

#define __FILENAME__ (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)

#define __MEDIA_LOG(func, fmt, args...)                                                       \
    do {                                                                                      \
        (void)func(LABEL, "{%{public}s():%{public}d} " fmt, __FUNCTION__, __LINE__, ##args);  \
    } while (0)

#define MEDIA_LOGD(fmt, ...) __MEDIA_LOG(::OHOS::HiviewDFX::HiLog::Debug, fmt, ##__VA_ARGS__)
#define MEDIA_LOGI(fmt, ...) __MEDIA_LOG(::OHOS::HiviewDFX::HiLog::Info, fmt, ##__VA_ARGS__)
#define MEDIA_LOGW(fmt, ...) __MEDIA_LOG(::OHOS::HiviewDFX::HiLog::Warn, fmt, ##__VA_ARGS__)
#define MEDIA_LOGE(fmt, ...) __MEDIA_LOG(::OHOS::HiviewDFX::HiLog::Error, fmt, ##__VA_ARGS__)
#define MEDIA_LOGF(fmt, ...) __MEDIA_LOG(::OHOS::HiviewDFX::HiLog::Fatal, fmt, ##__VA_ARGS__)

#define CHECK_AND_RETURN(cond)                      \
    do {                                            \
        if (!(cond)) {                                \
            MEDIA_LOGE("%{public}s, check failed!", #cond); \
            return;                                 \
        }                                           \
    } while (0)

#define CHECK_AND_RETURN_RET(cond, ret)                           \
    do {                                                          \
        if (!(cond)) {                                              \
            MEDIA_LOGE("%{public}s, check failed! ret = %{public}s", #cond, #ret); \
            return ret;                                           \
        }                                                         \
    } while (0)

#define CHECK_AND_RETURN_RET_LOG(cond, ret, fmt, ...)  \
    do {                                               \
        if (!(cond)) {                                 \
            MEDIA_LOGE(fmt, ##__VA_ARGS__);            \
            return ret;                                \
        }                                              \
    } while (0);

#define CHECK_AND_RETURN_LOG(cond, fmt, ...)           \
    do {                                               \
        if (!(cond)) {                                 \
            MEDIA_LOGE(fmt, ##__VA_ARGS__);            \
            return;                                    \
        }                                              \
    } while (0);

#define CHECK_AND_BREAK_LOG(cond, fmt, ...)            \
    do {                                               \
        if (!(cond)) {                                 \
            MEDIA_LOGE(fmt, ##__VA_ARGS__);            \
            break;                                     \
        }                                              \
    } while (0);

#define POINTER_MASK 0x00FFFFFF
#define FAKE_POINTER(addr) (POINTER_MASK & reinterpret_cast<uintptr_t>(addr))
} // OHOS
#endif // OHOS_MEDIA_LOG_H
