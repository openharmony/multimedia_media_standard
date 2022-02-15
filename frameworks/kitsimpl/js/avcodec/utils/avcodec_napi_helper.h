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

#ifndef AVCODEC_NAPI_HELPER_H
#define AVCODEC_NAPI_HELPER_H
#include <atomic>
#include <mutex>
#include <unordered_set>
#include "avcodec_napi_utils.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class AVCodecNapiHelper {
public:
    AVCodecNapiHelper() = default;
    ~AVCodecNapiHelper() = default;
    bool IsEos();
    bool IsStop();
    bool IsFlushing();
    void SetEos(bool eos);
    void SetStop(bool stop);
    void SetFlushing(bool flushing);
    void PushWork(AVCodecJSCallback *work);
    void RemoveWork(AVCodecJSCallback *work);
    void CancelAllWorks();
    DISALLOW_COPY_AND_MOVE(AVCodecNapiHelper);

private:
    std::atomic<bool> isEos_ = false;
    std::atomic<bool> isStop_ = false;
    std::atomic<bool> isFlushing_ = false;
    std::mutex mutex_;
    std::unordered_set<AVCodecJSCallback *> works_;
};
}
}
#endif
