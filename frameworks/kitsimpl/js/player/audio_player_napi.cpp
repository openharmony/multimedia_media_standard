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

#include "audio_player_napi.h"
#include "player_callback_napi.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioPlayerNapi"};
}

namespace OHOS {
namespace Media {
napi_ref AudioPlayerNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "AudioPlayer";
const std::string STATE_PLAYING = "playing";
const std::string STATE_PAUSED = "paused";
const std::string STATE_STOPPED = "stopped";
const std::string STATE_IDLE = "idle";
const std::string STATE_ERROR = "error";
const std::string CALLBACK_ERROR_TYPE_SOURCE = "sourceError";
const std::string CALLBACK_ERROR_TYPE_CALLBACK = "callbackError";
const std::string CALLBACK_ERROR_TYPE_PREPARE = "prepareError";
const std::string CALLBACK_ERROR_TYPE_PLAY = "playError";
const std::string CALLBACK_ERROR_TYPE_PAUSE = "pauseError";
const std::string CALLBACK_ERROR_TYPE_SEEK = "seekError";
const std::string CALLBACK_ERROR_TYPE_VOLUME = "volumeError";
const std::string CALLBACK_ERROR_TYPE_STOP = "stopError";
const std::string CALLBACK_ERROR_TYPE_RESET = "resetError";
const std::string CALLBACK_ERROR_TYPE_RELEASE = "releaseError";
const std::string CALLBACK_ERROR_TYPE_ON = "onError";
const std::string CALLBACK_ERROR_TYPE_LOOP = "loopError";
const std::string CALLBACK_ERROR_TYPE_GETTIME = "getTimeError";
const std::string CALLBACK_ERROR_TYPE_GETDURATION = "getDurationError";

static std::string GetStringArgument(napi_env env, napi_value value)
{
    std::string strValue = "";
    size_t bufLength = 0;
    napi_status status = napi_get_value_string_utf8(env, value, nullptr, 0, &bufLength);
    if (status == napi_ok && bufLength > 0) {
        char *buffer = (char *)malloc((bufLength + 1) * sizeof(char));
        CHECK_AND_RETURN_RET_LOG(buffer != nullptr, strValue, "no memory");
        status = napi_get_value_string_utf8(env, value, buffer, bufLength + 1, &bufLength);
        if (status == napi_ok) {
            MEDIA_LOGD("argument = %{public}s", buffer);
            strValue = buffer;
        }
        free(buffer);
        buffer = nullptr;
    }
    return strValue;
}

AudioPlayerNapi::AudioPlayerNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AudioPlayerNapi::~AudioPlayerNapi()
{
    if (errorCallback_ != nullptr) {
        napi_delete_reference(env_, errorCallback_);
    }
    if (playCallback_ != nullptr) {
        napi_delete_reference(env_, playCallback_);
    }
    if (pauseCallback_ != nullptr) {
        napi_delete_reference(env_, pauseCallback_);
    }
    if (stopCallback_ != nullptr) {
        napi_delete_reference(env_, stopCallback_);
    }
    if (resetCallback_ != nullptr) {
        napi_delete_reference(env_, resetCallback_);
    }
    if (dataLoadCallback_ != nullptr) {
        napi_delete_reference(env_, dataLoadCallback_);
    }
    if (finishCallback_ != nullptr) {
        napi_delete_reference(env_, finishCallback_);
    }
    if (timeUpdateCallback_ != nullptr) {
        napi_delete_reference(env_, timeUpdateCallback_);
    }
    if (volumeChangeCallback_ != nullptr) {
        napi_delete_reference(env_, volumeChangeCallback_);
    }
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
    callbackNapi_ = nullptr;
    nativePlayer_ = nullptr;
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

napi_value AudioPlayerNapi::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("play", Play),
        DECLARE_NAPI_FUNCTION("pause", Pause),
        DECLARE_NAPI_FUNCTION("stop", Stop),
        DECLARE_NAPI_FUNCTION("reset", Reset),
        DECLARE_NAPI_FUNCTION("release", Release),
        DECLARE_NAPI_FUNCTION("seek", Seek),
        DECLARE_NAPI_FUNCTION("on", On),
        DECLARE_NAPI_FUNCTION("setVolume", SetVolume),

        DECLARE_NAPI_GETTER_SETTER("src", GetSrc, SetSrc),
        DECLARE_NAPI_GETTER_SETTER("loop", GetLoop, SetLoop),

        DECLARE_NAPI_GETTER("currentTime", GetCurrentTime),
        DECLARE_NAPI_GETTER("duration", GetDuration),
        DECLARE_NAPI_GETTER("state", GetState)
    };

    napi_property_descriptor static_prop[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createAudioPlayer", CreateAudioPlayer),
    };

    napi_value constructor = nullptr;
    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "define class fail");

    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "create reference fail");

    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "set named property fail");

    status = napi_define_properties(env, exports, sizeof(static_prop) / sizeof(static_prop[0]), static_prop);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "define properties fail");

    MEDIA_LOGD("Init success");
    return exports;
}

napi_value AudioPlayerNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_value jsThis = nullptr;
    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok) {
        napi_get_undefined(env, &result);
        MEDIA_LOGE("get callback info fail");
        return result;
    }

    AudioPlayerNapi *playerNapi = new(std::nothrow) AudioPlayerNapi();
    CHECK_AND_RETURN_RET_LOG(playerNapi != nullptr, nullptr, "no memory");

    playerNapi->env_ = env;
    playerNapi->nativePlayer_ = PlayerFactory::CreatePlayer();
    CHECK_AND_RETURN_RET_LOG(playerNapi->nativePlayer_ != nullptr, nullptr, "nativePlayer_ no memory");

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(playerNapi),
        AudioPlayerNapi::Destructor, nullptr, &(playerNapi->wrapper_));
    if (status != napi_ok) {
        napi_get_undefined(env, &result);
        delete playerNapi;
        MEDIA_LOGE("native wrap fail");
        return result;
    }

    MEDIA_LOGD("Constructor success");
    return jsThis;
}

void AudioPlayerNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    (void)env;
    (void)finalize;
    if (nativeObject != nullptr) {
        delete reinterpret_cast<AudioPlayerNapi *>(nativeObject);
    }
    MEDIA_LOGD("Destructor success");
}

napi_value AudioPlayerNapi::CreateAudioPlayer(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_value constructor = nullptr;
    napi_status status = napi_get_reference_value(env, constructor_, &constructor);
    if (status != napi_ok) {
        MEDIA_LOGE("get reference value fail");
        napi_get_undefined(env, &result);
        return result;
    }

    status = napi_new_instance(env, constructor, 0, nullptr, &result);
    if (status != napi_ok) {
        MEDIA_LOGE("new instance fail");
        napi_get_undefined(env, &result);
        return result;
    }

    MEDIA_LOGD("CreateAudioPlayer success");
    return result;
}

napi_value AudioPlayerNapi::SetSrc(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };

    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr || args[0] == nullptr) {
        MEDIA_LOGE("SetSrc fail to napi_get_cb_info");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "get player napi error");

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_string) {
        player->uri_.clear();
        SendErrorCallback(env, player->errorCallback_, "invalid arguments type", CALLBACK_ERROR_TYPE_SOURCE);
        return undefinedResult;
    }

    player->uri_ = GetStringArgument(env, args[0]);
    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "nativePlayer_ no memory");
    int32_t ret = player->nativePlayer_->SetSource(player->uri_);
    if (ret != MSERR_OK) {
        SendErrorCallback(env, player->errorCallback_, std::to_string(ret), CALLBACK_ERROR_TYPE_SOURCE);
        return undefinedResult;
    }

    if (player->callbackNapi_ == nullptr) {
        player->callbackNapi_ = std::make_shared<PlayerCallbackNapi>(env, *player);
        ret = player->nativePlayer_->SetPlayerCallback(player->callbackNapi_);
        if (ret != MSERR_OK) {
            SendErrorCallback(env, player->errorCallback_, std::to_string(ret), CALLBACK_ERROR_TYPE_CALLBACK);
            return undefinedResult;
        }
    }

    ret = player->nativePlayer_->PrepareAsync();
    if (ret != MSERR_OK) {
        SendErrorCallback(env, player->errorCallback_, std::to_string(ret), CALLBACK_ERROR_TYPE_PREPARE);
        return undefinedResult;
    }

    MEDIA_LOGD("SetSrc success");
    return undefinedResult;
}

napi_value AudioPlayerNapi::GetSrc(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    napi_value jsThis = nullptr;
    napi_value jsResult = nullptr;
    AudioPlayerNapi *player = nullptr;
    size_t argCount = 0;

    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("GetSrc fail to napi_get_cb_info");
        return undefinedResult;
    }

    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "get player napi error");

    if (player->uri_.empty()) {
        return undefinedResult;
    }

    status = napi_create_string_utf8(env, player->uri_.c_str(), NAPI_AUTO_LENGTH, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_create_string_utf8 error");

    MEDIA_LOGD("GetSrc success");
    return jsResult;
}

napi_value AudioPlayerNapi::Play(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("Play fail to napi_get_cb_info");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "get player napi error");

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "nativePlayer_ no memory");
    int32_t ret = player->nativePlayer_->Play();
    if (ret != MSERR_OK) {
        SendErrorCallback(env, player->errorCallback_, std::to_string(ret), CALLBACK_ERROR_TYPE_PLAY);
        return undefinedResult;
    }
    MEDIA_LOGD("Play success");
    return undefinedResult;
}

napi_value AudioPlayerNapi::Pause(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("Pause fail to napi_get_cb_info");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "get player napi error");

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "nativePlayer_ no memory");
    int32_t ret = player->nativePlayer_->Pause();
    if (ret != MSERR_OK) {
        SendErrorCallback(env, player->errorCallback_, std::to_string(ret), CALLBACK_ERROR_TYPE_PAUSE);
        return undefinedResult;
    }
    MEDIA_LOGD("Pause success");
    return undefinedResult;
}

napi_value AudioPlayerNapi::Stop(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("Stop fail to napi_get_cb_info");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "get player napi error");

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "nativePlayer_ no memory");
    int32_t ret = player->nativePlayer_->Stop();
    if (ret != MSERR_OK) {
        SendErrorCallback(env, player->errorCallback_, std::to_string(ret), CALLBACK_ERROR_TYPE_STOP);
        return undefinedResult;
    }
    MEDIA_LOGD("Stop success");
    return undefinedResult;
}

napi_value AudioPlayerNapi::Reset(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("AudioPlayerNapi Reset");
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("Reset fail to napi_get_cb_info");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "get player napi error");

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "nativePlayer_ no memory");
    int32_t ret = player->nativePlayer_->Reset();
    if (ret != MSERR_OK) {
        SendErrorCallback(env, player->errorCallback_, std::to_string(ret), CALLBACK_ERROR_TYPE_RESET);
        return undefinedResult;
    }
    MEDIA_LOGD("Reset success");
    return undefinedResult;
}

napi_value AudioPlayerNapi::Release(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("Release fail to napi_get_cb_info");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "get player napi error");

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "nativePlayer_ no memory");
    int32_t ret = player->nativePlayer_->Release();
    if (ret != MSERR_OK) {
        SendErrorCallback(env, player->errorCallback_, std::to_string(ret), CALLBACK_ERROR_TYPE_RELEASE);
        return undefinedResult;
    }
    player->callbackNapi_ = nullptr;
    player->uri_.clear();
    player->currentState_ = PLAYER_IDLE;
    player->isRelease = true;

    MEDIA_LOGD("Release success");
    return undefinedResult;
}

napi_value AudioPlayerNapi::Seek(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 1;
    napi_value args[1] = { nullptr };
    napi_value jsThis = nullptr;

    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr || args[0] == nullptr) {
        MEDIA_LOGE("Seek fail to napi_get_cb_info");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "get player napi error");

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        SendErrorCallback(env, player->errorCallback_, "invalid arguments type", CALLBACK_ERROR_TYPE_SEEK);
        return undefinedResult;
    }

    int32_t position = -1;
    status = napi_get_value_int32(env, args[0], &position);
    if (status != napi_ok || position < 0) {
        SendErrorCallback(env, player->errorCallback_, "invalid position value", CALLBACK_ERROR_TYPE_SEEK);
        return undefinedResult;
    }

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "nativePlayer_ no memory");
    int32_t ret = player->nativePlayer_->Seek(position, SEEK_PREVIOUS_SYNC);
    if (ret != MSERR_OK) {
        SendErrorCallback(env, player->errorCallback_, std::to_string(ret), CALLBACK_ERROR_TYPE_SEEK);
        return undefinedResult;
    }

    MEDIA_LOGD("Seek success");
    return undefinedResult;
}

napi_value AudioPlayerNapi::SetVolume(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 1;
    napi_value args[1] = { nullptr };
    napi_value jsThis = nullptr;

    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr || args[0] == nullptr) {
        MEDIA_LOGE("SetVolume fail to napi_get_cb_info");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "get player napi error");

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        SendErrorCallback(env, player->errorCallback_, "invalid arguments type", CALLBACK_ERROR_TYPE_VOLUME);
        return undefinedResult;
    }

    double volumeLevel;
    status = napi_get_value_double(env, args[0], &volumeLevel);
    if (status != napi_ok || volumeLevel < 0.0f || volumeLevel > 1.0f) {
        SendErrorCallback(env, player->errorCallback_, "invalid volumeLevel value", CALLBACK_ERROR_TYPE_VOLUME);
        return undefinedResult;
    }

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "nativePlayer_ no memory");
    int32_t ret = player->nativePlayer_->SetVolume(static_cast<float>(volumeLevel), static_cast<float>(volumeLevel));
    if (ret != MSERR_OK) {
        SendErrorCallback(env, player->errorCallback_, std::to_string(ret), CALLBACK_ERROR_TYPE_VOLUME);
        return undefinedResult;
    }
    MEDIA_LOGD("SetVolume success");
    return undefinedResult;
}

void AudioPlayerNapi::SetCurrentState(PlayerStates state)
{
    currentState_ = state;
}

napi_value AudioPlayerNapi::On(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    static const size_t MIN_REQUIRED_ARG_COUNT = 2;
    size_t argCount = MIN_REQUIRED_ARG_COUNT;
    napi_value args[MIN_REQUIRED_ARG_COUNT] = { nullptr, nullptr };
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr || args[0] == nullptr || args[1] == nullptr) {
        MEDIA_LOGE("On fail to napi_get_cb_info");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "get player napi error");

    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string ||
        napi_typeof(env, args[1], &valueType1) != napi_ok || valueType1 != napi_function) {
        SendErrorCallback(env, player->errorCallback_, "invalid arguments type", CALLBACK_ERROR_TYPE_ON);
        return undefinedResult;
    }

    std::string callbackName = GetStringArgument(env, args[0]);
    MEDIA_LOGD("callbackName: %{public}s", callbackName.c_str());
    player->SaveCallbackReference(env, *player, callbackName, args[1]);
    return undefinedResult;
}

napi_value AudioPlayerNapi::SetLoop(napi_env env, napi_callback_info info)
{
    size_t argCount = 1;
    napi_value args[1] = { nullptr };
    napi_value jsThis = nullptr;
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr || args[0] == nullptr) {
        MEDIA_LOGE("SetLoop fail to napi_get_cb_info");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "get player napi error");

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_boolean) {
        SendErrorCallback(env, player->errorCallback_, "invalid arguments type", CALLBACK_ERROR_TYPE_LOOP);
        return undefinedResult;
    }

    bool loopFlag = false;
    status = napi_get_value_bool(env, args[0], &loopFlag);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_get_value_bool error");

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "nativePlayer_ no memory");
    int32_t ret = player->nativePlayer_->SetLooping(loopFlag);
    if (ret != MSERR_OK) {
        SendErrorCallback(env, player->errorCallback_, "Failed to enable/disable looping", CALLBACK_ERROR_TYPE_LOOP);
        return undefinedResult;
    }

    MEDIA_LOGD("SetLoop success");
    return undefinedResult;
}


napi_value AudioPlayerNapi::GetLoop(napi_env env, napi_callback_info info)
{
    napi_value jsThis = nullptr;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("GetLoop fail to napi_get_cb_info");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "get player napi error");

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "nativePlayer_ no memory");
    bool loopFlag = player->nativePlayer_->IsLooping();

    status = napi_get_boolean(env, loopFlag, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_get_boolean error");

    MEDIA_LOGD("GetSrc success loop Status: %{public}d", loopFlag);
    return jsResult;
}

napi_value AudioPlayerNapi::GetCurrentTime(napi_env env, napi_callback_info info)
{
    napi_value jsThis = nullptr;
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("GetCurrentTime fail to napi_get_cb_info");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "get player napi error");

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "nativePlayer_ no memory");
    int32_t currentTime = -1;
    int32_t ret = player->nativePlayer_->GetCurrentTime(currentTime);
    if (ret != MSERR_OK || currentTime < 0) {
        SendErrorCallback(env, player->errorCallback_, std::to_string(ret), CALLBACK_ERROR_TYPE_GETTIME);
        return undefinedResult;
    }

    napi_value jsResult = nullptr;
    status = napi_create_int32(env, currentTime, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_create_int32 error");

    MEDIA_LOGD("GetCurrenTime success, Current time: %{public}d", currentTime);
    return jsResult;
}

napi_value AudioPlayerNapi::GetDuration(napi_env env, napi_callback_info info)
{
    napi_value jsThis = nullptr;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("GetDuration fail to napi_get_cb_info");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "get player napi error");

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "nativePlayer_ no memory");
    int32_t duration = -1;
    int32_t ret = player->nativePlayer_->GetDuration(duration);
    if (ret != MSERR_OK || duration < 0) {
        SendErrorCallback(env, player->errorCallback_, std::to_string(ret), CALLBACK_ERROR_TYPE_GETDURATION);
        return undefinedResult;
    }

    status = napi_create_int32(env, duration, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_create_int64 error");

    MEDIA_LOGD("GetDuration success, Current time: %{public}d", duration);
    return jsResult;
}

static std::string GetJSState(PlayerStates currentState)
{
    std::string result;

    MEDIA_LOGD("GetJSState()! is called!, %{public}d", currentState);
    switch (currentState) {
        case PLAYER_STOPPED:
        case PLAYER_PLAYBACK_COMPLETE:
            result = STATE_STOPPED;
            break;
        case PLAYER_IDLE:
            result = STATE_IDLE;
            break;
        case PLAYER_INITIALIZED:
            result = STATE_IDLE;
            break;
        case PLAYER_PAUSED:
        case PLAYER_PREPARED:
            result = STATE_PAUSED;
            break;
        case PLAYER_STARTED:
            result = STATE_PLAYING;
            break;

        default:
            // Considering default state as stopped
            MEDIA_LOGE("Unknown state!, %{public}d", currentState);
            result = STATE_ERROR;
            break;
    }
    return result;
}

napi_value AudioPlayerNapi::GetState(napi_env env, napi_callback_info info)
{
    napi_value jsThis = nullptr;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    std::string curState;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_get_cb_info error");

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, (void **)&player);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_unwrap error");

    if (player->isRelease) {
        return undefinedResult;
    } else {
        curState = GetJSState(player->currentState_);
    }
    status = napi_create_string_utf8(env, curState.c_str(), NAPI_AUTO_LENGTH, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_create_string_utf8 error");

    MEDIA_LOGD("GetState success, State: %{public}d", player->currentState_);
    return jsResult;
}

void AudioPlayerNapi::SaveCallbackReference(napi_env env, AudioPlayerNapi &playerNapi,
    const std::string &callbackName, napi_value callback) const
{
    napi_ref *ref = nullptr;
    if (callbackName == PLAY_CALLBACK_NAME) {
        ref = &(playerNapi.playCallback_);
    } else if (callbackName == PAUSE_CALLBACK_NAME) {
        ref = &(playerNapi.pauseCallback_);
    } else if (callbackName == STOP_CALLBACK_NAME) {
        ref = &(playerNapi.stopCallback_);
    } else if (callbackName == RESET_CALLBACK_NAME) {
        ref = &(playerNapi.resetCallback_);
    } else if (callbackName == DATA_LOAD_CALLBACK_NAME) {
        ref = &(playerNapi.dataLoadCallback_);
    } else if (callbackName == FINISH_CALLBACK_NAME) {
        ref = &(playerNapi.finishCallback_);
    } else if (callbackName == TIME_UPDATE_CALLBACK_NAME) {
        ref = &(playerNapi.timeUpdateCallback_);
    } else if (callbackName == ERROR_CALLBACK_NAME) {
        ref = &(playerNapi.errorCallback_);
    } else if (callbackName == VOL_CHANGE_CALLBACK_NAME) {
        ref = &(playerNapi.volumeChangeCallback_);
    } else {
        MEDIA_LOGE("unknown callback: %{public}s", callbackName.c_str());
        return;
    }

    if (*ref != nullptr) {
        uint32_t thisRefCount = 0;
        napi_reference_unref(env, *ref, &thisRefCount);
        *ref = nullptr; // JS clears the callback after each on.
    }

    const int32_t refCount = 1;
    napi_status status = napi_create_reference(env, callback, refCount, ref);
    CHECK_AND_RETURN_LOG(status == napi_ok, "creating reference for callback fail")
}

void AudioPlayerNapi::SendErrorCallback(napi_env env, napi_ref &callbackRef,
    const std::string &errCode, const std::string &errType)
{
    MEDIA_LOGE("fail to %{public}s, code = %{public}s", errType.c_str(), errCode.c_str());
    CHECK_AND_RETURN_LOG(callbackRef != nullptr, "no callback reference");

    napi_value jsCallback = nullptr;
    napi_status status = napi_get_reference_value(env, callbackRef, &jsCallback);
    CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr, "get reference value fail");

    napi_value errorTypeVal = nullptr;
    status = napi_create_string_utf8(env, errType.c_str(), NAPI_AUTO_LENGTH, &errorTypeVal);
    CHECK_AND_RETURN_LOG(status == napi_ok && errorTypeVal != nullptr, "get error type value fail");

    napi_value errorCodeVal = nullptr;
    status = napi_create_string_utf8(env, errCode.c_str(), NAPI_AUTO_LENGTH, &errorCodeVal);
    CHECK_AND_RETURN_LOG(status == napi_ok && errorCodeVal != nullptr, "get error code value fail");

    napi_value args[1] = { nullptr };
    status = napi_create_error(env, errorCodeVal, errorTypeVal, &args[0]);
    CHECK_AND_RETURN_LOG(status == napi_ok && args[0] != nullptr, "create error callback fail");

    const size_t argCount = 1;
    napi_value result = nullptr;
    status = napi_call_function(env, nullptr, jsCallback, argCount, args, &result);
    CHECK_AND_RETURN_LOG(status == napi_ok, "call error callback fail");
}
}  // namespace Media
}  // namespace OHOS