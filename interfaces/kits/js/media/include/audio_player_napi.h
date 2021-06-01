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

#ifndef AUDIO_PLAYER_NAPI_H_
#define AUDIO_PLAYER_NAPI_H_

#include "player.h"
#include <gst/gst.h>
#include "napi/native_api.h"
#include "napi/native_node_api.h"

static const std::string AP_CALLBACK_PLAY = "play";
static const std::string AP_CALLBACK_PAUSE = "pause";
static const std::string AP_CALLBACK_STOP = "stop";
static const std::string AP_CALLBACK_DATA_LOAD = "dataLoad";
static const std::string AP_CALLBACK_FINISH = "finish";
static const std::string AP_CALLBACK_TIME_UPDATE = "timeUpdate";
static const std::string AP_CALLBACK_ERROR = "error";
static const std::string AP_CALLBACK_VOL_CHANGE = "volumeChange";

static const std::string AUDIO_PLAYER_NAPI_CLASS_NAME = "AudioPlayer";

static const std::string AP_STATE_PLAYING = "playing";
static const std::string AP_STATE_PAUSED = "paused";
static const std::string AP_STATE_STOPPED = "stopped";

static const std::string AUDIO_MIME = "audio/x-raw";
static const std::string VIDEO_MIME = "video/x-raw";

class AudioPlayerNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);
    std::shared_ptr<OHOS::Media::Player> GetAudioPlayerInstance();
    napi_ref GetErrorCallbackRef();
    napi_ref GetPlayCallbackRef();
    napi_ref GetPauseCallbackRef();
    napi_ref GetFinishCallbackRef();
    napi_ref GetStopCallbackRef();
    void SetCurrentState(OHOS::Media::Player::State state);

private:
    explicit AudioPlayerNapi();
    ~AudioPlayerNapi();

    static void Destructor(napi_env env, void* nativeObject, void* finalize_hint);
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static napi_value CreateAudioPlayerWrapper(napi_env env);

    // Static function to return AudioPlayer instance to JS domain
    static napi_value CreateAudioPlayer(napi_env env, napi_callback_info info);

    // Functions in AudioPlayer
    static napi_value Play(napi_env env, napi_callback_info info);
    static napi_value Pause(napi_env env, napi_callback_info info);
    static napi_value Stop(napi_env env, napi_callback_info info);
    static napi_value Release(napi_env env, napi_callback_info info);
    static napi_value Seek(napi_env env, napi_callback_info info);
    static napi_value On(napi_env env, napi_callback_info info);
    static napi_value SetVolume(napi_env env, napi_callback_info info);

    // src property
    static napi_value SetSrc(napi_env env, napi_callback_info info);
    static napi_value GetSrc(napi_env env, napi_callback_info info);

    // loop property
    static napi_value SetLoop(napi_env env, napi_callback_info info);
    static napi_value GetLoop(napi_env env, napi_callback_info info);

    // current time property (readonly)
    static napi_value GetCurrenTime(napi_env env, napi_callback_info info);

    // duration property (readonly)
    static napi_value GetDuration(napi_env env, napi_callback_info info);

    // state property (readonly)
    static napi_value GetState(napi_env env, napi_callback_info info);

    void SaveCallbackReference(napi_env env, AudioPlayerNapi* audioPlayer,
                               std::string callbackName, napi_value callback);

    // Player instance to interact with native layer
    std::shared_ptr<OHOS::Media::Player> player_;

    // Required references for napi function handling
    napi_env env_;
    napi_ref wrapper_;

    static napi_ref sConstructor_;

    void *nativeCallback_;
    napi_ref errorCallback_ = nullptr;
    napi_ref playCallback_ = nullptr;
    napi_ref pauseCallback_ = nullptr;
    napi_ref stopCallback_ = nullptr;
    napi_ref dataLoadCallback_ = nullptr;
    napi_ref finishCallback_ = nullptr;
    napi_ref timeUpdateCallback_ = nullptr;
    napi_ref volumeChangeCallback_ = nullptr;

    std::string uri_;
    OHOS::Media::Player::State currentState_ = OHOS::Media::Player::PLAYER_STATE_STOPPED;
};

// Callback from native
struct PlayerCallback : public OHOS::Media::Player::PlayerCallback {
    napi_env env_;
    AudioPlayerNapi *playerWrapper_;

    explicit PlayerCallback(napi_env environment = nullptr, AudioPlayerNapi *playerWrapper = nullptr);
    virtual ~PlayerCallback() {}

    void OnError(int32_t errorType, int32_t errCode,
                 const std::shared_ptr<OHOS::Media::Player> &player) const;
    void OnEndOfStream(const std::shared_ptr<OHOS::Media::Player> &player) const;
    void OnStateChanged(OHOS::Media::Player::State state,
                        const std::shared_ptr<OHOS::Media::Player> &player) const;
};

#endif /* AUDIO_PLAYER_NAPI_H_ */
