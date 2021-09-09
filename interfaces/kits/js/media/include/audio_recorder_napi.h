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
#include "media_errors.h"
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

class AudioRecorderNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);

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
    void ErrorCallback(napi_env env, MediaServiceExtErrCode errCode);
    void StateCallback(napi_env env, const std::string &callbackName);
    int32_t SetFormat(napi_env env, napi_value args, int32_t &sourceId);
    int32_t SetAudioProperties(napi_env env, napi_value args, int32_t sourceId);
    int32_t SetUri(napi_env env, napi_value args);
    int32_t CheckValidPath(const std::string &path);

    AudioRecorderNapi();
    ~AudioRecorderNapi();

    static napi_ref constructor_;
    napi_env env_ = nullptr;
    napi_ref wrapper_ = nullptr;
    std::shared_ptr<Recorder> nativeRecorder_ = nullptr;
    std::shared_ptr<RecorderCallback> callbackNapi_ = nullptr;
    std::string filePath_ = "";
};
}  // namespace Media
}  // namespace OHOS
#endif // AUDIO_RECORDER_NAPI_H_
