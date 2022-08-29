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

#include "engine_factory_repo.h"
#include <limits>
#include <cinttypes>
#include <dlfcn.h>
#include "directory_ex.h"
#include "media_errors.h"
#include "media_log.h"
#include "parameter.h"

namespace {
    static constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "EngineFactoryRepo"};
#ifdef __aarch64__
    static const std::string MEDIA_ENGINE_LIB_PATH = "/system/lib64/media";
#else
    static const std::string MEDIA_ENGINE_LIB_PATH = "/system/lib/media";
#endif
    static const std::string MEDIA_ENGINE_LIB_NAME_GSTREAMER = "libmedia_engine_gst.z.so";
    static const std::string MEDIA_ENGINE_LIB_NAME_HISTREAMER = "libmedia_engine_histreamer.z.so";
    static const std::string MEDIA_ENGINE_ENTRY_SYMBOL = "CreateEngineFactory";
}

namespace OHOS {
namespace Media {
using CreateFactoryFunc = IEngineFactory *(*)();

EngineFactoryRepo &EngineFactoryRepo::Instance()
{
    static EngineFactoryRepo inst;
    return inst;
}

EngineFactoryRepo::~EngineFactoryRepo()
{
    for (auto &lib : factoryLibs_) {
        if (lib != nullptr) {
            (void)dlclose(lib);
            lib = nullptr;
        }
    }
}

void EngineFactoryRepo::LoadLib(const std::string &libPath)
{
    void *handle = dlopen(libPath.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (handle == nullptr) {
        MEDIA_LOGE("failed to dlopen %{public}s, errno:%{public}d, errormsg:%{public}s",
                   libPath.c_str(), errno, dlerror());
        return;
    }

    CreateFactoryFunc entry = reinterpret_cast<CreateFactoryFunc>(dlsym(handle, MEDIA_ENGINE_ENTRY_SYMBOL.c_str()));
    if (entry == nullptr) {
        MEDIA_LOGE("failed to dlsym %{public}s for lib %{public}s, errno:%{public}d, errormsg:%{public}s",
            MEDIA_ENGINE_ENTRY_SYMBOL.c_str(), libPath.c_str(), errno, dlerror());
        (void)dlclose(handle);
        return;
    }

    std::shared_ptr<IEngineFactory> factory = std::shared_ptr<IEngineFactory>(entry());
    if (factory == nullptr) {
        MEDIA_LOGE("failed to create engine factory for lib: %{public}s", libPath.c_str());
        (void)dlclose(handle);
        return;
    }

    factoryLibs_.push_back(handle);
    factorys_.push_back(factory);
}

int32_t EngineFactoryRepo::LoadGstreamerEngine()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (gstreamerLoad_) {
        return MSERR_OK;
    }

    std::vector<std::string> allFiles;
    GetDirFiles(MEDIA_ENGINE_LIB_PATH, allFiles);
    for (auto &file : allFiles) {
        std::string::size_type namePos = file.find(MEDIA_ENGINE_LIB_NAME_GSTREAMER);
        if (namePos == std::string::npos) {
            continue;
        } else {
            LoadLib(file);
            gstreamerLoad_ = true;
            break;
        }
    }
    return MSERR_OK;
}

int32_t EngineFactoryRepo::LoadHistreamerEngine()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (histreamerLoad_) {
        MEDIA_LOGI("histramer is enabled");
        return MSERR_OK;
    }

    char useHistreamer[10] = {0}; // 10 for system parameter usage
    auto res = GetParameter("debug.media_service.histreamer", "0", useHistreamer, sizeof(useHistreamer));
    if (res == 1 && useHistreamer[0] == '1') {
        std::vector<std::string> allFiles;
        GetDirFiles(MEDIA_ENGINE_LIB_PATH, allFiles);
        for (auto &file : allFiles) {
            std::string::size_type namePos = file.find(MEDIA_ENGINE_LIB_NAME_HISTREAMER);
            if (namePos == std::string::npos) {
                continue;
            } else {
                LoadLib(file);
                histreamerLoad_ = true;
                break;
            }
        }
    }

    return MSERR_OK;
}

std::shared_ptr<IEngineFactory> EngineFactoryRepo::GetEngineFactory(
    IEngineFactory::Scene scene, const std::string &uri)
{
    (void)LoadGstreamerEngine();
    (void)LoadHistreamerEngine();

    int32_t maxScore = std::numeric_limits<int32_t>::min();
    std::shared_ptr<IEngineFactory> target = nullptr;
    for (auto &factory : factorys_) {
        int32_t score = factory->Score(scene, uri);
        if (maxScore < score) {
            maxScore = score;
            target = factory;
        }
    }
    if (target == nullptr && !factorys_.empty()) {
        target = factorys_.front();
    }

    MEDIA_LOGI("Selected factory: 0x%{public}06" PRIXPTR ", score: %{public}d", FAKE_POINTER(target.get()), maxScore);
    return target;
}
} // namespace Media
} // namespace OHOS
