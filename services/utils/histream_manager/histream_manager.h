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

#ifndef HISTREAM_MANAGER_H
#define HISTREAM_MANAGER_H

#include <mutex>

namespace OHOS {
namespace Media  {
class __attribute__((visibility("default"))) HistreamManager {
public:
    static HistreamManager &Instance()
    {
        static HistreamManager inst;
        return inst;
    }

    int32_t SetUp();
    void UpdateLogLevel() const;
private:
    HistreamManager() = default;
    ~HistreamManager() = default;

    bool isInit_ = false;
    std::mutex mutex_;
};
}
}
#endif