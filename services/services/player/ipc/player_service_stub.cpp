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

#include "player_service_stub.h"
#include "player_listener_proxy.h"
#include "media_server_manager.h"
#include "media_log.h"
#include "errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerServiceStub"};
}

namespace OHOS {
namespace Media {
sptr<PlayerServiceStub> PlayerServiceStub::Create()
{
    sptr<PlayerServiceStub> playerStub = new(std::nothrow) PlayerServiceStub();
    CHECK_AND_RETURN_RET_LOG(playerStub != nullptr, nullptr, "failed to new PlayerServiceStub");

    int32_t ret = playerStub->Init();
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, nullptr, "failed to player stub init");
    return playerStub;
}

PlayerServiceStub::PlayerServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayerServiceStub::~PlayerServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destory", FAKE_POINTER(this));
}

int32_t PlayerServiceStub::Init()
{
    playerServer_ = PlayerServer::Create();
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, ERR_NO_INIT, "failed to create PlayerServer");

    playerFuncs_[SET_LISTENER_OBJ] = &PlayerServiceStub::SetListenerObject;
    playerFuncs_[SET_SOURCE] = &PlayerServiceStub::SetSource;
    playerFuncs_[PLAY] = &PlayerServiceStub::Play;
    playerFuncs_[PREPARE] = &PlayerServiceStub::Prepare;
    playerFuncs_[PREPAREASYNC] = &PlayerServiceStub::PrepareAsync;
    playerFuncs_[PAUSE] = &PlayerServiceStub::Pause;
    playerFuncs_[STOP] = &PlayerServiceStub::Stop;
    playerFuncs_[RESET] = &PlayerServiceStub::Reset;
    playerFuncs_[RELEASE] = &PlayerServiceStub::Release;
    playerFuncs_[SET_VOLUME] = &PlayerServiceStub::SetVolume;
    playerFuncs_[SEEK] = &PlayerServiceStub::Seek;
    playerFuncs_[GET_CURRENT_TIME] = &PlayerServiceStub::GetCurrentTime;
    playerFuncs_[GET_DURATION] = &PlayerServiceStub::GetDuration;
    playerFuncs_[SET_PLAYERBACK_SPEED] = &PlayerServiceStub::SetPlaybackSpeed;
    playerFuncs_[GET_PLAYERBACK_SPEED] = &PlayerServiceStub::GetPlaybackSpeed;
    playerFuncs_[SET_VIDEO_SURFACE] = &PlayerServiceStub::SetVideoSurface;
    playerFuncs_[IS_PLAYING] = &PlayerServiceStub::IsPlaying;
    playerFuncs_[IS_LOOPING] = &PlayerServiceStub::IsLooping;
    playerFuncs_[SET_LOOPING] = &PlayerServiceStub::SetLooping;
    playerFuncs_[DESTROY] = &PlayerServiceStub::DestroyStub;
    playerFuncs_[SET_CALLBACK] = &PlayerServiceStub::SetPlayerCallback;
    return ERR_OK;
}

int32_t PlayerServiceStub::DestroyStub()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (deathRecipient_ != nullptr) {
        deathRecipient_->SetNotifyCb(nullptr);
    }
    deathRecipient_ = nullptr;
    playerCallback_ = nullptr;
    playerServer_ = nullptr;

    MediaServerManager::GetInstance().DestroyStubObject(MediaServerManager::PLAYER, AsObject());
    return ERR_OK;
}

void PlayerServiceStub::ClientDied()
{
    MEDIA_LOGE("player client pid is dead");
    (void)DestroyStub();
}

int PlayerServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    MEDIA_LOGI("Stub: OnRemoteRequest of code: %{public}d is received", code);

    auto itFunc = playerFuncs_.find(code);
    if (itFunc != playerFuncs_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            int32_t ret = (this->*memberFunc)(data, reply);
            if (ret != ERR_OK) {
                MEDIA_LOGE("calling memberFunc is failed.");
            }
            return ERR_OK;
        }
    }
    MEDIA_LOGW("PlayerServiceStub: no member func supporting, applying default process");

    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int32_t PlayerServiceStub::SetListenerObject(const sptr<IRemoteObject> &object)
{
    CHECK_AND_RETURN_RET_LOG(object != nullptr, ERR_INVALID_VALUE, "set listener object is nullptr");

    sptr<IStandardPlayerListener> listener = iface_cast<IStandardPlayerListener>(object);
    CHECK_AND_RETURN_RET_LOG(listener != nullptr, ERR_INVALID_VALUE, "failed to convert IStandardPlayerListener");

    deathRecipient_ = new(std::nothrow) MediaDeathRecipient();
    CHECK_AND_RETURN_RET_LOG(deathRecipient_ != nullptr, ERR_NO_MEMORY, "failed to new MediaDeathRecipient");

    deathRecipient_->SetNotifyCb(std::bind(&PlayerServiceStub::ClientDied, this));

    std::shared_ptr<PlayerCallback> callback = std::make_shared<PlayerListenerCallback>(listener, deathRecipient_);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, ERR_NO_MEMORY, "failed to new PlayerListenerCallback");

    playerCallback_ = callback;
    return ERR_OK;
}

int32_t PlayerServiceStub::SetSource(const std::string &uri)
{
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, ERR_INVALID_OPERATION, "player server is nullptr");
    return playerServer_->SetSource(uri);
}

int32_t PlayerServiceStub::Play()
{
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, ERR_INVALID_OPERATION, "player server is nullptr");
    return playerServer_->Play();
}

int32_t PlayerServiceStub::Prepare()
{
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, ERR_INVALID_OPERATION, "player server is nullptr");
    return playerServer_->Prepare();
}

int32_t PlayerServiceStub::PrepareAsync()
{
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, ERR_INVALID_OPERATION, "player server is nullptr");
    return playerServer_->PrepareAsync();
}

int32_t PlayerServiceStub::Pause()
{
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, ERR_INVALID_OPERATION, "player server is nullptr");
    return playerServer_->Pause();
}

int32_t PlayerServiceStub::Stop()
{
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, ERR_INVALID_OPERATION, "player server is nullptr");
    return playerServer_->Stop();
}

int32_t PlayerServiceStub::Reset()
{
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, ERR_INVALID_OPERATION, "player server is nullptr");
    return playerServer_->Reset();
}

int32_t PlayerServiceStub::Release()
{
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, ERR_INVALID_OPERATION, "player server is nullptr");
    return playerServer_->Release();
}

int32_t PlayerServiceStub::SetVolume(float leftVolume, float rightVolume)
{
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, ERR_INVALID_OPERATION, "player server is nullptr");
    return playerServer_->SetVolume(leftVolume, rightVolume);
}

int32_t PlayerServiceStub::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, ERR_INVALID_OPERATION, "player server is nullptr");
    return playerServer_->Seek(mSeconds, mode);
}

int32_t PlayerServiceStub::GetCurrentTime(int32_t &currentTime)
{
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, ERR_INVALID_OPERATION, "player server is nullptr");
    return playerServer_->GetCurrentTime(currentTime);
}

int32_t PlayerServiceStub::GetDuration(int32_t &duration)
{
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, ERR_INVALID_OPERATION, "player server is nullptr");
    return playerServer_->GetDuration(duration);
}

int32_t PlayerServiceStub::SetPlaybackSpeed(PlaybackRateMode mode)
{
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, ERR_INVALID_OPERATION, "player server is nullptr");
    return playerServer_->SetPlaybackSpeed(mode);
}

int32_t PlayerServiceStub::GetPlaybackSpeed(PlaybackRateMode &mode)
{
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, ERR_INVALID_OPERATION, "player server is nullptr");
    return playerServer_->GetPlaybackSpeed(mode);
}

int32_t PlayerServiceStub::SetVideoSurface(sptr<Surface> surface)
{
    MEDIA_LOGD("SetVideoSurface");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, ERR_INVALID_OPERATION, "player server is nullptr");
    return playerServer_->SetVideoSurface(surface);
}

bool PlayerServiceStub::IsPlaying()
{
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, ERR_INVALID_OPERATION, "player server is nullptr");
    return playerServer_->IsPlaying();
}

bool PlayerServiceStub::IsLooping()
{
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, ERR_INVALID_OPERATION, "player server is nullptr");
    return playerServer_->IsLooping();
}

int32_t PlayerServiceStub::SetLooping(bool loop)
{
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, ERR_INVALID_OPERATION, "player server is nullptr");
    return playerServer_->SetLooping(loop);
}

int32_t PlayerServiceStub::SetPlayerCallback()
{
    MEDIA_LOGD("SetPlayerCallback");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, ERR_INVALID_OPERATION, "player server is nullptr");
    return playerServer_->SetPlayerCallback(playerCallback_);
}

int32_t PlayerServiceStub::SetListenerObject(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    int32_t ret = SetListenerObject(object);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t PlayerServiceStub::SetSource(MessageParcel &data, MessageParcel &reply)
{
    std::string uri = data.ReadString();
    int32_t ret = SetSource(uri);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t PlayerServiceStub::Play(MessageParcel &data, MessageParcel &reply)
{
    int32_t ret = Play();
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t PlayerServiceStub::Prepare(MessageParcel &data, MessageParcel &reply)
{
    int32_t ret = Prepare();
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t PlayerServiceStub::PrepareAsync(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    int32_t ret = PrepareAsync();
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t PlayerServiceStub::Pause(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    int32_t ret = Pause();
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t PlayerServiceStub::Stop(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    int32_t ret = Stop();
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t PlayerServiceStub::Reset(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    int32_t ret = Reset();
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t PlayerServiceStub::Release(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    int32_t ret = Release();
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t PlayerServiceStub::SetVolume(MessageParcel &data, MessageParcel &reply)
{
    float leftVolume = data.ReadFloat();
    float rightVolume = data.ReadFloat();
    int32_t ret = SetVolume(leftVolume, rightVolume);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t PlayerServiceStub::Seek(MessageParcel &data, MessageParcel &reply)
{
    int32_t mSeconds = data.ReadInt32();
    int32_t mode = data.ReadInt32();
    int32_t ret = Seek(mSeconds, static_cast<PlayerSeekMode>(mode));
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t PlayerServiceStub::GetCurrentTime(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    int32_t currentTime = 0;
    int32_t ret = GetCurrentTime(currentTime);
    reply.WriteInt32(currentTime);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t PlayerServiceStub::GetDuration(MessageParcel &data, MessageParcel &reply)
{
    int32_t duration = 0;
    int32_t ret = GetDuration(duration);
    reply.WriteInt32(duration);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t PlayerServiceStub::SetPlaybackSpeed(MessageParcel &data, MessageParcel &reply)
{
    int32_t mode = data.ReadInt32();
    int32_t ret = SetPlaybackSpeed(static_cast<PlaybackRateMode>(mode));
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t PlayerServiceStub::GetPlaybackSpeed(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    PlaybackRateMode mode = SPEED_FORWARD_1_00_X;
    int32_t ret = GetPlaybackSpeed(mode);
    reply.WriteInt32(mode);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t PlayerServiceStub::SetVideoSurface(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, ERR_INVALID_VALUE, "object is nullptr");

    sptr<IBufferProducer> producer = iface_cast<IBufferProducer>(object);
    CHECK_AND_RETURN_RET_LOG(producer != nullptr, ERR_INVALID_VALUE, "failed to convert object to producer");

    sptr<Surface> surface = Surface::CreateSurfaceAsProducer(producer);
    CHECK_AND_RETURN_RET_LOG(surface != nullptr, ERR_INVALID_VALUE, "failed to create surface");

    std::string format = data.ReadString();
    MEDIA_LOGI("surfaceFormat is %{public}s!", format.c_str());
    const std::string surfaceFormat = "SURFACE_FORMAT";
    (void)surface->SetUserData(surfaceFormat, format);
    int32_t ret = SetVideoSurface(surface);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t PlayerServiceStub::IsPlaying(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    bool isPlay = IsPlaying();
    reply.WriteBool(isPlay);
    return ERR_OK;
}

int32_t PlayerServiceStub::IsLooping(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    bool isLoop = IsLooping();
    reply.WriteBool(isLoop);
    return ERR_OK;
}

int32_t PlayerServiceStub::SetLooping(MessageParcel &data, MessageParcel &reply)
{
    bool loop = data.ReadBool();
    int32_t ret = SetLooping(loop);
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t PlayerServiceStub::DestroyStub(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    int32_t ret = DestroyStub();
    reply.WriteInt32(ret);
    return ERR_OK;
}

int32_t PlayerServiceStub::SetPlayerCallback(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    int32_t ret = SetPlayerCallback();
    reply.WriteInt32(ret);
    return ERR_OK;
}
} // namespace Media
} // namespace OHOS
