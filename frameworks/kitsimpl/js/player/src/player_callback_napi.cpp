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

#include "player_callback_napi.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerCallbackNapi"};
}

namespace OHOS {
namespace Media {
PlayerCallbackNapi::PlayerCallbackNapi(napi_env env, AudioPlayerNapi &player)
    : env_(env),
      playerNapi_(player)
{
}

PlayerCallbackNapi::~PlayerCallbackNapi()
{
}

void PlayerCallbackNapi::OnError(int32_t errorType, int32_t errorCode)
{
    MEDIA_LOGD("OnError() is called, type: %{public}d, error code: %{public}d", errorType, errorCode);
    if (playerNapi_.errorCallback_ == nullptr) {
        MEDIA_LOGE("no error callback reference received from JS");
        return;
    }

    napi_value jsCallback = nullptr;
    napi_status status = napi_get_reference_value(env_, playerNapi_.errorCallback_, &jsCallback);
    if ((status != napi_ok) || (jsCallback == nullptr)) {
        MEDIA_LOGE("get reference value fail");
        return;
    }

    std::string errorTypeStr = std::to_string(errorType);
    napi_value errorTypeVal = nullptr;
    if (napi_create_string_utf8(env_, errorTypeStr.c_str(), NAPI_AUTO_LENGTH, &errorTypeVal) != napi_ok) {
        MEDIA_LOGE("get error type value fail");
        return;
    }

    std::string errorCodeStr = std::to_string(errorCode);
    napi_value errorCodeVal = nullptr;
    if (napi_create_string_utf8(env_, errorCodeStr.c_str(), NAPI_AUTO_LENGTH, &errorCodeVal) != napi_ok) {
        MEDIA_LOGE("get error code value fail");
        return;
    }

    napi_value args[1] = {nullptr};
    status = napi_create_error(env_, errorCodeVal, errorTypeVal, &args[0]);
    if (status != napi_ok) {
        MEDIA_LOGE("create error callback fail");
        return;
    }

    size_t argCount = 1;
    napi_value result = nullptr;
    status = napi_call_function(env_, nullptr, jsCallback, argCount, args, &result);
    if (status != napi_ok) {
        MEDIA_LOGE("call error callback fail");
    }

    MEDIA_LOGD("send error callback success");
}

void PlayerCallbackNapi::OnSeekDone(uint64_t currentPositon)
{
    MEDIA_LOGD("OnSeekDone() is called, currentPositon: %{public}llu", currentPositon);
    if (playerNapi_.timeUpdateCallback_ == nullptr) {
        MEDIA_LOGE("no error callback reference received from JS");
        return;
    }

    napi_value jsCallback = nullptr;
    napi_status status = napi_get_reference_value(env_, playerNapi_.timeUpdateCallback_, &jsCallback);
    if ((status != napi_ok) || (jsCallback == nullptr)) {
        MEDIA_LOGE("get reference value fail");
        return;
    }

    napi_value args[1] = {nullptr};
    status = napi_create_int64(env_, static_cast<int64_t>(currentPositon), &args[0]);
    if (status != napi_ok) {
        MEDIA_LOGE("create seekDone callback fail");
        return;
    }

    size_t argCount = 1;
    napi_value result = nullptr;
    status = napi_call_function(env_, nullptr, jsCallback, argCount, args, &result);
    if (status != napi_ok) {
        MEDIA_LOGE("call seekDone callback fail");
    }

    MEDIA_LOGD("send seekDone callback success");
}

void PlayerCallbackNapi::OnEndOfStream(bool isLooping)
{
    MEDIA_LOGD("OnEndOfStream() is called, isloop: %{public}d", isLooping);
    if (playerNapi_.finishCallback_ == nullptr) {
        MEDIA_LOGE("no error callback reference received from JS");
        return;
    }

    napi_value jsCallback = nullptr;
    napi_status status = napi_get_reference_value(env_, playerNapi_.finishCallback_, &jsCallback);
    if ((status != napi_ok) || (jsCallback == nullptr)) {
        MEDIA_LOGE("get reference value fail");
        return;
    }

    napi_value args[1] = {nullptr};
    status = napi_create_int32(env_, isLooping, &args[0]);
    if (status != napi_ok) {
        MEDIA_LOGE("create OnEndOfStream callback fail");
        return;
    }
    size_t argCount = 1;
    napi_value result = nullptr;
    status = napi_call_function(env_, nullptr, jsCallback, argCount, args, &result);
    if (status != napi_ok) {
        MEDIA_LOGE("call OnEndOfStream callback fail");
    }

    MEDIA_LOGD("send OnEndOfStream callback success");
}

void PlayerCallbackNapi::OnStateChanged(PlayerStates state)
{
    MEDIA_LOGD("OnStateChanged() is called, current state: %{public}d", state);
    napi_value jsCallback = nullptr;
    napi_value result = nullptr;
    napi_status status;
    playerNapi_.SetCurrentState(state);

    switch (state) {
        case PLAYER_IDLE:
            CHECK_AND_RETURN_LOG(playerNapi_.resetCallback_ != nullptr, "reset callback reference is null");
            status = napi_get_reference_value(env_, playerNapi_.resetCallback_, &jsCallback);
            CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr, "get reference value fail");
            status = napi_call_function(env_, nullptr, jsCallback, 0, nullptr, &result);
            CHECK_AND_RETURN_LOG(status == napi_ok, "fail to call reset callback ");
            break;
        case PLAYER_STOPPED:
            CHECK_AND_RETURN_LOG(playerNapi_.stopCallback_ != nullptr, "stop callback reference is null");
            status = napi_get_reference_value(env_, playerNapi_.stopCallback_, &jsCallback);
            CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr, "get reference value fail");
            status = napi_call_function(env_, nullptr, jsCallback, 0, nullptr, &result);
            CHECK_AND_RETURN_LOG(status == napi_ok, "call OnStateChanged callback fail");
            break;
        case PLAYER_PAUSED:
            CHECK_AND_RETURN_LOG(playerNapi_.pauseCallback_ != nullptr, "pause callback reference is null");
            status = napi_get_reference_value(env_, playerNapi_.pauseCallback_, &jsCallback);
            CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr, "get reference value fail");
            status = napi_call_function(env_, nullptr, jsCallback, 0, nullptr, &result);
            CHECK_AND_RETURN_LOG(status == napi_ok, "call OnStateChanged callback fail");
            break;
        case PLAYER_STARTED:
            CHECK_AND_RETURN_LOG(playerNapi_.playCallback_ != nullptr, "play callback reference is null");
            status = napi_get_reference_value(env_, playerNapi_.playCallback_, &jsCallback);
            CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr, "get reference value fail");
            status = napi_call_function(env_, nullptr, jsCallback, 0, nullptr, &result);
            CHECK_AND_RETURN_LOG(status == napi_ok, "call OnStateChanged callback fail");
            break;
        case PLAYER_PREPARED:
            CHECK_AND_RETURN_LOG(playerNapi_.dataLoadCallback_ != nullptr, "dataload callback reference is null");
            status = napi_get_reference_value(env_, playerNapi_.dataLoadCallback_, &jsCallback);
            CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr, "get reference value fail");
            status = napi_call_function(env_, nullptr, jsCallback, 0, nullptr, &result);
            CHECK_AND_RETURN_LOG(status == napi_ok, "call OnStateChanged callback fail");
            break;
        default:
            MEDIA_LOGE("Unknown state!, %{public}d", state);
            break;
    }
}

void PlayerCallbackNapi::OnMessage(int32_t type, int32_t extra)
{
    MEDIA_LOGD("OnMessage() is called, type: %{public}d, extra: %{public}d", type, extra);
}

void PlayerCallbackNapi::OnPositionUpdated(uint64_t postion)
{
    MEDIA_LOGD("OnPositionUpdated() is called, postion: %{public}llu", postion);
}
}  // namespace Media
}  // namespace OHOS
