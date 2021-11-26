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

#ifndef PLAYER_CALLBACK_NAPI_H_
#define PLAYER_CALLBACK_NAPI_H_

#include "audio_player_napi.h"
#include "player.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "common_napi.h"

namespace OHOS {
namespace Media {
class PlayerCallbackNapi : public PlayerCallback {
public:
    explicit PlayerCallbackNapi(napi_env env);
    virtual ~PlayerCallbackNapi();
    void SaveCallbackReference(const std::string &callbackName, napi_value callback);
    void SendErrorCallback(MediaServiceExtErrCode errCode, const std::string &info = "unknown");
    PlayerStates GetCurrentState() const;
    void OnError(PlayerErrorType errName, int32_t errMsg) override;
    void OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody) override;

private:
    void OnSeekDoneCb(int32_t currentPositon) const;
    void OnEosCb(int32_t isLooping) const;
    void OnStateChangeCb(PlayerStates state);
    void OnPositionUpdateCb(int32_t postion) const;
    void OnMessageCb(int32_t type) const;
    void OnVolumeChangeCb();
    void OnBufferingUpdateCb(const Format &infoBody) const;
    struct PlayerJsCallback {
        std::shared_ptr<AutoRef> callback = nullptr;
        std::string callbackName = "unknown";
        std::string errorMsg = "unknown";
        MediaServiceExtErrCode errorCode = MSERR_EXT_UNKNOWN;
        std::vector<int32_t> valueVec;
    };
    void OnJsCallBack(PlayerJsCallback *jsCb) const;
    void OnJsCallBackError(PlayerJsCallback *jsCb) const;
    void OnJsCallBackInt(PlayerJsCallback *jsCb) const;
    void OnJsCallBackBufferingUpdate(PlayerJsCallback *jsCb) const;
    std::mutex mutex_;
    napi_env env_ = nullptr;
    PlayerStates currentState_ = PLAYER_IDLE;
    std::shared_ptr<AutoRef> errorCallback_ = nullptr; // error
    std::shared_ptr<AutoRef> playCallback_ = nullptr; // started
    std::shared_ptr<AutoRef> pauseCallback_ = nullptr; // paused
    std::shared_ptr<AutoRef> stopCallback_ = nullptr; // stopped
    std::shared_ptr<AutoRef> resetCallback_ = nullptr; // idle
    std::shared_ptr<AutoRef> dataLoadCallback_ = nullptr; // prepared
    std::shared_ptr<AutoRef> finishCallback_ = nullptr; // endofstream
    std::shared_ptr<AutoRef> timeUpdateCallback_ = nullptr; // seekdone
    std::shared_ptr<AutoRef> volumeChangeCallback_ = nullptr; // volumedone
    std::shared_ptr<AutoRef> bufferingUpdateCallback_ = nullptr; // buffering update
};
}  // namespace Media
}  // namespace OHOS
#endif // PLAYER_CALLBACK_NAPI_H_
