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

#ifndef AVMUXER_NAPI_H
#define AVMUXER_NAPI_H

#include "avmuxer.h"
#include "media_errors.h"
#include "napi/native_api.h"

namespace OHOS {
namespace Media {
class AVMuxerNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);
private:
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static void Destructor(napi_env env, void *nativeObject, void *finalize);
    static napi_value CreateAVMuxer(napi_env env, napi_callback_info info);
    static void AsyncSetOutput(napi_env env, void *data);
    static napi_value SetOutput(napi_env env, napi_callback_info info);
    static napi_value SetLocation(napi_env env, napi_callback_info info);
    static napi_value SetOrientationHint(napi_env env, napi_callback_info info);
    static void AsyncAddTrack(napi_env env, void *data);
    static napi_value AddTrack(napi_env env, napi_callback_info info);
    static void AsyncStart(napi_env env, void *data);
    static napi_value Start(napi_env env, napi_callback_info info);
    static void AsyncWriteTrackSample(napi_env env, void *data);
    static napi_value WriteTrackSample(napi_env env, napi_callback_info info);
    static void AsyncStop(napi_env env, void *data);
    static napi_value Stop(napi_env env, napi_callback_info info);
    static void AsyncRelease(napi_env env, void *data);
    static napi_value Release(napi_env env, napi_callback_info info);

    AVMuxerNapi();
    ~AVMuxerNapi();
    
    static thread_local napi_ref constructor_;
    napi_env env_ = nullptr;
    napi_ref wrapper_ = nullptr;
    std::shared_ptr<AVMuxer> avmuxerImpl_ = nullptr;
};
} // namespace Media
} // namespace OHOS
#endif // AVMUXER_NAPI_H
