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

#ifndef ENGINE_FACTORY_REPO_H
#define ENGINE_FACTORY_REPO_H

#include <vector>
#include <mutex>
#include <memory>
#include "i_engine_factory.h"

namespace OHOS {
namespace Media {
class EngineFactoryRepo {
public:
    static EngineFactoryRepo &Instance();
    std::shared_ptr<IEngineFactory> GetEngineFactory(IEngineFactory::Scene scene, const std::string &uri = "");

private:
    EngineFactoryRepo() = default;
    ~EngineFactoryRepo();
    int32_t LoadGstreamerEngine();
    int32_t LoadHistreamerEngine();
    void LoadLib(const std::string &libPath);

    std::mutex mutex_;
    std::vector<std::shared_ptr<IEngineFactory>> factorys_;
    std::vector<void*> factoryLibs_;
    bool gstreamerLoad_ = false;
    bool histreamerLoad_ = false;
};
} // namespace Media
} // namespace OHOS
#endif
