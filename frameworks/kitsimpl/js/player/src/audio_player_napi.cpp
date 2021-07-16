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
#include "errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioPlayerNapi"};
}

namespace OHOS {
namespace Media {
napi_ref AudioPlayerNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "AudioPlayer";
const std::string PLAY_CALLBACK_NAME = "play";
const std::string PAUSE_CALLBACK_NAME = "pause";
const std::string STOP_CALLBACK_NAME = "stop";
const std::string RESET_CALLBACK_NAME = "reset";
const std::string DATA_LOAD_CALLBACK_NAME = "dataLoad";
const std::string FINISH_CALLBACK_NAME = "finish";
const std::string TIME_UPDATE_CALLBACK_NAME = "timeUpdate";
const std::string ERROR_CALLBACK_NAME = "error";
const std::string VOL_CHANGE_CALLBACK_NAME = "volumeChange";

const std::string STATE_PLAYING = "playing";
const std::string STATE_PAUSED = "paused";
const std::string STATE_STOPPED = "stopped";
const std::string STATE_IDLE = "idle";

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

napi_value AudioPlayerNapi::Play(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || argCount != 0) {
        MEDIA_LOGE("fail to Play");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, (void**) &player);
    if (status != napi_ok || argCount != 0) {
        MEDIA_LOGE("fail to Play");
        return undefinedResult;
    }

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "nativePlayer_ no memory");
    if (player->nativePlayer_->Play() != 0) {
        MEDIA_LOGE("fail to Play");
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
    if (status != napi_ok || argCount != 0) {
        MEDIA_LOGE("fail to Pause");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, (void**) &player);
    if (status != napi_ok || argCount != 0) {
        MEDIA_LOGE("fail to Pause");
        return undefinedResult;
    }

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "nativePlayer_ no memory");
    if (player->nativePlayer_->Pause() != 0) {
        MEDIA_LOGE("fail to Pause");
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
    if (status != napi_ok || argCount != 0) {
        MEDIA_LOGE("fail to Stop");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, (void**) &player);
    if (status != napi_ok || argCount != 0) {
        MEDIA_LOGE("fail to Stop");
        return undefinedResult;
    }

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "nativePlayer_ no memory");
    if (player->nativePlayer_->Stop() != 0) {
        MEDIA_LOGE("fail to Stop");
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
    if (status != napi_ok || argCount != 0) {
        MEDIA_LOGE("fail to Reset");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, (void**) &player);
    if (status != napi_ok || argCount != 0) {
        MEDIA_LOGE("fail to Reset");
        return undefinedResult;
    }

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "nativePlayer_ no memory");
    if (player->nativePlayer_->Reset() != 0) {
        MEDIA_LOGE("fail to Reset");
    }
    return undefinedResult;
}

napi_value AudioPlayerNapi::Release(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || argCount != 0) {
        MEDIA_LOGE("fail to Release");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, (void**) &player);
    if (status != napi_ok || argCount != 0) {
        MEDIA_LOGE("fail to Release");
        return undefinedResult;
    }

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "nativePlayer_ no memory");
    if (player->nativePlayer_->Release() != 0) {
        MEDIA_LOGE("fail to Release");
    }
    MEDIA_LOGD("Release success");
    return undefinedResult;
}

napi_value AudioPlayerNapi::Seek(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 1;
    napi_value result = nullptr;
    napi_value args[1] = { nullptr };
    napi_value jsThis = nullptr;
    napi_value jsCallback = nullptr;
    int64_t position = -1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && argCount >= 1, nullptr, "fail to Seek");

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, (void**) &player);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "fail to Seek");

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        MEDIA_LOGE("invalid arguments type");
        return undefinedResult;
    }

    status = napi_get_value_int64(env, args[0], &position);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "fail to Seek");
    CHECK_AND_RETURN_RET_LOG(position >= 0, undefinedResult, "fail to Seek position(%{public}lld)", position);

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "nativePlayer_ no memory");
    if (player->nativePlayer_->Seek(static_cast<uint64_t>(position), 0) != 0) {
        MEDIA_LOGE("fail to Seek");
        return undefinedResult;
    }

    if (player->timeUpdateCallback_ == nullptr) {
        MEDIA_LOGE("fail to Seek");
        return undefinedResult;
    }

    status = napi_get_reference_value(env, player->timeUpdateCallback_, &jsCallback);
    if (status != napi_ok && jsCallback == nullptr) {
        MEDIA_LOGE("fail to Seek");
        return undefinedResult;
    }

    status = napi_call_function(env, nullptr, jsCallback, 0, nullptr, &result);
    if (status != napi_ok) {
        MEDIA_LOGE("fail to Seek");
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
    napi_value result = nullptr;
    napi_value args[1] = { nullptr };
    napi_value jsThis = nullptr;
    napi_value jsCallback = nullptr;
    double volumeLevel;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && argCount >= 1, nullptr, "fail to SetVolume");

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, (void**) &player);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "fail to SetVolume");

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        MEDIA_LOGE("invalid arguments type");
        return undefinedResult;
    }

    status = napi_get_value_double(env, args[0], &volumeLevel);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "fail to SetVolume");

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "nativePlayer_ no memory");
    if (player->nativePlayer_->SetVolume(static_cast<float>(volumeLevel), static_cast<float>(volumeLevel)) != 0) {
        MEDIA_LOGE("fail to SetVolume");
        return undefinedResult;
    }

    if (player->volumeChangeCallback_ == nullptr) {
        MEDIA_LOGE("fail to SetVolume");
        return undefinedResult;
    }

    status = napi_get_reference_value(env, player->volumeChangeCallback_, &jsCallback);
    if (status != napi_ok && jsCallback == nullptr) {
        MEDIA_LOGE("fail to SetVolume");
        return undefinedResult;
    }

    status = napi_call_function(env, nullptr, jsCallback, 0, nullptr, &result);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "fail to SetVolume");

    MEDIA_LOGD("SetVolume success");
    return undefinedResult;
}

void AudioPlayerNapi::SetCurrentState(int32_t state)
{
    currentState_ = state;
}

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

napi_value AudioPlayerNapi::On(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 2;
    napi_value args[2] = { nullptr };
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && argCount >= 2, nullptr, "invalid argument");

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, (void **)&player);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "set callback fail");

    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string
        || napi_typeof(env, args[1], &valueType1) != napi_ok || valueType1 != napi_function) {
        MEDIA_LOGE("invalid arguments type");
        return undefinedResult;
    }

    std::string callbackName = GetStringArgument(env, args[0]);
    MEDIA_LOGD("callbackName: %{public}s", callbackName.c_str());
    player->SaveCallbackReference(env, *player, callbackName, args[1]);
    return undefinedResult;
}

napi_value AudioPlayerNapi::SetSrc(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };

    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || argCount < 1) {
        MEDIA_LOGE("fail to Seek");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, (void **)&player);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_unwrap error");

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_string) {
        player->uri_ = nullptr;
        MEDIA_LOGE("invalid arguments type");
        return undefinedResult;
    }

    player->uri_ = GetStringArgument(env, args[0]);
    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "nativePlayer_ no memory");
    if (player->nativePlayer_->SetSource(player->uri_) != 0) {
        MEDIA_LOGE("fail to SetSrc");
        return undefinedResult;
    }

    player->callbackNapi_ = std::make_shared<PlayerCallbackNapi>(env, *player);
    if (player->nativePlayer_->SetPlayerCallback(player->callbackNapi_) != 0) {
        MEDIA_LOGE("fail to SetPlayerCallback");
        return undefinedResult;
    }

    if (player->nativePlayer_->PrepareAsync() != 0) {
        MEDIA_LOGE("fail to Prepare");
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
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_get_cb_info error");

    status = napi_unwrap(env, jsThis, (void **)&player);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_unwrap error");

    if (player->uri_.length() == 0) {
        MEDIA_LOGE("empty uri");
        return undefinedResult;
    }

    status = napi_create_string_utf8(env, player->uri_.c_str(), NAPI_AUTO_LENGTH, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_create_string_utf8 error");

    MEDIA_LOGD("GetSrc success");
    return jsResult;
}

napi_value AudioPlayerNapi::SetLoop(napi_env env, napi_callback_info info)
{
    bool loopFlag = false;
    size_t argCount = 1;
    napi_value args[1] = { nullptr };
    napi_value jsThis = nullptr;
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || argCount < 1) {
        MEDIA_LOGE("set callback fail");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, (void **)&player);
    if (status != napi_ok) {
        MEDIA_LOGE("set callback fail");
        return undefinedResult;
    }

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_boolean) {
        MEDIA_LOGE("invalid arguments type");
        return undefinedResult;
    }

    status = napi_get_value_bool(env, args[0], &loopFlag);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_get_value_bool error");

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "nativePlayer_ no memory");
    if (player->nativePlayer_->SetLooping(loopFlag) != 0) {
        MEDIA_LOGE("Failed to enable/disable looping");
        return undefinedResult;
    }

    MEDIA_LOGD("SetLoop success");
    return undefinedResult;
}


napi_value AudioPlayerNapi::GetLoop(napi_env env, napi_callback_info info)
{
    bool loopFlag = false;
    napi_value jsThis = nullptr;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_get_cb_info error");

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, (void **)&player);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_unwrap error");

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "nativePlayer_ no memory");
    loopFlag = player->nativePlayer_->IsLooping();

    status = napi_get_boolean(env, loopFlag, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_get_boolean error");

    MEDIA_LOGD("GetSrc success loop Status: %{public}d", loopFlag);
    return jsResult;
}

napi_value AudioPlayerNapi::GetCurrentTime(napi_env env, napi_callback_info info)
{
    napi_value jsThis = nullptr;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    uint64_t currentTime;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_get_cb_info error");

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, (void **)&player);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_unwrap error");

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "nativePlayer_ no memory");
    if (player->nativePlayer_->GetCurrentTime(currentTime) != 0) {
        MEDIA_LOGE("GetCurrenTime fail");
        return undefinedResult;
    }

    int64_t napiTime = static_cast<int64_t>(currentTime);
    status = napi_create_int64(env, napiTime, &jsResult);
    if (status != napi_ok) {
        MEDIA_LOGE("GetCurrenTime fail");
        return undefinedResult;
    }

    MEDIA_LOGD("GetCurrenTime success, Current time: %{public}llu", currentTime);
    return jsResult;
}

napi_value AudioPlayerNapi::GetDuration(napi_env env, napi_callback_info info)
{
    napi_value jsThis = nullptr;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    uint64_t duration;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_get_cb_info error");

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, (void **)&player);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_unwrap error");

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "nativePlayer_ no memory");
    if (player->nativePlayer_->GetDuration(duration) != ERR_OK) {
        MEDIA_LOGE("GetDuration fail");
        return undefinedResult;
    }

    int64_t napiTime = static_cast<int64_t>(duration);
    status = napi_create_int64(env, napiTime, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_create_int64 error");

    MEDIA_LOGD("GetDuration success, Current time: %{public}llu", duration);
    return jsResult;
}

static std::string GetJSState(int32_t currentState)
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
            result = STATE_STOPPED;
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

    curState = GetJSState(player->currentState_);
    status = napi_create_string_utf8(env, curState.c_str(), NAPI_AUTO_LENGTH, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_create_string_utf8 error");

    MEDIA_LOGD("GetState success, State: %{public}d", status);
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

    int32_t refCount = 1;
    napi_status status = napi_create_reference(env, callback, refCount, ref);
    CHECK_AND_RETURN_LOG(status == napi_ok, "creating reference for callback fail")
}
}  // namespace Media
}  // namespace OHOS
