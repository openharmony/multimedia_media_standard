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

#ifndef AUDIO_RECORDER_NAPI_H_
#define AUDIO_RECORDER_NAPI_H_

#include "recorder.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace Media {
enum JSAudioSourceType : int32_t {
    JS_MIC = 1,
};

enum JSAudioEncoder : int32_t {
    JS_AAC_LC = 1,
};

enum JSFileFormat : int32_t {
    JS_MP4 = 1,
    JS_M4A = 2,
};

class RecorderCallbackNapi;
class AudioRecorderNapi {
public:
    explicit AudioRecorderNapi();
    ~AudioRecorderNapi();

    static napi_value Init(napi_env env, napi_value exports);

    napi_ref errorCallback_ = nullptr;

private:
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static void Destructor(napi_env env, void *nativeObject, void *finalize);
    static napi_value CreateAudioRecorder(napi_env env, napi_callback_info info);
    static napi_value Prepare(napi_env env, napi_callback_info info);
    static napi_value Start(napi_env env, napi_callback_info info);
    static napi_value Pause(napi_env env, napi_callback_info info);
    static napi_value Resume(napi_env env, napi_callback_info info);
    static napi_value Stop(napi_env env, napi_callback_info info);
    static napi_value Reset(napi_env env, napi_callback_info info);
    static napi_value Release(napi_env env, napi_callback_info info);
    static napi_value On(napi_env env, napi_callback_info info);
    void SendCallback(napi_env env, napi_callback_info info, napi_ref callbackRef) const;
    void SendErrorCallback(napi_env env, napi_callback_info info, napi_ref callbackRef,
        const std::string &errCode, const std::string &errType) const;
    int32_t SetFormat(napi_env env, napi_value args, int32_t &sourceId) const;
    int32_t SetAudioProperties(napi_env env, napi_value args, int32_t sourceId) const;
    int32_t SetFilePath(napi_env env, napi_value args) const;
    void GetAudioConfig(napi_env env, napi_value configObj, const std::string &type, int32_t *configItem) const;
    void SaveCallbackReference(napi_env env, AudioRecorderNapi &recorderNapi,
        const std::string &callbackName, napi_value callback) const;

    static napi_ref constructor_;
    napi_ref prepareCallback_ = nullptr;
    napi_ref startCallback_ = nullptr;
    napi_ref pauseCallback_ = nullptr;
    napi_ref resumeCallback_ = nullptr;
    napi_ref stopCallback_ = nullptr;
    napi_ref resetCallback_ = nullptr;
    napi_ref releaseCallback_ = nullptr;
    napi_env env_ = nullptr;
    napi_ref wrapper_ = nullptr;
    std::shared_ptr<Recorder> nativeRecorder_ = nullptr;
    std::shared_ptr<RecorderCallbackNapi> callbackNapi_ = nullptr;
};
}  // namespace Media
}  // namespace OHOS
#endif // AUDIO_RECORDER_NAPI_H_
