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

#include "media_server_manager.h"
#include <unordered_set>
#include "recorder_service_stub.h"
#include "player_service_stub.h"
#include "avmetadatahelper_service_stub.h"
#include "avcodeclist_service_stub.h"
#include "avmuxer_service_stub.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaServerManager"};
}

namespace OHOS {
namespace Media {
constexpr uint32_t SERVER_MAX_NUMBER = 16;
MediaServerManager &MediaServerManager::GetInstance()
{
    static MediaServerManager instance;
    return instance;
}

int32_t WriteInfo(int32_t fd, std::string &dumpString, std::vector<Dumper> dumpers, bool needDetail)
{
    int32_t i = 0;
    for (auto iter : dumpers) {
        dumpString += "-----Instance #" + std::to_string(i) + ": ";
        dumpString += "pid = ";
        dumpString += std::to_string(iter.pid_);
        dumpString += " uid = ";
        dumpString += std::to_string(iter.uid_);
        dumpString += "-----\n";
        write(fd, dumpString.c_str(), dumpString.size());
        dumpString.clear();
        i++;
        if (!needDetail) {
            continue;
        }
        if (iter.entry_(fd) != MSERR_OK) {
            return OHOS::INVALID_OPERATION;
        }
    }
    write(fd, dumpString.c_str(), dumpString.size());
    dumpString.clear();

    return OHOS::NO_ERROR;
}

int32_t MediaServerManager::Dump(int32_t fd, const std::vector<std::u16string> &args)
{
    std::string dumpString;
    std::unordered_set<std::u16string> argSets;
    for (decltype(args.size()) index = 0; index < args.size(); ++index) {
        argSets.insert(args[index]);
    }

    dumpString += "------------------PlayerServer------------------\n";
    if (WriteInfo(fd, dumpString, dumperTbl_[StubType::PLAYER],
        argSets.find(u"player") != argSets.end()) != OHOS::NO_ERROR) {
        MEDIA_LOGW("Failed to write PlayerServer information");
        return OHOS::INVALID_OPERATION;
    }

    dumpString += "------------------RecorderServer------------------\n";
    if (WriteInfo(fd, dumpString, dumperTbl_[StubType::RECORDER],
        argSets.find(u"recorder") != argSets.end()) != OHOS::NO_ERROR) {
        MEDIA_LOGW("Failed to write RecorderServer information");
        return OHOS::INVALID_OPERATION;
    }

    dumpString += "------------------CodecServer------------------\n";
    if (WriteInfo(fd, dumpString, dumperTbl_[StubType::AVCODEC],
        argSets.find(u"codec") != argSets.end()) != OHOS::NO_ERROR) {
        MEDIA_LOGW("Failed to write CodecServer information");
        return OHOS::INVALID_OPERATION;
    }

    dumpString += "------------------AVMetaServer------------------\n";
    if (WriteInfo(fd, dumpString, dumperTbl_[StubType::AVMETADATAHELPER], false) != OHOS::NO_ERROR) {
        MEDIA_LOGW("Failed to write AVMetaServer information");
        return OHOS::INVALID_OPERATION;
    }

    return OHOS::NO_ERROR;
}

MediaServerManager::MediaServerManager()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaServerManager::~MediaServerManager()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

sptr<IRemoteObject> MediaServerManager::CreateStubObject(StubType type)
{
    std::lock_guard<std::mutex> lock(mutex_);
    switch (type) {
        case RECORDER: {
            return CreateRecorderStubObject();
        }
        case PLAYER: {
            return CreatePlayerStubObject();
        }
        case AVMETADATAHELPER: {
            return CreateAVMetadataHelperStubObject();
        }
        case AVCODECLIST: {
            return CreateAVCodecListStubObject();
        }
        case AVCODEC: {
            return CreateAVCodecStubObject();
        }
        case AVMUXER: {
            return CreateAVMuxerStubObject();
        }
        default: {
            MEDIA_LOGE("default case, media server manager failed");
            return nullptr;
        }
    }
}

sptr<IRemoteObject> MediaServerManager::CreatePlayerStubObject()
{
    if (playerStubMap_.size() >= SERVER_MAX_NUMBER) {
        MEDIA_LOGE("The number of player services(%{public}zu) has reached the upper limit."
            "Please release the applied resources.", playerStubMap_.size());
        return nullptr;
    }
    sptr<PlayerServiceStub> playerStub = PlayerServiceStub::Create();
    if (playerStub == nullptr) {
        MEDIA_LOGE("failed to create PlayerServiceStub");
        return nullptr;
    }

    sptr<IRemoteObject> object = playerStub->AsObject();
    if (object != nullptr) {
        pid_t pid = IPCSkeleton::GetCallingPid();
        playerStubMap_[object] = pid;

        Dumper dumper;
        dumper.entry_ = [player = playerStub](int32_t fd) -> int32_t {
            return player->DumpInfo(fd);
        };
        dumper.pid_ = pid;
        dumper.uid_ = IPCSkeleton::GetCallingUid();
        dumper.remoteObject_ = object;
        dumperTbl_[StubType::PLAYER].emplace_back(dumper);
        MEDIA_LOGD("The number of player services(%{public}zu) pid(%{public}d).",
            playerStubMap_.size(), pid);
    }
    return object;
}

sptr<IRemoteObject> MediaServerManager::CreateRecorderStubObject()
{
    if (recorderStubMap_.size() >= SERVER_MAX_NUMBER) {
        MEDIA_LOGE("The number of recorder services(%{public}zu) has reached the upper limit."
            "Please release the applied resources.", recorderStubMap_.size());
        return nullptr;
    }
    sptr<RecorderServiceStub> recorderStub = RecorderServiceStub::Create();
    if (recorderStub == nullptr) {
        MEDIA_LOGE("failed to create RecorderServiceStub");
        return nullptr;
    }
    sptr<IRemoteObject> object = recorderStub->AsObject();
    if (object != nullptr) {
        pid_t pid = IPCSkeleton::GetCallingPid();
        recorderStubMap_[object] = pid;

        Dumper dumper;
        dumper.entry_ = [recorder = recorderStub](int32_t fd) -> int32_t {
            return recorder->DumpInfo(fd);
        };
        dumper.pid_ = pid;
        dumper.uid_ = IPCSkeleton::GetCallingUid();
        dumper.remoteObject_ = object;
        dumperTbl_[StubType::RECORDER].emplace_back(dumper);
        MEDIA_LOGD("The number of recorder services(%{public}zu) pid(%{public}d).",
            recorderStubMap_.size(), pid);
    }
    return object;
}

sptr<IRemoteObject> MediaServerManager::CreateAVMetadataHelperStubObject()
{
    constexpr uint32_t metadataHelperNumMax = 32;
    if (avMetadataHelperStubMap_.size() >= metadataHelperNumMax) {
        MEDIA_LOGE("The number of avmetadatahelper services(%{public}zu) has reached the upper limit."
            "Please release the applied resources.", avMetadataHelperStubMap_.size());
        return nullptr;
    }
    sptr<AVMetadataHelperServiceStub> avMetadataHelperStub = AVMetadataHelperServiceStub::Create();
    if (avMetadataHelperStub == nullptr) {
        MEDIA_LOGE("failed to create AVMetadataHelperServiceStub");
        return nullptr;
    }
    sptr<IRemoteObject> object = avMetadataHelperStub->AsObject();
    if (object != nullptr) {
        pid_t pid = IPCSkeleton::GetCallingPid();
        avMetadataHelperStubMap_[object] = pid;

        Dumper dumper;
        dumper.pid_ = pid;
        dumper.uid_ = IPCSkeleton::GetCallingUid();
        dumper.remoteObject_ = object;
        dumperTbl_[StubType::AVMETADATAHELPER].emplace_back(dumper);

        MEDIA_LOGD("The number of avmetadatahelper services(%{public}zu) pid(%{public}d).",
            avMetadataHelperStubMap_.size(), pid);
    }
    return object;
}

sptr<IRemoteObject> MediaServerManager::CreateAVCodecListStubObject()
{
    if (avCodecListStubMap_.size() >= SERVER_MAX_NUMBER) {
        MEDIA_LOGE("The number of codeclist services(%{public}zu) has reached the upper limit."
            "Please release the applied resources.", avCodecListStubMap_.size());
        return nullptr;
    }
    sptr<AVCodecListServiceStub> avCodecListStub = AVCodecListServiceStub::Create();
    if (avCodecListStub == nullptr) {
        MEDIA_LOGE("failed to create AVCodecListServiceStub");
        return nullptr;
    }
    sptr<IRemoteObject> object = avCodecListStub->AsObject();
    if (object != nullptr) {
        pid_t pid = IPCSkeleton::GetCallingPid();
        avCodecListStubMap_[object] = pid;
        MEDIA_LOGD("The number of codeclist services(%{public}zu).", avCodecListStubMap_.size());
    }
    return object;
}

sptr<IRemoteObject> MediaServerManager::CreateAVCodecStubObject()
{
    if (avCodecStubMap_.size() >= SERVER_MAX_NUMBER) {
        MEDIA_LOGE("The number of avcodec services(%{public}zu) has reached the upper limit."
            "Please release the applied resources.", avCodecStubMap_.size());
        return nullptr;
    }
    sptr<AVCodecServiceStub> avCodecHelperStub = AVCodecServiceStub::Create();
    if (avCodecHelperStub == nullptr) {
        MEDIA_LOGE("failed to create AVCodecServiceStub");
        return nullptr;
    }
    sptr<IRemoteObject> object = avCodecHelperStub->AsObject();
    if (object != nullptr) {
        pid_t pid = IPCSkeleton::GetCallingPid();
        avCodecStubMap_[object] = pid;

        Dumper dumper;
        dumper.entry_ = [avcodec = avCodecHelperStub](int32_t fd) -> int32_t {
            return avcodec->DumpInfo(fd);
        };
        dumper.pid_ = pid;
        dumper.uid_ = IPCSkeleton::GetCallingUid();
        dumper.remoteObject_ = object;
        dumperTbl_[StubType::AVCODEC].emplace_back(dumper);
        MEDIA_LOGD("The number of avcodec services(%{public}zu).", avCodecStubMap_.size());
    }
    return object;
}

sptr<IRemoteObject> MediaServerManager::CreateAVMuxerStubObject()
{
    if (avmuxerStubMap_.size() >= SERVER_MAX_NUMBER) {
        MEDIA_LOGE("The number of avmuxer services(%{public}zu) has reached the upper limit."
            "Please release the applied resources.", avmuxerStubMap_.size());
        return nullptr;
    }
    sptr<AVMuxerServiceStub> avmuxerStub = AVMuxerServiceStub::Create();
    if (avmuxerStub == nullptr) {
        MEDIA_LOGE("failed to create AVMuxerServiceStub");
        return nullptr;
    }
    sptr<IRemoteObject> object = avmuxerStub->AsObject();
    if (object != nullptr) {
        pid_t pid = IPCSkeleton::GetCallingPid();
        avmuxerStubMap_[object] = pid;
        MEDIA_LOGD("The number of avmuxer services(%{public}zu).", avmuxerStubMap_.size());
    }
    return object;
}

void MediaServerManager::DestroyStubObject(StubType type, sptr<IRemoteObject> object)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pid_t pid = IPCSkeleton::GetCallingPid();
    DestroyDumper(type, object);
    switch (type) {
        case RECORDER: {
            for (auto it = recorderStubMap_.begin(); it != recorderStubMap_.end(); it++) {
                if (it->first == object) {
                    MEDIA_LOGD("destroy recorder stub services(%{public}zu) pid(%{public}d).",
                        recorderStubMap_.size(), pid);
                    (void)recorderStubMap_.erase(it);
                    return;
                }
            }
            MEDIA_LOGE("find recorder object failed, pid(%{public}d).", pid);
            break;
        }
        case PLAYER: {
            for (auto it = playerStubMap_.begin(); it != playerStubMap_.end(); it++) {
                if (it->first == object) {
                    MEDIA_LOGD("destroy player stub services(%{public}zu) pid(%{public}d).",
                        playerStubMap_.size(), pid);
                    (void)playerStubMap_.erase(it);
                    return;
                }
            }
            MEDIA_LOGE("find player object failed, pid(%{public}d).", pid);
            break;
        }
        case AVMETADATAHELPER: {
            for (auto it = avMetadataHelperStubMap_.begin(); it != avMetadataHelperStubMap_.end(); it++) {
                if (it->first == object) {
                    MEDIA_LOGD("destroy avmetadatahelper stub services(%{public}zu) pid(%{public}d).",
                        avMetadataHelperStubMap_.size(), pid);
                    (void)avMetadataHelperStubMap_.erase(it);
                    return;
                }
            }
            MEDIA_LOGE("find avmetadatahelper object failed, pid(%{public}d).", pid);
            break;
        }
        case AVCODEC: {
            for (auto it = avCodecStubMap_.begin(); it != avCodecStubMap_.end(); it++) {
                if (it->first == object) {
                    MEDIA_LOGD("destroy avcodec stub services(%{public}zu) pid(%{public}d).",
                        avCodecStubMap_.size(), pid);
                    (void)avCodecStubMap_.erase(it);
                    return;
                }
            }
            MEDIA_LOGE("find avcodec object failed, pid(%{public}d).", pid);
            break;
        }
        case AVCODECLIST: {
            for (auto it = avCodecListStubMap_.begin(); it != avCodecListStubMap_.end(); it++) {
                if (it->first == object) {
                    MEDIA_LOGD("destroy avcodeclist stub services(%{public}zu) pid(%{public}d).",
                        avCodecListStubMap_.size(), pid);
                    (void)avCodecListStubMap_.erase(it);
                    return;
                }
            }
            MEDIA_LOGE("find avcodeclist object failed, pid(%{public}d).", pid);
            break;
        }
        case AVMUXER: {
            for (auto it = avmuxerStubMap_.begin(); it != avmuxerStubMap_.end(); it++) {
                if (it->first == object) {
                    MEDIA_LOGD("destory avmuxer stub services(%{public}zu) pid(%{public}d).",
                        avmuxerStubMap_.size(), pid);
                    (void)avmuxerStubMap_.erase(it);
                    return;
                }
            }
            MEDIA_LOGE("find avmuxer object failed, pid(%{public}d).", pid);
            break;
        }
        default: {
            MEDIA_LOGE("default case, media server manager failed, pid(%{public}d).", pid);
            break;
        }
    }
}

void MediaServerManager::DestroyStubObjectForPid(pid_t pid)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("recorder stub services(%{public}zu) pid(%{public}d).", recorderStubMap_.size(), pid);
    DestroyDumperForPid(pid);
    for (auto itRecorder = recorderStubMap_.begin(); itRecorder != recorderStubMap_.end();) {
        if (itRecorder->second == pid) {
            itRecorder = recorderStubMap_.erase(itRecorder);
        } else {
            itRecorder++;
        }
    }
    MEDIA_LOGD("recorder stub services(%{public}zu).", recorderStubMap_.size());

    MEDIA_LOGD("player stub services(%{public}zu) pid(%{public}d).", playerStubMap_.size(), pid);
    for (auto itPlayer = playerStubMap_.begin(); itPlayer != playerStubMap_.end();) {
        if (itPlayer->second == pid) {
            itPlayer = playerStubMap_.erase(itPlayer);
        } else {
            itPlayer++;
        }
    }
    MEDIA_LOGD("player stub services(%{public}zu).", playerStubMap_.size());

    MEDIA_LOGD("avmetadatahelper stub services(%{public}zu) pid(%{public}d).", avMetadataHelperStubMap_.size(), pid);
    for (auto itAvMetadata = avMetadataHelperStubMap_.begin(); itAvMetadata != avMetadataHelperStubMap_.end();) {
        if (itAvMetadata->second == pid) {
            itAvMetadata = avMetadataHelperStubMap_.erase(itAvMetadata);
        } else {
            itAvMetadata++;
        }
    }
    MEDIA_LOGD("avmetadatahelper stub services(%{public}zu).", avMetadataHelperStubMap_.size());

    MEDIA_LOGD("avcodec stub services(%{public}zu) pid(%{public}d).", avCodecStubMap_.size(), pid);
    for (auto itAvCodec = avCodecStubMap_.begin(); itAvCodec != avCodecStubMap_.end();) {
        if (itAvCodec->second == pid) {
            itAvCodec = avCodecStubMap_.erase(itAvCodec);
        } else {
            itAvCodec++;
        }
    }
    MEDIA_LOGD("avcodec stub services(%{public}zu).", avCodecStubMap_.size());

    MEDIA_LOGD("avcodeclist stub services(%{public}zu) pid(%{public}d).", avCodecListStubMap_.size(), pid);
    for (auto itAvCodecList = avCodecListStubMap_.begin(); itAvCodecList != avCodecListStubMap_.end();) {
        if (itAvCodecList->second == pid) {
            itAvCodecList = avCodecListStubMap_.erase(itAvCodecList);
        } else {
            itAvCodecList++;
        }
    }
    MEDIA_LOGD("avcodeclist stub services(%{public}zu).", avCodecListStubMap_.size());

    MEDIA_LOGD("avmuxer stub services(%{public}zu) pid(%{public}d).", avmuxerStubMap_.size(), pid);
    for (auto itAVMuxer = avmuxerStubMap_.begin(); itAVMuxer != avmuxerStubMap_.end();) {
        if (itAVMuxer->second == pid) {
            itAVMuxer = avmuxerStubMap_.erase(itAVMuxer);
        } else {
            itAVMuxer++;
        }
    }
    MEDIA_LOGD("avmuxer stub services(%{public}zu).", avmuxerStubMap_.size());
}

void MediaServerManager::DestroyDumper(StubType type, sptr<IRemoteObject> object)
{
    for (auto it = dumperTbl_[type].begin(); it != dumperTbl_[type].end(); it++) {
        if (it->remoteObject_ == object) {
            (void)dumperTbl_[type].erase(it);
            MEDIA_LOGD("MediaServerManager::DestroyDumper");
            return;
        }
    }
}

void MediaServerManager::DestroyDumperForPid(pid_t pid)
{
    for (auto &dumpers : dumperTbl_) {
        for (auto it = dumpers.second.begin(); it != dumpers.second.end();) {
            if (it->pid_ == pid) {
                it = dumpers.second.erase(it);
                MEDIA_LOGD("MediaServerManager::DestroyDumperForPid");
            } else {
                it++;
            }
        }
    }
}
} // namespace Media
} // namespace OHOS
