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
const std::string AUDIO_INTERRUPT_CALLBACK_NAME = "audioInterrupt";
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

void PlayerCallbackNapi::SaveCallbackReference(const std::string &name, std::weak_ptr<AutoRef> ref)
{
    std::lock_guard<std::mutex> lock(mutex_);
    refMap_[name] = ref;
}

void PlayerCallbackNapi::SendErrorCallback(MediaServiceExtErrCode errCode, const std::string &info)
{
    MEDIA_LOGE("in ErrorCallback: %{public}s", info.c_str());
    std::lock_guard<std::mutex> lock(mutex_);
    if (refMap_.find(ERROR_CALLBACK_NAME) == refMap_.end()) {
        MEDIA_LOGW("can not find error callback!");
        return;
    }

    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "cb is nullptr");

    cb->callback = refMap_.at(ERROR_CALLBACK_NAME);
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
    MEDIA_LOGD("OnError is called, name: %{public}d, message: %{public}d", errorType, errorCode);
    MediaServiceExtErrCode err = MSErrorToExtError(static_cast<MediaServiceErrCode>(errorCode));
    return SendErrorCallback(err);
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
        case INFO_TYPE_INTERRUPT_EVENT:
            OnAudioInterruptCb(infoBody);
            break;
        default:
            break;
    }
    MEDIA_LOGD("send OnInfo callback success");
}

void PlayerCallbackNapi::OnSeekDoneCb(int32_t currentPositon) const
{
    MEDIA_LOGD("OnSeekDone is called, currentPositon: %{public}d", currentPositon);
    if (refMap_.find(TIME_UPDATE_CALLBACK_NAME) == refMap_.end()) {
        MEDIA_LOGW("can not find timeupdate callback!");
        return;
    }

    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "No memory");
    cb->callback = refMap_.at(TIME_UPDATE_CALLBACK_NAME);
    cb->callbackName = TIME_UPDATE_CALLBACK_NAME;
    cb->valueVec.push_back(currentPositon);
    return OnJsCallBackInt(cb);
}

void PlayerCallbackNapi::OnBufferingUpdateCb(const Format &infoBody) const
{
    if (refMap_.find(BUFFERING_UPDATE_CALLBACK_NAME) == refMap_.end()) {
        MEDIA_LOGW("can not find buffering update callback!");
        return;
    }

    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "No memory");
    cb->callback = refMap_.at(BUFFERING_UPDATE_CALLBACK_NAME);
    cb->callbackName = BUFFERING_UPDATE_CALLBACK_NAME;

    int32_t value = 0;
    int32_t bufferingType = -1;
    if (infoBody.ContainKey(std::string(PlayerKeys::PLAYER_BUFFERING_START))) {
        bufferingType = BUFFERING_START;
        (void)infoBody.GetIntValue(std::string(PlayerKeys::PLAYER_BUFFERING_START), value);
    } else if (infoBody.ContainKey(std::string(PlayerKeys::PLAYER_BUFFERING_END))) {
        bufferingType = BUFFERING_END;
        (void)infoBody.GetIntValue(std::string(PlayerKeys::PLAYER_BUFFERING_END), value);
    } else if (infoBody.ContainKey(std::string(PlayerKeys::PLAYER_BUFFERING_PERCENT))) {
        bufferingType = BUFFERING_PERCENT;
        (void)infoBody.GetIntValue(std::string(PlayerKeys::PLAYER_BUFFERING_PERCENT), value);
    } else if (infoBody.ContainKey(std::string(PlayerKeys::PLAYER_CACHED_DURATION))) {
        bufferingType = CACHED_DURATION;
        (void)infoBody.GetIntValue(std::string(PlayerKeys::PLAYER_CACHED_DURATION), value);
    } else {
        return;
    }

    MEDIA_LOGD("OnBufferingUpdateCb is called, buffering type: %{public}d value: %{public}d", bufferingType, value);

    cb->valueVec.push_back(bufferingType);
    cb->valueVec.push_back(value);
    return OnJsCallBackIntVec(cb);
}

void PlayerCallbackNapi::OnEosCb(int32_t isLooping) const
{
    MEDIA_LOGD("OnEndOfStream is called, isloop: %{public}d", isLooping);
}

void PlayerCallbackNapi::OnStateChangeCb(PlayerStates state)
{
    MEDIA_LOGD("OnStateChanged is called, current state: %{public}d", state);
    currentState_ = state;

    std::string callbackName = "unknown";
    switch (state) {
        case PLAYER_PREPARED:
            callbackName = DATA_LOAD_CALLBACK_NAME;
            break;
        case PLAYER_STARTED:
            callbackName = PLAY_CALLBACK_NAME;
            break;
        case PLAYER_PAUSED:
            callbackName = PAUSE_CALLBACK_NAME;
            break;
        case PLAYER_STOPPED:
            callbackName = STOP_CALLBACK_NAME;
            break;
        case PLAYER_IDLE:
            callbackName = RESET_CALLBACK_NAME;
            break;
        case PLAYER_PLAYBACK_COMPLETE:
            callbackName = FINISH_CALLBACK_NAME;
            break;
        default:
            callbackName = "unknown";
            break;
    }

    if (refMap_.find(callbackName) == refMap_.end()) {
        MEDIA_LOGW("can not find %{public}s callback!", callbackName.c_str());
        return;
    }

    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "No memory");
    cb->callback = refMap_.at(callbackName);
    cb->callbackName = callbackName;
    return OnJsCallBack(cb);
}

void PlayerCallbackNapi::OnPositionUpdateCb(int32_t position) const
{
    MEDIA_LOGD("OnPositionUpdateCb is called, position: %{public}d", position);
}

void PlayerCallbackNapi::OnMessageCb(int32_t type) const
{
    MEDIA_LOGD("OnMessageCb is called, type: %{public}d", type);
}

void PlayerCallbackNapi::OnVolumeChangeCb()
{
    MEDIA_LOGD("OnVolumeChangeCb in");
    if (refMap_.find(VOL_CHANGE_CALLBACK_NAME) == refMap_.end()) {
        MEDIA_LOGW("can not find vol change callback!");
        return;
    }
    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "No memory");
    cb->callback = refMap_.at(VOL_CHANGE_CALLBACK_NAME);
    cb->callbackName = VOL_CHANGE_CALLBACK_NAME;
    return OnJsCallBack(cb);
}

void PlayerCallbackNapi::OnAudioInterruptCb(const Format &infoBody) const
{
    MEDIA_LOGD("OnAudioInterruptCb in");
    if (refMap_.find(AUDIO_INTERRUPT_CALLBACK_NAME) == refMap_.end()) {
        MEDIA_LOGW("can not find audio interrupt callback!");
        return;
    }

    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "No memory");
    cb->callback = refMap_.at(AUDIO_INTERRUPT_CALLBACK_NAME);
    cb->callbackName = AUDIO_INTERRUPT_CALLBACK_NAME;
    int32_t eventType = 0;
    int32_t forceType = 0;
    int32_t hintType = 0;
    (void)infoBody.GetIntValue("eventType", eventType);
    (void)infoBody.GetIntValue("forceType", forceType);
    (void)infoBody.GetIntValue("hintType", hintType);
    cb->interruptEvent.eventType = AudioStandard::InterruptType(eventType);
    cb->interruptEvent.forceType = AudioStandard::InterruptForceType(forceType);
    cb->interruptEvent.hintType = AudioStandard::InterruptHint(hintType);
    return OnJsCallBackInterrupt(cb);
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
        MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work start", request.c_str());
        do {
            CHECK_AND_BREAK_LOG(status != UV_ECANCELED, "%{public}s canceled", request.c_str());
            std::shared_ptr<AutoRef> ref = event->callback.lock();
            CHECK_AND_BREAK_LOG(ref != nullptr, "%{public}s AutoRef is nullptr", request.c_str());

            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr,
                "%{public}s get reference value fail", request.c_str());

            // Call back function
            napi_value result = nullptr;
            nstatus = napi_call_function(ref->env_, nullptr, jsCallback, 0, nullptr, &result);
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
        MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work start", request.c_str());
        do {
            CHECK_AND_BREAK_LOG(status != UV_ECANCELED, "%{public}s canceled", request.c_str());
            std::shared_ptr<AutoRef> ref = event->callback.lock();
            CHECK_AND_BREAK_LOG(ref != nullptr, "%{public}s AutoRef is nullptr", request.c_str());

            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr, "%{public}s get reference value fail",
                request.c_str());

            napi_value msgValStr = nullptr;
            nstatus = napi_create_string_utf8(ref->env_, event->errorMsg.c_str(), NAPI_AUTO_LENGTH, &msgValStr);
            CHECK_AND_RETURN_LOG(nstatus == napi_ok && msgValStr != nullptr, "create error message str fail");

            napi_value args[1] = { nullptr };
            nstatus = napi_create_error(ref->env_, nullptr, msgValStr, &args[0]);
            CHECK_AND_RETURN_LOG(nstatus == napi_ok && args[0] != nullptr, "create error callback fail");

            nstatus = CommonNapi::FillErrorArgs(ref->env_, static_cast<int32_t>(event->errorCode), args[0]);
            CHECK_AND_RETURN_LOG(nstatus == napi_ok, "create error callback fail");

            // Call back function
            napi_value result = nullptr;
            nstatus = napi_call_function(ref->env_, nullptr, jsCallback, 1, args, &result);
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
        MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work start", request.c_str());
        do {
            CHECK_AND_BREAK_LOG(status != UV_ECANCELED, "%{public}s canceled", request.c_str());
            std::shared_ptr<AutoRef> ref = event->callback.lock();
            CHECK_AND_BREAK_LOG(ref != nullptr, "%{public}s AutoRef is nullptr", request.c_str());

            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr, "%{public}s get reference value fail",
                request.c_str());

            CHECK_AND_BREAK_LOG(event->valueVec.size() == 1, "%{public}s get reference value fail", request.c_str());
            // Call back function
            napi_value args[1] = { nullptr };
            nstatus = napi_create_int32(ref->env_, event->valueVec[0], &args[0]);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && args[0] != nullptr,
                "%{public}s fail to create callback", request.c_str());

            napi_value result = nullptr;
            nstatus = napi_call_function(ref->env_, nullptr, jsCallback, 1, args, &result);
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

void PlayerCallbackNapi::OnJsCallBackIntVec(PlayerJsCallback *jsCb) const
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
        MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work start", request.c_str());
        do {
            CHECK_AND_BREAK_LOG(status != UV_ECANCELED, "%{public}s canceled", request.c_str());
            std::shared_ptr<AutoRef> ref = event->callback.lock();
            CHECK_AND_BREAK_LOG(ref != nullptr, "%{public}s AutoRef is nullptr", request.c_str());

            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr, "%{public}s get reference value fail",
                request.c_str());

            CHECK_AND_BREAK_LOG(event->valueVec.size() == 2, "%{public}s get reference value fail", request.c_str());
            // Call back function
            napi_value args[2] = { nullptr };
            nstatus = napi_create_int32(ref->env_, event->valueVec[0], &args[0]);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && args[0] != nullptr,
                "%{public}s fail to create callback", request.c_str());

            nstatus = napi_create_int32(ref->env_, event->valueVec[1], &args[1]);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && args[1] != nullptr,
                "%{public}s fail to create callback", request.c_str());

            const size_t argCount = 2;
            napi_value result = nullptr;
            nstatus = napi_call_function(ref->env_, nullptr, jsCallback, argCount, args, &result);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "%{public}s fail to call callback", request.c_str());
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

void PlayerCallbackNapi::OnJsCallBackIntArray(PlayerJsCallback *jsCb) const
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
        MEDIA_LOGD("JsCallBack %{public}s, size = %{public}zu", request.c_str(), event->valueVec.size());

        do {
            CHECK_AND_BREAK_LOG(status != UV_ECANCELED, "%{public}s canceled", request.c_str());
            std::shared_ptr<AutoRef> ref = event->callback.lock();
            CHECK_AND_BREAK_LOG(ref != nullptr, "%{public}s AutoRef is nullptr", request.c_str());

            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr,
                "%{public}s failed call callback", request.c_str());

            napi_value array = nullptr;
            bool ret = CommonNapi::AddArrayInt(ref->env_, array, event->valueVec);
            CHECK_AND_BREAK_LOG(ret == true, "%{public}s failed call callback", request.c_str());

            napi_value result = nullptr;
            napi_value args[1] = {array};
            nstatus = napi_call_function(ref->env_, nullptr, jsCallback, 1, args, &result);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "%{public}s fail to call callback", request.c_str());
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

void PlayerCallbackNapi::OnJsCallBackInterrupt(PlayerJsCallback *jsCb) const
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (loop == nullptr) {
        delete jsCb;
        return;
    }

    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        delete jsCb;
        return;
    }
    work->data = reinterpret_cast<void *>(jsCb);

    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        CHECK_AND_RETURN_LOG(work != nullptr, "work is nullptr");
        PlayerJsCallback *event = reinterpret_cast<PlayerJsCallback *>(work->data);
        std::string request = event->callbackName;
        MEDIA_LOGD("JsCallBack %{public}s", request.c_str());

        do {
            CHECK_AND_BREAK_LOG(status != UV_ECANCELED, "%{public}s canceled", request.c_str());
            std::shared_ptr<AutoRef> ref = event->callback.lock();
            CHECK_AND_BREAK_LOG(ref != nullptr, "%{public}s AutoRef is nullptr", request.c_str());

            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr,
                "%{public}s failed call callback", request.c_str());
            
            napi_value args[1] = {nullptr};
            napi_create_object(ref->env_, &args[0]);
            CommonNapi::SetPropertyInt32(ref->env_, args[0], "eventType",
                static_cast<int32_t>(event->interruptEvent.eventType));
            CommonNapi::SetPropertyInt32(ref->env_, args[0], "forceType",
                static_cast<int32_t>(event->interruptEvent.forceType));
            CommonNapi::SetPropertyInt32(ref->env_, args[0], "hintType",
                static_cast<int32_t>(event->interruptEvent.hintType));

            napi_value result = nullptr;
            nstatus = napi_call_function(ref->env_, nullptr, jsCallback, 1, args, &result);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "%{public}s fail to call callback", request.c_str());
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
} // namespace Media
} // namespace OHOS
