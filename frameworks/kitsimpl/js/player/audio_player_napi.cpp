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
#include <climits>
#include "player_callback_napi.h"
#include "media_log.h"
#include "media_errors.h"
#include "media_data_source_napi.h"
#include "media_data_source_callback.h"
#include "common_napi.h"

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

AudioPlayerNapi::AudioPlayerNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AudioPlayerNapi::~AudioPlayerNapi()
{
    callbackNapi_ = nullptr;
    nativePlayer_ = nullptr;
    dataSrcCallBack_ = nullptr;
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
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
        DECLARE_NAPI_FUNCTION("getTrackDescription", GetTrackDescription),

        DECLARE_NAPI_GETTER_SETTER("src", GetSrc, SetSrc),
        DECLARE_NAPI_GETTER_SETTER("mediaDataSrc", GetMediaDataSrc, SetMediaDataSrc),
        DECLARE_NAPI_GETTER_SETTER("loop", GetLoop, SetLoop),

        DECLARE_NAPI_GETTER("currentTime", GetCurrentTime),
        DECLARE_NAPI_GETTER("duration", GetDuration),
        DECLARE_NAPI_GETTER("state", GetState)
    };

    napi_property_descriptor staticProperty[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createAudioPlayer", CreateAudioPlayer),
    };

    napi_value constructor = nullptr;
    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define AudioPlayer class");

    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to create reference of constructor");

    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set constructor");

    status = napi_define_properties(env, exports, sizeof(staticProperty) / sizeof(staticProperty[0]), staticProperty);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define static function");

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
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return result;
    }

    AudioPlayerNapi *playerNapi = new(std::nothrow) AudioPlayerNapi();
    CHECK_AND_RETURN_RET_LOG(playerNapi != nullptr, nullptr, "No memory");

    playerNapi->env_ = env;
    playerNapi->nativePlayer_ = PlayerFactory::CreatePlayer();
    CHECK_AND_RETURN_RET_LOG(playerNapi->nativePlayer_ != nullptr, nullptr, "No memory");

    if (playerNapi->callbackNapi_ == nullptr) {
        playerNapi->callbackNapi_ = std::make_shared<PlayerCallbackNapi>(env);
        (void)playerNapi->nativePlayer_->SetPlayerCallback(playerNapi->callbackNapi_);
    }

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(playerNapi),
        AudioPlayerNapi::Destructor, nullptr, &(playerNapi->wrapper_));
    if (status != napi_ok) {
        napi_get_undefined(env, &result);
        delete playerNapi;
        MEDIA_LOGE("Failed to wrap native instance");
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
        MEDIA_LOGE("Failed to get the representation of constructor object");
        napi_get_undefined(env, &result);
        return result;
    }

    status = napi_new_instance(env, constructor, 0, nullptr, &result);
    if (status != napi_ok) {
        MEDIA_LOGE("Failed to instantiate JavaScript audio player instance");
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
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "Failed to retrieve instance");

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_string) {
        player->uri_.clear();
        player->ErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    player->uri_ = CommonNapi::GetStringArgument(env, args[0]);
    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "No memory");
    int32_t ret = player->nativePlayer_->SetSource(player->uri_);
    if (ret != MSERR_OK) {
        player->ErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    ret = player->nativePlayer_->PrepareAsync();
    if (ret != MSERR_OK) {
        player->ErrorCallback(MSERR_EXT_INVALID_VAL);
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
        MEDIA_LOGE("Failed to retrieve details about the callback");
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

napi_value AudioPlayerNapi::SetMediaDataSrc(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("SetMediaDataSrc start");
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };

    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr && args[0] != nullptr,
        undefinedResult, "Failed to retrieve details about the callback");

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "get player napi error");
    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "No memory");
    if (player->dataSrcCallBack_ != nullptr) {
        player->ErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }
    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_object) {
        player->ErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    player->dataSrcCallBack_ = MediaDataSourceCallback::Create(env, args[0]);
    if (player->dataSrcCallBack_ == nullptr) {
        player->ErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }
    int32_t ret = player->nativePlayer_->SetSource(player->dataSrcCallBack_);
    if (ret != MSERR_OK) {
        player->ErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    ret = player->nativePlayer_->PrepareAsync();
    if (ret != MSERR_OK) {
        player->ErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }
    MEDIA_LOGD("SetMediaDataSrc success");
    return undefinedResult;
}

napi_value AudioPlayerNapi::GetMediaDataSrc(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    napi_value jsThis = nullptr;
    napi_value jsResult = nullptr;
    AudioPlayerNapi *player = nullptr;
    size_t argCount = 0;

    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr,
        undefinedResult, "Failed to retrieve details about the callback");

    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "Failed to retrieve instance");

    if (player->dataSrcCallBack_ == nullptr) {
        player->ErrorCallback(MSERR_EXT_NO_MEMORY);
        return undefinedResult;
    }

    jsResult = player->dataSrcCallBack_->GetDataSrc();
    CHECK_AND_RETURN_RET_LOG(jsResult != nullptr, undefinedResult, "Failed to get the representation of src object");

    MEDIA_LOGD("GetMediaDataSrc success");
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
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "get player napi error");

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "No memory");
    int32_t ret = player->nativePlayer_->Play();
    if (ret != MSERR_OK) {
        player->ErrorCallback(MSERR_EXT_UNKNOWN);
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
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "get player napi error");

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "No memory");
    int32_t ret = player->nativePlayer_->Pause();
    if (ret != MSERR_OK) {
        player->ErrorCallback(MSERR_EXT_UNKNOWN);
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
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "get player napi error");

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "No memory");
    int32_t ret = player->nativePlayer_->Stop();
    if (ret != MSERR_OK) {
        player->ErrorCallback(MSERR_EXT_UNKNOWN);
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
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "get player napi error");

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "No memory");
    if (player->dataSrcCallBack_ != nullptr) {
        player->dataSrcCallBack_->Release();
    }
    int32_t ret = player->nativePlayer_->Reset();
    if (ret != MSERR_OK) {
        player->ErrorCallback(MSERR_EXT_UNKNOWN);
        return undefinedResult;
    }
    player->dataSrcCallBack_ = nullptr;
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
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "get player napi error");

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "No memory");
    if (player->dataSrcCallBack_ != nullptr) {
        player->dataSrcCallBack_->Release();
    }
    int32_t ret = player->nativePlayer_->Release();
    if (ret != MSERR_OK) {
        player->ErrorCallback(MSERR_EXT_UNKNOWN);
        return undefinedResult;
    }
    player->callbackNapi_ = nullptr;
    player->uri_.clear();
    player->dataSrcCallBack_ = nullptr;
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
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "Failed to retrieve instance");

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        player->ErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    int32_t position = -1;
    status = napi_get_value_int32(env, args[0], &position);
    if (status != napi_ok || position < 0) {
        player->ErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "No memory");
    int32_t ret = player->nativePlayer_->Seek(position, SEEK_PREVIOUS_SYNC);
    if (ret != MSERR_OK) {
        player->ErrorCallback(MSERR_EXT_UNKNOWN);
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
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "Failed to retrieve instance");

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        player->ErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    double volumeLevel;
    status = napi_get_value_double(env, args[0], &volumeLevel);
    if (status != napi_ok || volumeLevel < 0.0f || volumeLevel > 1.0f) {
        player->ErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "No memory");
    int32_t ret = player->nativePlayer_->SetVolume(static_cast<float>(volumeLevel), static_cast<float>(volumeLevel));
    if (ret != MSERR_OK) {
        player->ErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }
    MEDIA_LOGD("SetVolume success");
    return undefinedResult;
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
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "Failed to retrieve instance");

    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string ||
        napi_typeof(env, args[1], &valueType1) != napi_ok || valueType1 != napi_function) {
        player->ErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    std::string callbackName = CommonNapi::GetStringArgument(env, args[0]);
    MEDIA_LOGD("callbackName: %{public}s", callbackName.c_str());

    CHECK_AND_RETURN_RET_LOG(player->callbackNapi_ != nullptr, undefinedResult, "callbackNapi_ is nullptr");
    std::shared_ptr<PlayerCallbackNapi> cb = std::static_pointer_cast<PlayerCallbackNapi>(player->callbackNapi_);
    cb->SaveCallbackReference(callbackName, args[1]);
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
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "Failed to retrieve instance");

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_boolean) {
        player->ErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    bool loopFlag = false;
    status = napi_get_value_bool(env, args[0], &loopFlag);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_get_value_bool error");

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "No memory");
    int32_t ret = player->nativePlayer_->SetLooping(loopFlag);
    if (ret != MSERR_OK) {
        player->ErrorCallback(MSERR_EXT_UNKNOWN);
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
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "Failed to retrieve instance");

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "No memory");
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
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "Failed to retrieve instance");

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "No memory");
    int32_t currentTime = -1;
    int32_t ret = player->nativePlayer_->GetCurrentTime(currentTime);
    if (ret != MSERR_OK || currentTime < 0) {
        player->ErrorCallback(MSERR_EXT_UNKNOWN);
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
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&player));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && player != nullptr, undefinedResult, "Failed to retrieve instance");

    CHECK_AND_RETURN_RET_LOG(player->nativePlayer_ != nullptr, undefinedResult, "No memory");
    int32_t duration = -1;
    int32_t ret = player->nativePlayer_->GetDuration(duration);
    if (ret != MSERR_OK) {
        player->ErrorCallback(MSERR_EXT_UNKNOWN);
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
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "Failed to retrieve details about the callback");

    AudioPlayerNapi *player = nullptr;
    status = napi_unwrap(env, jsThis, (void **)&player);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "Failed to retrieve instance");

    if (player->callbackNapi_ == nullptr) {
        return undefinedResult;
    } else {
        std::shared_ptr<PlayerCallbackNapi> cb = std::static_pointer_cast<PlayerCallbackNapi>(player->callbackNapi_);
        curState = GetJSState(cb->GetCurrentState());
        MEDIA_LOGD("GetState success, State: %{public}s", curState.c_str());
    }
    status = napi_create_string_utf8(env, curState.c_str(), NAPI_AUTO_LENGTH, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_create_string_utf8 error");
    return jsResult;
}

void AudioPlayerNapi::ErrorCallback(MediaServiceExtErrCode errCode)
{
    if (callbackNapi_ != nullptr) {
        std::shared_ptr<PlayerCallbackNapi> napiCb = std::static_pointer_cast<PlayerCallbackNapi>(callbackNapi_);
        napiCb->SendErrorCallback(errCode);
    }
}

void AudioPlayerNapi::AsyncGetTrackDescription(napi_env env, void *data)
{
    auto asyncContext = reinterpret_cast<AudioPlayerAsyncContext *>(data);
    CHECK_AND_RETURN_LOG(asyncContext != nullptr, "AudioPlayerAsyncContext is nullptr!");

    if (asyncContext->jsPlayer == nullptr || asyncContext->jsPlayer->nativePlayer_ == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "jsPlayer or nativePlayer is nullptr");
        return;
    }

    auto player = asyncContext->jsPlayer->nativePlayer_;
    std::vector<Format> &audioInfo = asyncContext->jsPlayer->audioTrackInfoVec_;
    audioInfo.clear();
    int32_t ret = player->GetAudioTrackInfo(audioInfo);
    if (ret != MSERR_OK) {
        asyncContext->SignError(MSERR_EXT_UNKNOWN, "failed to operate playback");
        return;
    }

    if (audioInfo.empty()) {
        asyncContext->SignError(MSERR_EXT_UNKNOWN, "audio tranck info is empty");
        return;
    }

    asyncContext->JsResult = std::make_unique<MediaJsResultArray>(audioInfo);
    MEDIA_LOGD("AsyncGetTrackDescription Out");
}

napi_value AudioPlayerNapi::GetTrackDescription(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    MEDIA_LOGD("GetTrackDescription In");
    std::unique_ptr<AudioPlayerAsyncContext> asyncContext = std::make_unique<AudioPlayerAsyncContext>(env);

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncContext->SignError(MSERR_EXT_INVALID_VAL, "failed to napi_get_cb_info");
    }

    asyncContext->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncContext->deferred = CommonNapi::CreatePromise(env, asyncContext->callbackRef, result);
    // get jsPlayer
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->jsPlayer));
    // async work
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "GetTrackDescription", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, AudioPlayerNapi::AsyncGetTrackDescription,
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    return result;
}
}  // namespace Media
}  // namespace OHOS
