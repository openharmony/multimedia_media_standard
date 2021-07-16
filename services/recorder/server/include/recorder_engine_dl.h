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

#ifndef RECORDER_ENGINE_DL_H
#define RECORDER_ENGINE_DL_H

#include "i_recorder_engine.h"

namespace OHOS {
namespace Media {
class RecorderEngineDl {
public:
    static RecorderEngineDl &Instance();
    std::shared_ptr<IRecorderEngine> CreateEngine();
    ~RecorderEngineDl();

private:
    RecorderEngineDl();
    void GetEntry();

private:
    std::mutex mutex_;
    void *engineLib_ = nullptr;
    using CreateEngineFunc = IRecorderEngine *(*)();
    CreateEngineFunc createFunc_ = nullptr;
};
} // OHOS
} // Media
#endif // RECORDER_ENGINE_DL_H