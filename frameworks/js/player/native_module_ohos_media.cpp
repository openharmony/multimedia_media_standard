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

#include "native_module_ohos_media.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "JSMediaModule"};
}

/*
 * Function registering all props and functions of ohos.media module
 * which involves player and the recorder
 */
static napi_value Export(napi_env env, napi_value exports)
{
    MEDIA_LOGD("Export() is called");
    OHOS::Media::AudioDecoderNapi::Init(env, exports);
    OHOS::Media::AudioEncoderNapi::Init(env, exports);
    OHOS::Media::AudioPlayerNapi::Init(env, exports);
    OHOS::Media::AudioRecorderNapi::Init(env, exports);
    OHOS::Media::MediaCapsNapi::Init(env, exports);
    OHOS::Media::MediaVideoCapsNapi::Init(env, exports);
    OHOS::Media::MediaDataSourceNapi::Init(env, exports);
    OHOS::Media::VideoDecoderNapi::Init(env, exports);
    OHOS::Media::VideoEncoderNapi::Init(env, exports);
    OHOS::Media::VideoPlayerNapi::Init(env, exports);
    OHOS::Media::VideoRecorderNapi::Init(env, exports);
    OHOS::Media::MediaEnumNapi::Init(env, exports);
    return exports;
}

/*
 * module define
 */
static napi_module g_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Export,
    .nm_modname = "multimedia.media",
    .nm_priv = ((void*)0),
    .reserved = {0}
};

/*
 * module register
 */
extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    MEDIA_LOGD("RegisterModule() is called");
    napi_module_register(&g_module);
}
