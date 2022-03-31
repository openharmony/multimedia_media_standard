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
#include "media_errors.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "media_data_source_callback.h"
#include "common_napi.h"

namespace OHOS {
namespace Media {
class AudioPlayerNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);

private:
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static void Destructor(napi_env env, void *nativeObject, void *finalize);
    static napi_value CreateAudioPlayer(napi_env env, napi_callback_info info);
    static napi_value CreateAudioPlayerAsync(napi_env env, napi_callback_info info);
    static napi_value Play(napi_env env, napi_callback_info info);
    static napi_value Pause(napi_env env, napi_callback_info info);
    static napi_value Stop(napi_env env, napi_callback_info info);
    static napi_value Reset(napi_env env, napi_callback_info info);
    static napi_value Seek(napi_env env, napi_callback_info info);
    static napi_value SetVolume(napi_env env, napi_callback_info info);
    static napi_value Release(napi_env env, napi_callback_info info);
    static napi_value On(napi_env env, napi_callback_info info);
    static napi_value SetSrc(napi_env env, napi_callback_info info);
    static napi_value GetSrc(napi_env env, napi_callback_info info);
    static napi_value GetMediaDataSrc(napi_env env, napi_callback_info info);
    static napi_value SetMediaDataSrc(napi_env env, napi_callback_info info);
    static napi_value SetFdSrc(napi_env env, napi_callback_info info);
    static napi_value GetFdSrc(napi_env env, napi_callback_info info);
    static napi_value SetLoop(napi_env env, napi_callback_info info);
    static napi_value GetLoop(napi_env env, napi_callback_info info);
    static napi_value GetCurrentTime(napi_env env, napi_callback_info info);
    static napi_value GetDuration(napi_env env, napi_callback_info info);
    static napi_value GetState(napi_env env, napi_callback_info info);
    static napi_value GetTrackDescription(napi_env env, napi_callback_info info);
    static void AsyncGetTrackDescription(napi_env env, void *data);
    void ErrorCallback(MediaServiceExtErrCode errCode);
    AudioPlayerNapi();
    ~AudioPlayerNapi();

    static thread_local napi_ref constructor_;
    std::shared_ptr<MediaDataSourceCallback> dataSrcCallBack_ = nullptr;
    napi_env env_ = nullptr;
    napi_ref wrapper_ = nullptr;
    std::shared_ptr<Player> nativePlayer_ = nullptr;
    std::shared_ptr<PlayerCallback> callbackNapi_ = nullptr;
    std::string uri_ = "";
    std::vector<Format> audioTrackInfoVec_;
    AVFileDescriptor rawFd_;
};
} // namespace Media
} // namespace OHOS
#endif // AUDIO_PLAYER_NAPI_H_
