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

namespace OHOS {
namespace Media {
const std::string PLAY_CALLBACK_NAME = "play";
const std::string PAUSE_CALLBACK_NAME = "pause";
const std::string STOP_CALLBACK_NAME = "stop";
const std::string RESET_CALLBACK_NAME = "reset";
const std::string DATA_LOAD_CALLBACK_NAME = "dataLoad";
const std::string FINISH_CALLBACK_NAME = "finish";
const std::string TIME_UPDATE_CALLBACK_NAME = "timeUpdate";
const std::string ERROR_CALLBACK_NAME = "error";
const std::string VOL_CHANGE_CALLBACK_NAME = "volumeChange";

struct PlayerJsCallback {
    napi_env env = nullptr;
    napi_ref &callback;
    std::string callbackName = "unknown";
    std::string errorType = "unknown";
    std::string errorCode = "unknown";
    int32_t position = -1;
};

class PlayerCallbackNapi : public PlayerCallback {
public:
    PlayerCallbackNapi(napi_env env, AudioPlayerNapi &player);
    virtual ~PlayerCallbackNapi();

protected:
    void OnError(int32_t errorType, int32_t errorCode) override;
    void OnSeekDone(uint64_t currentPositon) override;
    void OnEndOfStream(bool isLooping) override;
    void OnStateChanged(PlayerStates state) override;
    void OnMessage(int32_t type, int32_t extra) override;
    void OnPositionUpdated(uint64_t postion) override;

private:
    void OnJsCallBack(PlayerJsCallback *jsCb);
    void OnJsCallBackError(PlayerJsCallback *jsCb);
    void OnJsCallBackPosition(PlayerJsCallback *jsCb);

private:
    napi_env env_ = nullptr;
    AudioPlayerNapi &playerNapi_;
};
}  // namespace Media
}  // namespace OHOS
#endif // PLAYER_CALLBACK_NAPI_H_