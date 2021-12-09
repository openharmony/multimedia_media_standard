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
const std::string PLAY_CALLBACK_NAME = "play";
const std::string PAUSE_CALLBACK_NAME = "pause";
const std::string STOP_CALLBACK_NAME = "stop";
const std::string RESET_CALLBACK_NAME = "reset";
const std::string DATA_LOAD_CALLBACK_NAME = "dataLoad";
const std::string FINISH_CALLBACK_NAME = "finish";
const std::string TIME_UPDATE_CALLBACK_NAME = "timeUpdate";
const std::string ERROR_CALLBACK_NAME = "error";
const std::string VOL_CHANGE_CALLBACK_NAME = "volumeChange";
const std::string BUFFERING_UPDATE_CALLBACK_NAME = "bufferingUpdate";
}

namespace OHOS {
namespace Media {
PlayerCallbackNapi::PlayerCallbackNapi(napi_env env)
    : env_(env)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayerCallbackNapi::~PlayerCallbackNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void PlayerCallbackNapi::SaveCallbackReference(const std::string &callbackName, napi_value args)
{
    std::lock_guard<std::mutex> lock(mutex_);
    napi_ref callback = nullptr;
    const int32_t refCount = 1;
    napi_status status = napi_create_reference(env_, args, refCount, &callback);
    CHECK_AND_RETURN_LOG(status == napi_ok && callback != nullptr, "creating reference for callback fail");

    std::shared_ptr<AutoRef> cb = std::make_shared<AutoRef>(env_, callback);
    if (callbackName == PLAY_CALLBACK_NAME) {
        playCallback_ = cb;
    } else if (callbackName == PAUSE_CALLBACK_NAME) {
        pauseCallback_ = cb;
    } else if (callbackName == STOP_CALLBACK_NAME) {
        stopCallback_ = cb;
    } else if (callbackName == RESET_CALLBACK_NAME) {
        resetCallback_ = cb;
    } else if (callbackName == DATA_LOAD_CALLBACK_NAME) {
        dataLoadCallback_ = cb;
    } else if (callbackName == FINISH_CALLBACK_NAME) {
        finishCallback_ = cb;
    } else if (callbackName == TIME_UPDATE_CALLBACK_NAME) {
        timeUpdateCallback_ = cb;
    } else if (callbackName == ERROR_CALLBACK_NAME) {
        errorCallback_ = cb;
    } else if (callbackName == VOL_CHANGE_CALLBACK_NAME) {
        volumeChangeCallback_ = cb;
    } else if (callbackName == BUFFERING_UPDATE_CALLBACK_NAME) {
        bufferingUpdateCallback_ = cb;
    } else {
        MEDIA_LOGW("Unknown callback type: %{public}s", callbackName.c_str());
    }
}

void PlayerCallbackNapi::SendErrorCallback(MediaServiceExtErrCode errCode, const std::string &info)
{
    MEDIA_LOGE("in ErrorCallback: %{public}s", info.c_str());
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(errorCallback_ != nullptr, "no error callback reference");

    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "cb is nullptr");

    cb->callback = errorCallback_;
    cb->callbackName = info.c_str();
    cb->errorMsg = MSExtErrorToString(errCode);
    cb->errorCode = errCode;
    return OnJsCallBackError(cb);
}

PlayerStates PlayerCallbackNapi::GetCurrentState() const
{
    return currentState_;
}

void PlayerCallbackNapi::OnError(PlayerErrorType errorType, int32_t errorCode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("OnError is called, name: %{public}d, message: %{public}d", errorType, errorCode);
    CHECK_AND_RETURN_LOG(errorCallback_ != nullptr, "Cannot find the reference of error callback");

    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "No memory");
    cb->callback = errorCallback_;
    cb->callbackName = ERROR_CALLBACK_NAME;
    cb->errorMsg = MSErrorToString(static_cast<MediaServiceErrCode>(errorCode));
    cb->errorCode = MSErrorToExtError(static_cast<MediaServiceErrCode>(errorCode));
    return OnJsCallBackError(cb);
}

void PlayerCallbackNapi::OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    std::lock_guard<std::mutex> lock(mutex_);
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
        case INFO_TYPE_BUFFERING_UPDATE:
            OnBufferingUpdateCb(infoBody);
            break;
        default:
            break;
    }
    MEDIA_LOGD("send OnInfo callback success");
}

void PlayerCallbackNapi::OnSeekDoneCb(int32_t currentPositon) const
{
    MEDIA_LOGD("OnSeekDone is called, currentPositon: %{public}d", currentPositon);
    CHECK_AND_RETURN_LOG(timeUpdateCallback_ != nullptr, "Cannot find the reference of timeUpdate callback");

    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "No memory");
    cb->callback = timeUpdateCallback_;
    cb->callbackName = TIME_UPDATE_CALLBACK_NAME;
    cb->valueVec.push_back(currentPositon);
    return OnJsCallBackInt(cb);
}

void PlayerCallbackNapi::OnBufferingUpdateCb(const Format &infoBody) const
{
    CHECK_AND_RETURN_LOG(bufferingUpdateCallback_ != nullptr,
        "Cannot find the reference of buffering percent callback");

    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "No memory");
    cb->callback = bufferingUpdateCallback_;
    cb->callbackName = BUFFERING_UPDATE_CALLBACK_NAME;

    int32_t value = 0;
    int32_t bufferingType = -1;
    if (infoBody.GetIntValue(std::string(PlayerKeys::PLAYER_BUFFERING_START), value)) {
        bufferingType = BUFFERING_START;
    } else if (infoBody.GetIntValue(std::string(PlayerKeys::PLAYER_BUFFERING_END), value)) {
        bufferingType = BUFFERING_END;
    } else if (infoBody.GetIntValue(std::string(PlayerKeys::PLAYER_BUFFERING_PERCENT), value)) {
        bufferingType = BUFFERING_PERCENT;
    } else if (infoBody.GetIntValue(std::string(PlayerKeys::PLAYER_CACHED_DURATION), value)) {
        bufferingType = CACHED_DURATION;
    } else {
        return;
    }

    MEDIA_LOGD("OnBufferingUpdateCb is called, buffering type: %{public}d value: %{public}d", bufferingType, value);

    cb->valueVec.push_back(bufferingType);
    cb->valueVec.push_back(value);
    return OnJsCallBackBufferingUpdate(cb);
}

void PlayerCallbackNapi::OnEosCb(int32_t isLooping) const
{
    MEDIA_LOGD("OnEndOfStream is called, isloop: %{public}d", isLooping);
}

void PlayerCallbackNapi::OnStateChangeCb(PlayerStates state)
{
    MEDIA_LOGD("OnStateChanged is called, current state: %{public}d", state);
    currentState_ = state;

    std::shared_ptr<AutoRef> callback = nullptr;
    std::string callbackName = "unknown";
    switch (state) {
        case PLAYER_PREPARED:
            callback = dataLoadCallback_;
            callbackName = DATA_LOAD_CALLBACK_NAME;
            break;
        case PLAYER_STARTED:
            callback = playCallback_;
            callbackName = PLAY_CALLBACK_NAME;
            break;
        case PLAYER_PAUSED:
            callback = pauseCallback_;
            callbackName = PAUSE_CALLBACK_NAME;
            break;
        case PLAYER_STOPPED:
            callback = stopCallback_;
            callbackName = STOP_CALLBACK_NAME;
            break;
        case PLAYER_IDLE:
            callback = resetCallback_;
            callbackName = RESET_CALLBACK_NAME;
            break;
        case PLAYER_PLAYBACK_COMPLETE:
            callback = finishCallback_;
            callbackName = FINISH_CALLBACK_NAME;
            break;
        default:
            callback = nullptr;
            callbackName = "unknown";
            break;
    }
    CHECK_AND_RETURN_LOG(callback != nullptr, "Cannot find the reference of callback");
    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "No memory");
    cb->callback = callback;
    cb->callbackName = callbackName;
    return OnJsCallBack(cb);
}

void PlayerCallbackNapi::OnPositionUpdateCb(int32_t postion) const
{
    MEDIA_LOGD("OnPositionUpdateCb is called, postion: %{public}d", postion);
}

void PlayerCallbackNapi::OnMessageCb(int32_t type) const
{
    MEDIA_LOGD("OnMessageCb is called, type: %{public}d", type);
}

void PlayerCallbackNapi::OnVolumeChangeCb()
{
    MEDIA_LOGD("OnVolumeChangeCb in");
    CHECK_AND_RETURN_LOG(volumeChangeCallback_ != nullptr, "Cannot find the reference of volumeChange callback");

    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "No memory");
    cb->callback = volumeChangeCallback_;
    cb->callbackName = VOL_CHANGE_CALLBACK_NAME;
    return OnJsCallBack(cb);
}

void PlayerCallbackNapi::OnJsCallBack(PlayerJsCallback *jsCb) const
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (loop == nullptr) {
        delete jsCb;
        return;
    }

    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        MEDIA_LOGE("No memory");
        delete jsCb;
        return;
    }
    work->data = reinterpret_cast<void *>(jsCb);

    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        CHECK_AND_RETURN_LOG(work != nullptr, "work is nullptr");
        PlayerJsCallback *event = reinterpret_cast<PlayerJsCallback *>(work->data);
        std::string request = event->callbackName;
        napi_env env = event->callback->env_;
        napi_ref callback = event->callback->cb_;
        MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work start", request.c_str());
        do {
            CHECK_AND_BREAK_LOG(status != UV_ECANCELED, "%{public}s canceled", request.c_str());
            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(env, callback, &jsCallback);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr, "%{public}s get reference value fail",
                request.c_str());

            // Call back function
            napi_value result = nullptr;
            nstatus = napi_call_function(env, nullptr, jsCallback, 0, nullptr, &result);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "%{public}s fail to napi call function", request.c_str());
        } while (0);
        delete event;
        delete work;
    });
    if (ret != 0) {
        MEDIA_LOGE("Failed to execute libuv work queue");
        delete jsCb;
        delete work;
    }
}

void PlayerCallbackNapi::OnJsCallBackError(PlayerJsCallback *jsCb) const
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (loop == nullptr) {
        delete jsCb;
        return;
    }

    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        MEDIA_LOGE("No memory");
        delete jsCb;
        return;
    }
    work->data = reinterpret_cast<void *>(jsCb);

    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        CHECK_AND_RETURN_LOG(work != nullptr, "work is nullptr");
        PlayerJsCallback *event = reinterpret_cast<PlayerJsCallback *>(work->data);
        std::string request = event->callbackName;
        napi_env env = event->callback->env_;
        napi_ref callback = event->callback->cb_;
        MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work start", request.c_str());
        do {
            CHECK_AND_BREAK_LOG(status != UV_ECANCELED, "%{public}s canceled", request.c_str());
            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(env, callback, &jsCallback);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr, "%{public}s get reference value fail",
                request.c_str());

            napi_value msgValStr = nullptr;
            nstatus = napi_create_string_utf8(env, event->errorMsg.c_str(), NAPI_AUTO_LENGTH, &msgValStr);
            CHECK_AND_RETURN_LOG(nstatus == napi_ok && msgValStr != nullptr, "create error message str fail");

            napi_value args[1] = { nullptr };
            nstatus = napi_create_error(env, nullptr, msgValStr, &args[0]);
            CHECK_AND_RETURN_LOG(nstatus == napi_ok && args[0] != nullptr, "create error callback fail");

            nstatus = CommonNapi::FillErrorArgs(env, static_cast<int32_t>(event->errorCode), args[0]);
            CHECK_AND_RETURN_LOG(nstatus == napi_ok, "create error callback fail");

            // Call back function
            const size_t argCount = 1;
            napi_value result = nullptr;
            nstatus = napi_call_function(env, nullptr, jsCallback, argCount, args, &result);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "%{public}s fail to napi call function", request.c_str());
        } while (0);
        delete event;
        delete work;
    });
    if (ret != 0) {
        MEDIA_LOGE("Failed to execute libuv work queue");
        delete jsCb;
        delete work;
    }
}

void PlayerCallbackNapi::OnJsCallBackInt(PlayerJsCallback *jsCb) const
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (loop == nullptr) {
        delete jsCb;
        return;
    }

    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        MEDIA_LOGE("No memory");
        delete jsCb;
        return;
    }
    work->data = reinterpret_cast<void *>(jsCb);

    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        CHECK_AND_RETURN_LOG(work != nullptr, "work is nullptr");
        PlayerJsCallback *event = reinterpret_cast<PlayerJsCallback *>(work->data);
        std::string request = event->callbackName;
        napi_env env = event->callback->env_;
        napi_ref callback = event->callback->cb_;
        MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work start", request.c_str());
        do {
            CHECK_AND_BREAK_LOG(status != UV_ECANCELED, "%{public}s canceled", request.c_str());

            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(env, callback, &jsCallback);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr, "%{public}s get reference value fail",
                request.c_str());

            CHECK_AND_BREAK_LOG(event->valueVec.size() == 1, "%{public}s get reference value fail", request.c_str());
            // Call back function
            napi_value args[1] = { nullptr };
            nstatus = napi_create_int32(env, event->valueVec[0], &args[0]);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && args[0] != nullptr,
                "%{public}s fail to create callback", request.c_str());

            const size_t argCount = 1;
            napi_value result = nullptr;
            nstatus = napi_call_function(env, nullptr, jsCallback, argCount, args, &result);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "%{public}s fail to call seekDone callback", request.c_str());
        } while (0);
        delete event;
        delete work;
    });
    if (ret != 0) {
        MEDIA_LOGE("Failed to execute libuv work queue");
        delete jsCb;
        delete work;
    }
}

void PlayerCallbackNapi::OnJsCallBackBufferingUpdate(PlayerJsCallback *jsCb) const
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (loop == nullptr) {
        delete jsCb;
        return;
    }

    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        MEDIA_LOGE("No memory");
        delete jsCb;
        return;
    }
    work->data = reinterpret_cast<void *>(jsCb);

    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        CHECK_AND_RETURN_LOG(work != nullptr, "work is nullptr");
        PlayerJsCallback *event = reinterpret_cast<PlayerJsCallback *>(work->data);
        std::string request = event->callbackName;
        napi_env env = event->callback->env_;
        napi_ref callback = event->callback->cb_;
        MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work start", request.c_str());
        do {
            CHECK_AND_BREAK_LOG(status != UV_ECANCELED, "%{public}s canceled", request.c_str());

            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(env, callback, &jsCallback);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr, "%{public}s get reference value fail",
                request.c_str());

            CHECK_AND_BREAK_LOG(event->valueVec.size() == 2, "%{public}s get reference value fail", request.c_str());
            // Call back function
            napi_value args[2] = { nullptr };
            nstatus = napi_create_int32(env, event->valueVec[0], &args[0]);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && args[0] != nullptr,
                "%{public}s fail to create callback", request.c_str());

            nstatus = napi_create_int32(env, event->valueVec[1], &args[1]);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && args[1] != nullptr,
                "%{public}s fail to create callback", request.c_str());

            const size_t argCount = 2;
            napi_value result = nullptr;
            nstatus = napi_call_function(env, nullptr, jsCallback, argCount, args, &result);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "%{public}s fail to call seekDone callback", request.c_str());
        } while (0);
        delete event;
        delete work;
    });
    if (ret != 0) {
        MEDIA_LOGE("Failed to execute libuv work queue");
        delete jsCb;
        delete work;
    }
}
}  // namespace Media
}  // namespace OHOS
