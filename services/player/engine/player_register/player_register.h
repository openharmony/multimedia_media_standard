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
#ifndef PLAYER_REGISTER_H
#define PLAYER_REGISTER_H

#include "i_player_engine.h"

namespace OHOS {
namespace Media {
class PlayerFactoryMake {
public:
    virtual ~PlayerFactoryMake() = default;
    virtual int Score(std::string uri) const = 0;
    virtual IPlayerEngine *CreatePlayer() const = 0;
};

class PlayerRegister {
public:
    class RegisterHelp {
    public:
        explicit RegisterHelp(std::shared_ptr<PlayerFactoryMake> factoryPtr);
        ~RegisterHelp() = default;
    };

    IPlayerEngine *CreateEngine(const std::string &uri) const;
    static PlayerRegister &GetInstance()
    {
        static PlayerRegister instance;
        return instance;
    }
private:
    std::vector<std::shared_ptr<PlayerFactoryMake>> playerEngineVec_;
};

#define REGISTER_MEDIAPLAYERBASE(K) static PlayerRegister::RegisterHelp regist(K);
}
}
#endif // PLAYER_REGISTER_H