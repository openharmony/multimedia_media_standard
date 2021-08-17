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

#ifndef RECORDER_CALLBACK_NAPI_H_
#define RECORDER_CALLBACK_NAPI_H_

#include "audio_recorder_napi.h"
#include "recorder.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace Media {
const std::string ERROR_CALLBACK_NAME = "error";
class RecorderCallbackNapi : public RecorderCallback {
public:
    RecorderCallbackNapi(napi_env env, AudioRecorderNapi &recorder);
    virtual ~RecorderCallbackNapi();

protected:
    void OnError(int32_t errorType, int32_t errorCode) override;
    void OnInfo(int32_t type, int32_t extra) override;

private:
    napi_env env_;
    AudioRecorderNapi &recorderNapi_;
};
}  // namespace Media
}  // namespace OHOS
#endif // RECORDER_CALLBACK_NAPI_H_