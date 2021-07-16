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

#ifndef MEDIA_SERVER_MANAGER_H
#define MEDIA_SERVER_MANAGER_H

#include <memory>
#include "iremote_object.h"
#include "recorder_service_stub.h"
#include "player_service_stub.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class MediaServerManager {
public:
    static MediaServerManager &GetInstance();
    ~MediaServerManager();
    DISALLOW_COPY_AND_MOVE(MediaServerManager);

    enum StubType {
        RECORDER = 0,
        PLAYER,
    };
    sptr<IRemoteObject> CreateStubObject(StubType type);
    void DestroyStubObject(StubType type, sptr<IRemoteObject> object);

private:
    MediaServerManager();

private:
    std::list<sptr<IRemoteObject>> recorderStubList_;
    std::list<sptr<IRemoteObject>> playerStubList_;
    std::mutex mutex_;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_SERVER_MANAGER_H