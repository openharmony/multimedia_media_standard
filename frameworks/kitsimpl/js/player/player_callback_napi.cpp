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
#include <uv.h>
#include "media_errors.h"
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

void PlayerCallbackNapi::OnError(PlayerErrorType errorType, int32_t errorCode)
{
    MEDIA_LOGD("OnError is called, type: %{public}d, error code: %{public}d", errorType, errorCode);
    CHECK_AND_RETURN_LOG(env_ != nullptr, "env is nullptr");
    CHECK_AND_RETURN_LOG(playerNapi_.errorCallback_ != nullptr, "errorCallback_ is nullptr");
    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback {
        .env = env_,
        .callback = playerNapi_.errorCallback_,
        .callbackName = ERROR_CALLBACK_NAME,
        .errorType = PlayerErrorTypeToString(errorType),
        .errorCode = MSErrorToString(static_cast<MediaServiceErrCode>(errorCode)),
    };
    CHECK_AND_RETURN_LOG(cb != nullptr, "cb is nullptr");
    return OnJsCallBackError(cb);
}

void PlayerCallbackNapi::OnVolumeChangeCb()
{
    MEDIA_LOGD("OnVolumeChangeCb in");
    CHECK_AND_RETURN_LOG(env_ != nullptr, "env is nullptr");
    CHECK_AND_RETURN_LOG(playerNapi_.volumeChangeCallback_ != nullptr, "volumeChangeCallback_ is nullptr");

    napi_ref *ref = &playerNapi_.volumeChangeCallback_;
    uint32_t thisRefCount = 0;
    napi_reference_ref(env_, *ref, &thisRefCount);

    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback {
        .env = env_,
        .callback = *ref,
        .callbackName = VOL_CHANGE_CALLBACK_NAME,
    };
    CHECK_AND_RETURN_LOG(cb != nullptr, "cb is nullptr");
    return OnJsCallBack(cb);
}

void PlayerCallbackNapi::OnInfo(PlayerOnInfoType type, int32_t extra, const Format &InfoBody)
{
    MEDIA_LOGI("OnInfo is called, PlayerOnInfoType: %{public}d", type);
    switch (type) {
        case INFO_TYPE_SEEKDONE:
            OnSeekDoneCb(extra);
            break;
        case INFO_TYPE_EOS:
            OnEosCb(extra);
            break;
        case INFO_TYPE_STATE_CHANGE:
            OnStateChangeCb(static_cast<PlayerStates>(extra));
            break;
        case INFO_TYPE_POSITION_UPDATE:
            OnPositionUpdateCb(extra);
            break;
        case INFO_TYPE_MESSAGE:
            OnMessageCb(extra);
            break;
        case INFO_TYPE_VOLUME_CHANGE:
            OnVolumeChangeCb();
            break;
        default:
            break;
    }
    MEDIA_LOGD("send OnInfo callback success");
}

void PlayerCallbackNapi::OnSeekDoneCb(int32_t currentPositon)
{
    MEDIA_LOGD("OnSeekDone is called, currentPositon: %{public}d", currentPositon);
    CHECK_AND_RETURN_LOG(env_ != nullptr, "env is nullptr");
    CHECK_AND_RETURN_LOG(playerNapi_.timeUpdateCallback_ != nullptr, "timeUpdateCallback_ is nullptr");

    napi_ref *ref = &playerNapi_.timeUpdateCallback_;
    uint32_t thisRefCount = 0;
    napi_reference_ref(env_, *ref, &thisRefCount);

    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback {
        .env = env_,
        .callback = *ref,
        .callbackName = TIME_UPDATE_CALLBACK_NAME,
        .position = currentPositon,
    };
    CHECK_AND_RETURN_LOG(cb != nullptr, "cb is nullptr");
    return OnJsCallBackPosition(cb);
}

void PlayerCallbackNapi::OnEosCb(int32_t isLooping)
{
    MEDIA_LOGD("OnEndOfStream is called, isloop: %{public}d", isLooping);
    CHECK_AND_RETURN_LOG(env_ != nullptr, "env is nullptr");
    CHECK_AND_RETURN_LOG(playerNapi_.finishCallback_ != nullptr, "finishCallback_ is nullptr");

    napi_ref *ref = &playerNapi_.finishCallback_;
    uint32_t thisRefCount = 0;
    napi_reference_ref(env_, *ref, &thisRefCount);

    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback {
        .env = env_,
        .callback = *ref,
        .callbackName = FINISH_CALLBACK_NAME,
    };
    CHECK_AND_RETURN_LOG(cb != nullptr, "cb is nullptr");
    return OnJsCallBack(cb);
}

void PlayerCallbackNapi::OnStateChangeCb(PlayerStates state)
{
    MEDIA_LOGD("OnStateChanged is called, current state: %{public}d", state);
    playerNapi_.SetCurrentState(state);

    napi_ref *ref = nullptr;
    std::string callbackName = "unknown";
    switch (state) {
        case PLAYER_PREPARED:
            ref = &playerNapi_.dataLoadCallback_;
            callbackName = DATA_LOAD_CALLBACK_NAME;
            break;
        case PLAYER_STARTED:
            ref = &playerNapi_.playCallback_;
            callbackName = PLAY_CALLBACK_NAME;
            break;
        case PLAYER_PAUSED:
            ref = &playerNapi_.pauseCallback_;
            callbackName = PAUSE_CALLBACK_NAME;
            break;
        case PLAYER_STOPPED:
            ref = &playerNapi_.stopCallback_;
            callbackName = STOP_CALLBACK_NAME;
            break;
        case PLAYER_IDLE:
            ref = &playerNapi_.resetCallback_;
            callbackName = RESET_CALLBACK_NAME;
            break;
        default:
            ref = nullptr;
            callbackName = "unknown";
            break;
    }
    CHECK_AND_RETURN_LOG(env_ != nullptr, "env is nullptr");
    CHECK_AND_RETURN_LOG(ref != nullptr, "ref is nullptr");
    CHECK_AND_RETURN_LOG(*ref != nullptr, "callback is nullptr");

    uint32_t thisRefCount = 0;
    napi_reference_ref(env_, *ref, &thisRefCount);

    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback {
        .env = env_,
        .callback = *ref,
        .callbackName = callbackName,
    };
    CHECK_AND_RETURN_LOG(cb != nullptr, "cb is nullptr");
    return OnJsCallBack(cb);
}

void PlayerCallbackNapi::OnPositionUpdateCb(int32_t postion)
{
    MEDIA_LOGD("OnPositionUpdateCb is called, postion: %{public}d", postion);
}

void PlayerCallbackNapi::OnMessageCb(int32_t type)
{
    MEDIA_LOGD("OnMessageCb is called, type: %{public}d", type);
}

void PlayerCallbackNapi::OnJsCallBack(PlayerJsCallback *jsCb)
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);

    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        MEDIA_LOGE("fail to new uv_work_t");
        delete jsCb;
        return;
    }
    work->data = reinterpret_cast<void *>(jsCb);

    uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        PlayerJsCallback *event = reinterpret_cast<PlayerJsCallback *>(work->data);
        std::string request = event->callbackName;
        MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work start", request.c_str());
        do {
            napi_value jsCallback = nullptr;
            status = napi_get_reference_value(event->env, event->callback, &jsCallback);
            CHECK_AND_BREAK_LOG(status == napi_ok && jsCallback != nullptr, "%{public}s get reference value fail",
                request.c_str());

            // Call back function
            napi_value result = nullptr;
            status = napi_call_function(event->env, nullptr, jsCallback, 0, nullptr, &result);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s fail to napi call function", request.c_str());

            uint32_t thisRefCount = 0;
            napi_reference_unref(event->env, event->callback, &thisRefCount);
        } while (0);
        delete event;
        delete work;
    });
}

void PlayerCallbackNapi::OnJsCallBackError(PlayerJsCallback *jsCb)
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);

    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        MEDIA_LOGE("fail to new uv_work_t");
        delete jsCb;
        return;
    }
    work->data = reinterpret_cast<void *>(jsCb);

    uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        PlayerJsCallback *event = reinterpret_cast<PlayerJsCallback *>(work->data);
        std::string request = event->callbackName;
        MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work start", request.c_str());
        do {
            napi_value jsCallback = nullptr;
            status = napi_get_reference_value(event->env, event->callback, &jsCallback);
            CHECK_AND_BREAK_LOG(status == napi_ok && jsCallback != nullptr, "%{public}s get reference value fail",
                request.c_str());

            napi_value errorTypeVal = nullptr;
            status = napi_create_string_utf8(event->env, event->errorType.c_str(), NAPI_AUTO_LENGTH, &errorTypeVal);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s fail to get error type value", request.c_str());

            napi_value errorCodeVal = nullptr;
            status = napi_create_string_utf8(event->env, event->errorCode.c_str(), NAPI_AUTO_LENGTH, &errorCodeVal);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s fail to get error code value", request.c_str());

            napi_value args[1] = { nullptr };
            status = napi_create_error(event->env, errorTypeVal, errorCodeVal, &args[0]);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s fail to create error callback", request.c_str());

            // Call back function
            const size_t argCount = 1;
            napi_value result = nullptr;
            status = napi_call_function(event->env, nullptr, jsCallback, argCount, args, &result);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s fail to napi call function", request.c_str());

            uint32_t thisRefCount = 0;
            napi_reference_unref(event->env, event->callback, &thisRefCount);
        } while (0);
        delete event;
        delete work;
    });
}

void PlayerCallbackNapi::OnJsCallBackPosition(PlayerJsCallback *jsCb)
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);

    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        MEDIA_LOGE("fail to new uv_work_t");
        delete jsCb;
        return;
    }
    work->data = reinterpret_cast<void *>(jsCb);

    uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        PlayerJsCallback *event = reinterpret_cast<PlayerJsCallback *>(work->data);
        std::string request = event->callbackName;
        MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work start", request.c_str());
        do {
            napi_value jsCallback = nullptr;
            status = napi_get_reference_value(event->env, event->callback, &jsCallback);
            CHECK_AND_BREAK_LOG(status == napi_ok && jsCallback != nullptr, "%{public}s get reference value fail",
                request.c_str());

            // Call back function
            napi_value args[1] = { nullptr };
            status = napi_create_int32(event->env, event->position, &args[0]);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s fail to create position callback", request.c_str());

            const size_t argCount = 1;
            napi_value result = nullptr;
            status = napi_call_function(event->env, nullptr, jsCallback, argCount, args, &result);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s fail to call seekDone callback", request.c_str());

            uint32_t thisRefCount = 0;
            napi_reference_unref(event->env, event->callback, &thisRefCount);
        } while (0);
        delete event;
        delete work;
    });
}
}  // namespace Media
}  // namespace OHOS