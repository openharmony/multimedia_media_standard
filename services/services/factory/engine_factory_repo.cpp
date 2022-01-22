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

namespace {
    static constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "EngineFactoryRepo"};
#ifdef __aarch64__
    static const std::string MEDIA_ENGINE_LIB_PATH = "/system/lib64/media";
#else
    static const std::string MEDIA_ENGINE_LIB_PATH = "/system/lib/media";
#endif
    static const std::string MEDIA_ENGINE_LIB_NAME_PREFIX = "libmedia_engine_";
    static const std::string MEDIA_ENGINE_LIB_NAME_SUFFIX = ".z.so";
    static const std::string MEDIA_ENGINE_ENTRY_SYMBOL = "CreateEngineFactory";
}

namespace OHOS {
namespace Media {
using CreateFactoryFunc = IEngineFactory *(*)();

static bool IsAlphaNumUnderLine(const std::string str)
{
    for (auto &c : str) {
        if (!(std::isalnum(c) || c == '_')) {
            return false;
        }
    }
    return true;
}

static std::vector<std::string> GetMediaEngineLibs()
{
    std::vector<std::string> allFiles;
    std::vector<std::string> allLibs;
    GetDirFiles(MEDIA_ENGINE_LIB_PATH, allFiles);
    for (auto &file : allFiles) {
        std::string::size_type namePos = file.find(MEDIA_ENGINE_LIB_NAME_PREFIX);
        if (namePos == std::string::npos) {
            continue;
        }
        namePos += MEDIA_ENGINE_LIB_NAME_PREFIX.size();
        std::string::size_type nameEnd = file.rfind(MEDIA_ENGINE_LIB_NAME_SUFFIX);
        if ((nameEnd == std::string::npos) || (nameEnd + MEDIA_ENGINE_LIB_NAME_SUFFIX.size() != file.size())) {
            continue;
        }
        MEDIA_LOGI("lib: %{public}s", file.c_str());
        if (!IsAlphaNumUnderLine(file.substr(namePos, nameEnd - namePos))) {
            continue;
        }
        MEDIA_LOGI("find media engine library: %{public}s", file.c_str());
        allLibs.push_back(file);
    }

    return allLibs;
}

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
        }
    }
}

int32_t EngineFactoryRepo::Init()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (inited_) {
        return MSERR_OK;
    }

    std::vector<std::string> allLibs = GetMediaEngineLibs();
    for (auto &lib : allLibs) {
        LoadLib(lib);
    }
    MEDIA_LOGI("load engine factory count: %{public}zu", factorys_.size());

    inited_ = true;
    return MSERR_OK;
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

std::shared_ptr<IEngineFactory> EngineFactoryRepo::GetEngineFactory(
    IEngineFactory::Scene scene, const std::string &uri)
{
    (void)Init();

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
}
}
