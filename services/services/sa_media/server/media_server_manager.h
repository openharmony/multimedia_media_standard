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
#include <functional>
#include "iremote_object.h"
#include "ipc_skeleton.h"
#include "recorder_service_stub.h"
#include "player_service_stub.h"
#include "avcodec_service_stub.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
using DumperEntry = std::function<int32_t(int32_t)>;
struct Dumper {
    pid_t pid_;
    pid_t uid_;
    DumperEntry entry_;
    sptr<IRemoteObject> remoteObject_;
};
class MediaServerManager : public NoCopyable {
public:
    static MediaServerManager &GetInstance();
    ~MediaServerManager();

    enum StubType {
        RECORDER = 0,
        PLAYER,
        AVMETADATAHELPER,
        AVCODECLIST,
        AVCODEC,
    };
    sptr<IRemoteObject> CreateStubObject(StubType type);
    void DestroyStubObject(StubType type, sptr<IRemoteObject> object);
    void DestroyStubObjectForPid(pid_t pid);
    int32_t Dump(int32_t fd, const std::vector<std::u16string> &args);
    void DestroyDumper(StubType type, sptr<IRemoteObject> object);
    void DestroyDumperForPid(pid_t pid);

private:
    MediaServerManager();
    sptr<IRemoteObject> CreatePlayerStubObject();
    sptr<IRemoteObject> CreateRecorderStubObject();
    sptr<IRemoteObject> CreateAVMetadataHelperStubObject();
    sptr<IRemoteObject> CreateAVCodecListStubObject();
    sptr<IRemoteObject> CreateAVCodecStubObject();
    std::map<sptr<IRemoteObject>, pid_t> recorderStubMap_;
    std::map<sptr<IRemoteObject>, pid_t> playerStubMap_;
    std::map<sptr<IRemoteObject>, pid_t> avMetadataHelperStubMap_;
    std::map<sptr<IRemoteObject>, pid_t> avCodecListStubMap_;
    std::map<sptr<IRemoteObject>, pid_t> avCodecStubMap_;
    std::map<StubType, std::vector<Dumper>> dumperTbl_;

    std::mutex mutex_;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_SERVER_MANAGER_H
