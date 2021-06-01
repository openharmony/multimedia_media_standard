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

#include "hilog/log.h"
#include "audio_player_napi.h"

using namespace OHOS;
using HiviewDFX::HiLog;
using HiviewDFX::HiLogLabel;

napi_ref AudioPlayerNapi::sConstructor_ = nullptr;

namespace {
    constexpr HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioPlayerNapi"};
    const int PARAM0 = 0;
    const int PARAM1 = 1;
    const int ARGS_ONE = 1;
    const int ARGS_TWO = 2;
}

AudioPlayerNapi::AudioPlayerNapi()
    : env_(nullptr), wrapper_(nullptr), nativeCallback_(nullptr) {}

AudioPlayerNapi::~AudioPlayerNapi()
{
    HiLog::Debug(LABEL, "~AudioPlayerNapi() is called!");
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
    if (nativeCallback_ != nullptr) {
        delete (PlayerCallback *) nativeCallback_;
    }
    if (player_ != nullptr) {
        player_ = nullptr;
    }
}

PlayerCallback::PlayerCallback(napi_env environment, AudioPlayerNapi *playerWrapper)
    : env_(environment), playerWrapper_(playerWrapper)
{
}

void PlayerCallback::OnError(int32_t errorType, int32_t errCode,
                             const std::shared_ptr<Media::Player> &player) const
{
    size_t argCount = ARGS_ONE;
    napi_value jsCallback = nullptr;
    napi_status status;
    napi_value args[ARGS_ONE], result, errorCodeVal, errorTypeVal;
    std::string errCodeStr, errTypeStr;

    HiLog::Debug(LABEL, "OnError() is called!, type: %{public}d, error code: %{public}d", errorType, errCode);
    if (playerWrapper_->GetErrorCallbackRef() == nullptr) {
        HiLog::Error(LABEL, "No error callback reference received from JS");
        return;
    }

    status = napi_get_reference_value(env_, playerWrapper_->GetErrorCallbackRef(), &jsCallback);
    if (status == napi_ok && jsCallback != nullptr) {
        errCodeStr = std::to_string(errCode);
        errTypeStr = std::to_string(errorType);
        if (napi_create_string_utf8(env_, errCodeStr.c_str(), NAPI_AUTO_LENGTH, &errorCodeVal) == napi_ok
            && napi_create_string_utf8(env_, errTypeStr.c_str(), NAPI_AUTO_LENGTH, &errorTypeVal) == napi_ok) {
            status = napi_create_error(env_, errorCodeVal, errorTypeVal, &args[PARAM0]);
            if (status == napi_ok) {
                status = napi_call_function(env_, nullptr, jsCallback, argCount, args, &result);
                if (status == napi_ok) {
                    return;
                }
            }
        }
    }
    HiLog::Error(LABEL, "Failed to call error callback!");
}

void PlayerCallback::OnEndOfStream(const std::shared_ptr<Media::Player> &player) const
{
    napi_status status;
    napi_value result;
    napi_value jsCallback = nullptr;

    HiLog::Debug(LABEL, "OnEndOfStream() is called!");
    if (playerWrapper_->GetFinishCallbackRef() != nullptr) {
        status = napi_get_reference_value(env_, playerWrapper_->GetFinishCallbackRef(), &jsCallback);
        if (jsCallback != nullptr) {
            status = napi_call_function(env_, nullptr, jsCallback, 0, nullptr, &result);
            if (status == napi_ok) {
                return;
            }
        }
    }
    HiLog::Error(LABEL, "Failed to call finish callback!");
}

void PlayerCallback::OnStateChanged(Media::Player::State state, const std::shared_ptr<Media::Player> &player) const
{
    napi_status status;
    napi_value result;
    napi_value jsCallback = nullptr;

    HiLog::Debug(LABEL, "OnStateChanged() is called!, %{public}d", state);
    playerWrapper_->SetCurrentState(state);
    switch (state) {
        case Media::Player::PLAYER_STATE_STOPPED:
            if (playerWrapper_->GetStopCallbackRef() != nullptr) {
                status = napi_get_reference_value(env_, playerWrapper_->GetStopCallbackRef(), &jsCallback);
                if (jsCallback != nullptr) {
                    status = napi_call_function(env_, nullptr, jsCallback, 0, nullptr, &result);
                    if (status == napi_ok) {
                        return;
                    }
                }
            }
            break;

        case Media::Player::PLAYER_STATE_PAUSED:
            if (playerWrapper_->GetPauseCallbackRef() != nullptr) {
                status = napi_get_reference_value(env_, playerWrapper_->GetPauseCallbackRef(), &jsCallback);
                if (jsCallback != nullptr) {
                    status = napi_call_function(env_, nullptr, jsCallback, 0, nullptr, &result);
                    if (status == napi_ok) {
                        return;
                    }
                }
            }
            break;

        case Media::Player::PLAYER_STATE_PLAYING:
            if (playerWrapper_->GetPlayCallbackRef() != nullptr) {
                status = napi_get_reference_value(env_, playerWrapper_->GetPlayCallbackRef(), &jsCallback);
                if (jsCallback != nullptr) {
                    status = napi_call_function(env_, nullptr, jsCallback, 0, nullptr, &result);
                    if (status == napi_ok) {
                        return;
                    }
                }
            }
            break;

        default:
            HiLog::Error(LABEL, "Unknown state!, %{public}d", state);
            break;
    }
}

void AudioPlayerNapi::Destructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    if (nativeObject != nullptr) {
        reinterpret_cast<AudioPlayerNapi*>(nativeObject)->~AudioPlayerNapi();
    }
}

napi_value AudioPlayerNapi::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value ctorObj;
    int32_t refCount = 1;
    napi_property_descriptor audio_player_properties[] = {
        DECLARE_NAPI_FUNCTION("play", Play),
        DECLARE_NAPI_FUNCTION("pause", Pause),
        DECLARE_NAPI_FUNCTION("stop", Stop),
        DECLARE_NAPI_FUNCTION("release", Release),
        DECLARE_NAPI_FUNCTION("seek", Seek),
        DECLARE_NAPI_FUNCTION("on", On),
        DECLARE_NAPI_FUNCTION("setVolume", SetVolume),

        DECLARE_NAPI_GETTER_SETTER("src", GetSrc, SetSrc),
        DECLARE_NAPI_GETTER_SETTER("loop", GetLoop, SetLoop),

        DECLARE_NAPI_GETTER("currentTime", GetCurrenTime),
        DECLARE_NAPI_GETTER("duration", GetDuration),
        DECLARE_NAPI_GETTER("state", GetState)
    };

    napi_property_descriptor static_prop[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createAudioPlayer", CreateAudioPlayer),
    };

    HiLog::Debug(LABEL, "AudioPlayerNapi::Init() is called!");
    status = napi_define_class(env, AUDIO_PLAYER_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor, nullptr,
        sizeof(audio_player_properties) / sizeof(audio_player_properties[PARAM0]), audio_player_properties, &ctorObj);
    if (status == napi_ok) {
        status = napi_create_reference(env, ctorObj, refCount, &sConstructor_);
        if (status == napi_ok) {
            status = napi_set_named_property(env, exports, AUDIO_PLAYER_NAPI_CLASS_NAME.c_str(), ctorObj);
            if (status == napi_ok) {
                status = napi_define_properties(env, exports,
                                                sizeof(static_prop) / sizeof(static_prop[PARAM0]), static_prop);
                if (status == napi_ok) {
                    HiLog::Info(LABEL, "All props and functions are configured..");
                    return exports;
                }
            }
        }
    }
    HiLog::Error(LABEL, "Failure in AudioPlayerNapi::Init()");
    return nullptr;
}

// Constructor callback
napi_value AudioPlayerNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsThis = nullptr;
    napi_value result = nullptr;
    size_t argCount = 0;

    HiLog::Debug(LABEL, "Constructor() is called!");
    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status == napi_ok) {
        AudioPlayerNapi* obj = new AudioPlayerNapi();
        if (obj != nullptr) {
            obj->env_ = env;
            obj->player_ = Media::Player::Create(AUDIO_MIME, VIDEO_MIME);
            status = napi_wrap(env, jsThis, reinterpret_cast<void*>(obj),
                               AudioPlayerNapi::Destructor, nullptr, &(obj->wrapper_));
            if (status == napi_ok) {
                obj->nativeCallback_ = (void *) new PlayerCallback(env, obj);
                obj->player_->SetPlayerCallback(*((PlayerCallback *)obj->nativeCallback_));
                HiLog::Debug(LABEL, "Setting callback is done!");
                return jsThis;
            }
        }
    }
    HiLog::Error(LABEL, "Failed in AudioPlayerNapi::New()!");
    napi_get_undefined(env, &result);
    return result;
}

napi_value AudioPlayerNapi::CreateAudioPlayer(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value ctor;

    HiLog::Debug(LABEL, "CreateAudioPlayer() is called!");
    status = napi_get_reference_value(env, sConstructor_, &ctor);
    if (status == napi_ok) {
        status = napi_new_instance(env, ctor, 0, nullptr, &result);
        if (status == napi_ok) {
            return result;
        }
    }
    HiLog::Error(LABEL, "Failed in AudioPlayerNapi::CreateAudioPlayer()!");
    napi_get_undefined(env, &result);
    return result;
}

napi_ref AudioPlayerNapi::GetErrorCallbackRef()
{
    napi_ref errorCb = errorCallback_;
    return errorCb;
}

napi_ref AudioPlayerNapi::GetPlayCallbackRef()
{
    napi_ref playCb = playCallback_;
    return playCb;
}

napi_ref AudioPlayerNapi::GetPauseCallbackRef()
{
    napi_ref pauseCb = pauseCallback_;
    return pauseCb;
}

napi_ref AudioPlayerNapi::GetFinishCallbackRef()
{
    napi_ref finishCb = finishCallback_;
    return finishCb;
}

napi_ref AudioPlayerNapi::GetStopCallbackRef()
{
    napi_ref stopCb = stopCallback_;
    return stopCb;
}

void AudioPlayerNapi::SetCurrentState(Media::Player::State state)
{
    this->currentState_ = state;
}

// Function to read string argument from napi_value
static std::string GetStringArgument(napi_env env, napi_value value)
{
    napi_status status;
    std::string strValue = "";
    size_t bufLength = 0;
    char *buffer = nullptr;

    // get buffer length first and get buffer based on length
    status = napi_get_value_string_utf8(env, value, nullptr, 0, &bufLength);
    if (status == napi_ok && bufLength > 0) {
        // Create a buffer and create std::string later from it
        buffer = (char *) malloc((bufLength + 1) * sizeof(char));
        if (buffer != nullptr) {
            status = napi_get_value_string_utf8(env, value, buffer, bufLength + 1, &bufLength);
            if (status == napi_ok) {
                HiLog::Info(LABEL, "argument = %{public}s", buffer);
                strValue = buffer;
            }
            free(buffer);
            buffer = nullptr;
        }
    }
    return strValue;
}

// Functions in AudioPlayer
napi_value AudioPlayerNapi::Play(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_value undefinedResult = nullptr;
    AudioPlayerNapi* playerWrapper = nullptr;

    napi_get_undefined(env, &undefinedResult);
    HiLog::Debug(LABEL, "Play() is called!");
    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || argCount != 0) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, jsThis, (void**) &playerWrapper);
    if (status == napi_ok) {
        if (playerWrapper->player_->Play() != 0) {
            HiLog::Error(LABEL, "Failed to Play");
        }
        return undefinedResult;
    }
    HiLog::Error(LABEL, "Failed to Play");
    return undefinedResult;
}

napi_value AudioPlayerNapi::Pause(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_value undefinedResult = nullptr;
    AudioPlayerNapi* playerWrapper = nullptr;

    HiLog::Debug(LABEL, "Pause() is called!");
    napi_get_undefined(env, &undefinedResult);
    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || argCount != 0) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, jsThis, (void**) &playerWrapper);
    if (status == napi_ok) {
        if (playerWrapper->player_->Pause() != 0) {
            HiLog::Error(LABEL, "Failed to pause");
        }
        return undefinedResult;
    }
    HiLog::Error(LABEL, "Failed to Pause");
    return undefinedResult;
}

napi_value AudioPlayerNapi::Stop(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_value undefinedResult = nullptr;
    AudioPlayerNapi* playerWrapper = nullptr;

    HiLog::Debug(LABEL, "Stop() is called!");
    napi_get_undefined(env, &undefinedResult);
    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || argCount != 0) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, jsThis, (void**) &playerWrapper);
    if (status == napi_ok) {
        if (playerWrapper->player_->Stop() != 0) {
            HiLog::Error(LABEL, "Failed to stop");
        }
        return undefinedResult;
    }

    HiLog::Error(LABEL, "Failed to stop");
    return undefinedResult;
}

napi_value AudioPlayerNapi::Release(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_value undefinedResult = nullptr;
    AudioPlayerNapi* playerWrapper = nullptr;

    HiLog::Debug(LABEL, "Release() is called!");
    napi_get_undefined(env, &undefinedResult);
    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || argCount != 0) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, jsThis, (void**) &playerWrapper);
    if (status == napi_ok) {
        if (playerWrapper->player_->Release() != 0) {
            HiLog::Error(LABEL, "Failed to Release");
        }
        return undefinedResult;
    }
    HiLog::Error(LABEL, "Failed to Release");
    return undefinedResult;
}

napi_value AudioPlayerNapi::Seek(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argCount = ARGS_ONE;
    napi_value jsThis = nullptr;
    napi_value args[ARGS_ONE] = {0};
    napi_value jsCallback, result;
    napi_value undefinedResult = nullptr;
    int64_t position;
    AudioPlayerNapi* playerWrapper = nullptr;
    napi_valuetype valueType = napi_undefined;

    HiLog::Debug(LABEL, "Seek() is called!");
    napi_get_undefined(env, &undefinedResult);
    status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || argCount < ARGS_ONE) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, jsThis, (void**) &playerWrapper);
    if (status != napi_ok) {
        HiLog::Error(LABEL, "Failed to unwrap player object!");
        return undefinedResult;
    }

    if (napi_typeof(env, args[PARAM0], &valueType) != napi_ok
        || valueType != napi_number) {
        HiLog::Error(LABEL, "Invalid arguments type!");
        return undefinedResult;
    }

    status = napi_get_value_int64(env, args[PARAM0], &position);
    if (status == napi_ok) {
        if (playerWrapper->player_->Rewind(position, -1) == 0) {
            if (playerWrapper->timeUpdateCallback_ != nullptr) {
                status = napi_get_reference_value(env, playerWrapper->timeUpdateCallback_, &jsCallback);
                if (status == napi_ok && jsCallback != nullptr) {
                    status = napi_call_function(env, nullptr, jsCallback, 0, nullptr, &result);
                }
            }
        } else {
            HiLog::Error(LABEL, "Failed to seek() from native");
        }
        return undefinedResult;
    }

    HiLog::Error(LABEL, "Failed to seek");
    return undefinedResult;
}

napi_value AudioPlayerNapi::SetVolume(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argCount = ARGS_ONE;
    napi_value jsThis = nullptr;
    napi_value args[ARGS_ONE] = {0};
    napi_value jsCallback, result;
    napi_value undefinedResult = nullptr;
    double volumeLevel;
    AudioPlayerNapi* playerWrapper = nullptr;
    napi_valuetype valueType = napi_undefined;

    HiLog::Debug(LABEL, "SetVolume() is called!");
    napi_get_undefined(env, &undefinedResult);
    status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || argCount < ARGS_ONE) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, jsThis, (void**) &playerWrapper);
    if (status != napi_ok) {
        HiLog::Error(LABEL, "Failed to unwrap player object!");
        return undefinedResult;
    }

    if (napi_typeof(env, args[PARAM0], &valueType) != napi_ok
        || valueType != napi_number) {
        HiLog::Error(LABEL, "Invalid arguments type!");
        return undefinedResult;
    }

    status = napi_get_value_double(env, args[PARAM0], &volumeLevel);
    if (status == napi_ok) {
        HiLog::Debug(LABEL, "volumeLevel: %{public}f:", volumeLevel);
        if (playerWrapper->player_->SetVolume(volumeLevel, volumeLevel) == 0) {
            if (playerWrapper->volumeChangeCallback_ != nullptr) {
                status = napi_get_reference_value(env, playerWrapper->volumeChangeCallback_, &jsCallback);
                if (status == napi_ok && jsCallback != nullptr) {
                    status = napi_call_function(env, nullptr, jsCallback, 0, nullptr, &result);
                }
            }
        } else {
            HiLog::Error(LABEL, "Failed to SetVolume() from native");
        }
        return undefinedResult;
    }

    HiLog::Error(LABEL, "Failed to SetVolume");
    return undefinedResult;
}

void AudioPlayerNapi::SaveCallbackReference(napi_env env, AudioPlayerNapi* audioPlayer,
    std::string callbackName, napi_value callback)
{
    napi_ref *ref = nullptr;
    napi_status status;
    int32_t refCount = 1;

    if (callbackName == AP_CALLBACK_PLAY) {
        ref = &(audioPlayer->playCallback_);
    } else if (callbackName == AP_CALLBACK_PAUSE) {
        ref = &(audioPlayer->pauseCallback_);
    } else if (callbackName == AP_CALLBACK_STOP) {
        ref = &(audioPlayer->stopCallback_);
    } else if (callbackName == AP_CALLBACK_DATA_LOAD) {
        ref = &(audioPlayer->dataLoadCallback_);
    } else if (callbackName == AP_CALLBACK_FINISH) {
        ref = &(audioPlayer->finishCallback_);
    } else if (callbackName == AP_CALLBACK_TIME_UPDATE) {
        ref = &(audioPlayer->timeUpdateCallback_);
    } else if (callbackName == AP_CALLBACK_ERROR) {
        ref = &(audioPlayer->errorCallback_);
    } else if (callbackName == AP_CALLBACK_VOL_CHANGE) {
        ref = &(audioPlayer->volumeChangeCallback_);
    } else {
        HiLog::Error(LABEL, "Unknown callback!!, %{public}s", callbackName.c_str());
        return;
    }

    status = napi_create_reference(env, callback, refCount, ref);
    if (status != napi_ok) {
        HiLog::Error(LABEL, "Unknown error while creating reference for callback!");
    }
}

napi_value AudioPlayerNapi::On(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argCount = ARGS_TWO;
    napi_value jsThis = nullptr;
    napi_value undefinedResult = nullptr;
    napi_value args[ARGS_TWO] = {0};
    AudioPlayerNapi* playerWrapper = nullptr;
    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    std::string callbackName;

    HiLog::Debug(LABEL, "on() is called!");
    napi_get_undefined(env, &undefinedResult);
    status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || argCount < ARGS_TWO) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return nullptr;
    }

    status = napi_unwrap(env, jsThis, reinterpret_cast<void**>(&playerWrapper));
    if (status == napi_ok) {
        if (napi_typeof(env, args[PARAM0], &valueType0) != napi_ok || valueType0 != napi_string
            || napi_typeof(env, args[PARAM1], &valueType1) != napi_ok || valueType1 != napi_function) {
            HiLog::Error(LABEL, "Invalid arguments type!");
            return undefinedResult;
        }

        callbackName = GetStringArgument(env, args[PARAM0]);
        HiLog::Debug(LABEL, "callbackName: %{public}s", callbackName.c_str());
        playerWrapper->SaveCallbackReference(env, playerWrapper, callbackName, args[PARAM1]);
        return undefinedResult;
    }

    HiLog::Error(LABEL, "Failed to set callback");
    return undefinedResult;
}

// src property
napi_value AudioPlayerNapi::SetSrc(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argCount = ARGS_ONE;
    napi_value jsThis = nullptr;
    napi_value jsCallback, result;
    napi_value args[ARGS_ONE] = {0};
    napi_value undefinedResult = nullptr;
    AudioPlayerNapi* playerWrapper = nullptr;
    napi_valuetype valueType = napi_undefined;

    HiLog::Debug(LABEL, "setSrc() is called!");
    napi_get_undefined(env, &undefinedResult);
    status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || argCount < ARGS_ONE) {
        HiLog::Error(LABEL, "Failed to get arguments or invalid nuumber of arguments!");
        return undefinedResult;
    }

    if (napi_unwrap(env, jsThis, (void**) &playerWrapper) != napi_ok) {
        HiLog::Error(LABEL, "Failed to unwrap player object!");
        return undefinedResult;
    }
    if (napi_typeof(env, args[PARAM0], &valueType) != napi_ok || valueType != napi_string) {
        HiLog::Error(LABEL, "Invalid arguments!");
        playerWrapper->uri_ = nullptr;
        return undefinedResult;
    }

    playerWrapper->uri_ = GetStringArgument(env, args[PARAM0]);  // Init Source
    HiLog::Debug(LABEL, "src: %{public}s", (playerWrapper->uri_).c_str());
    if (playerWrapper->player_->SetSource(playerWrapper->uri_) == 0) {  // set source to player native
        HiLog::Debug(LABEL, "src is loaded!");
        if (playerWrapper->player_->Prepare() == 0) {
            HiLog::Debug(LABEL, "Prepare() is succesful!");
            if (playerWrapper->dataLoadCallback_ == nullptr) {
                HiLog::Debug(LABEL, "dataLoadCallback_ is not registered!");
                return undefinedResult;
            }
            status = napi_get_reference_value(env, playerWrapper->dataLoadCallback_, &jsCallback);
            if (status == napi_ok && jsCallback != nullptr) {
                if (napi_call_function(env, nullptr, jsCallback, 0, nullptr, &result) == napi_ok) {
                    return undefinedResult;
                }
            }
        } else {
            HiLog::Error(LABEL, "Failed to prepare() from native");
        }
    }
    playerWrapper->uri_ = nullptr;
    HiLog::Error(LABEL, "Failed to set Src");
    return undefinedResult;
}

napi_value AudioPlayerNapi::GetSrc(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    AudioPlayerNapi* playerWrapper = nullptr;

    HiLog::Debug(LABEL, "GetSrc() is called!");
    napi_get_undefined(env, &undefinedResult);
    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return nullptr;
    }

    status = napi_unwrap(env, jsThis, (void**) &playerWrapper);
    if (status == napi_ok && playerWrapper != nullptr) {
        if (playerWrapper->uri_.length() != 0) {
            HiLog::Debug(LABEL, "URI: %{public}s", playerWrapper->uri_.c_str());
            status = napi_create_string_utf8(env, playerWrapper->uri_.c_str(), NAPI_AUTO_LENGTH, &jsResult);
            return (status == napi_ok) ? jsResult : undefinedResult;
        }
    }

    HiLog::Error(LABEL, "Failed in GetSrc()!");
    return undefinedResult;
}

// loop property
napi_value AudioPlayerNapi::SetLoop(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argCount = ARGS_ONE;
    napi_value jsThis = nullptr;
    napi_value args[ARGS_ONE] = {0};
    napi_value undefinedResult = nullptr;
    bool loopFlag = false;
    AudioPlayerNapi* playerWrapper = nullptr;
    napi_valuetype valueType = napi_undefined;

    HiLog::Debug(LABEL, "SetLoop() is called!");
    napi_get_undefined(env, &undefinedResult);
    status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || argCount < ARGS_ONE) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, jsThis, (void**) &playerWrapper);
    if (status == napi_ok) {
        if (napi_typeof(env, args[PARAM0], &valueType) != napi_ok
            || valueType != napi_boolean) {
            HiLog::Error(LABEL, "Invalid arguments type!");
            return undefinedResult;
        }

        status = napi_get_value_bool(env, args[PARAM0], &loopFlag);
        if (status == napi_ok) {
            if (playerWrapper->player_->EnableSingleLooping(loopFlag) != 0) {
                HiLog::Error(LABEL, "Failed to enable/disable looping");
            }
            return undefinedResult;
        }
    }
    HiLog::Error(LABEL, "Failed to set Loop");
    return undefinedResult;
}

napi_value AudioPlayerNapi::GetLoop(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    AudioPlayerNapi* playerWrapper = nullptr;
    bool loopFlag = false;

    HiLog::Debug(LABEL, "GetLoop() is called!");
    napi_get_undefined(env, &undefinedResult);
    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, jsThis, (void**) &playerWrapper);
    if (status == napi_ok && playerWrapper != nullptr) {
        loopFlag = playerWrapper->player_->IsSingleLooping();
        HiLog::Debug(LABEL, "Loop: %{public}d", loopFlag);
        status = napi_get_boolean(env, loopFlag, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        }
    }
    HiLog::Error(LABEL, "Failed in GetLoop()!");
    return undefinedResult;
}

// current time property (readonly)
napi_value AudioPlayerNapi::GetCurrenTime(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    AudioPlayerNapi* playerWrapper = nullptr;
    int64_t currentTime;

    HiLog::Debug(LABEL, "GetCurrenTime()! is called!");
    napi_get_undefined(env, &undefinedResult);
    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, jsThis, (void**) &playerWrapper);
    if (status == napi_ok && playerWrapper != nullptr) {
        if (playerWrapper->player_->GetCurrentTime(currentTime) == 0) {
            HiLog::Debug(LABEL, "Current time: %{public}lld", currentTime);
            status = napi_create_int64(env, currentTime, &jsResult);
            if (status == napi_ok) {
                return jsResult;
            }
        }
    }
    HiLog::Error(LABEL, "Failed in GetCurrenTime()!");
    return undefinedResult;
}

// duration property (readonly)
napi_value AudioPlayerNapi::GetDuration(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    AudioPlayerNapi* playerWrapper = nullptr;
    int64_t duration;

    HiLog::Debug(LABEL, "GetDuration()! is called!");
    napi_get_undefined(env, &undefinedResult);
    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, jsThis, (void**) &playerWrapper);
    if (status == napi_ok && playerWrapper != nullptr) {
        if (playerWrapper->player_->GetDuration(duration) == 0) {
            HiLog::Debug(LABEL, "Duration: %{public}lld", duration);
            status = napi_create_int64(env, duration, &jsResult);
            if (status == napi_ok) {
                return jsResult;
            }
        }
    }
    HiLog::Error(LABEL, "Failed in getDuration()!");
    return undefinedResult;
}

static std::string GetJSState(Media::Player::State currentState)
{
    std::string result;

    HiLog::Debug(LABEL, "GetJSState()! is called!, %{public}d", currentState);
    switch (currentState) {
        case Media::Player::PLAYER_STATE_STOPPED:
            result = AP_STATE_STOPPED;
            break;

        case Media::Player::PLAYER_STATE_PAUSED:
            result = AP_STATE_PAUSED;
            break;

        case Media::Player::PLAYER_STATE_PLAYING:
            result = AP_STATE_PLAYING;
            break;

        case Media::Player::PLAYER_STATE_BUFFERING:
            // Considering buffering state is equivalant to playing
            result = AP_STATE_PLAYING;
            break;

        default:
            // Considering default state as stopped
            HiLog::Error(LABEL, "Unknown state!, %{public}d", currentState);
            result = AP_STATE_STOPPED;
            break;
    }
    return result;
}

// state property (readonly)
napi_value AudioPlayerNapi::GetState(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_value jsResult = nullptr;
    napi_value undefinedResult = nullptr;
    AudioPlayerNapi *playerWrapper = nullptr;
    std::string curState;

    HiLog::Debug(LABEL, "GetState()! is called!");
    napi_get_undefined(env, &undefinedResult);
    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return undefinedResult;
    }

    status = napi_unwrap(env, jsThis, (void**) &playerWrapper);
    if (status == napi_ok && playerWrapper != nullptr) {
        curState = GetJSState(playerWrapper->currentState_);
        HiLog::Debug(LABEL, "curState: %{public}s", curState.c_str());
        status = napi_create_string_utf8(env, curState.c_str(), NAPI_AUTO_LENGTH, &jsResult);
        if (status != napi_ok) {
            HiLog::Debug(LABEL, "Failed to create");
            napi_get_undefined(env, &jsResult);
        }
        return jsResult;
    }
    HiLog::Error(LABEL, "Failed in GetState()!");
    return undefinedResult;
}
