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

#ifndef I_ENGINE_FACTORY_H
#define I_ENGINE_FACTORY_H

#include <string>
#include <memory>
#include "i_player_engine.h"
#include "i_recorder_engine.h"
#include "i_avmetadatahelper_engine.h"
#include "i_avcodeclist_engine.h"

namespace OHOS {
namespace Media {
class IEngineFactory {
public:
    enum class Scene {
        SCENE_PLAYBACK,
        SCENE_AVMETADATA,
        SCENE_RECORDER,
        SCENE_AVCODECLIST,
    };

    virtual ~IEngineFactory() = default;
    virtual int32_t Score(Scene scene, const std::string &uri = "") = 0;
    virtual std::unique_ptr<IPlayerEngine> CreatePlayerEngine() = 0;
    virtual std::unique_ptr<IRecorderEngine> CreateRecorderEngine() = 0;
    virtual std::unique_ptr<IAVMetadataHelperEngine> CreateAVMetadataHelperEngine() = 0;
    virtual std::unique_ptr<IAVCodecListEngine> CreateAVCodecListEngine() = 0;

protected:
    static constexpr int32_t MAX_SCORE = 100;
    static constexpr int32_t MIN_SCORE = 0;
};
}
}
#endif