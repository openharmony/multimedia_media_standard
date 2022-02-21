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

#ifndef AVCODEC_NAPI_UTILS_H
#define AVCODEC_NAPI_UTILS_H

#include "avcodec_common.h"
#include "avsharedmemory.h"
#include "format.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace Media {
struct AVCodecJSCallback {
    bool cancelled = false;
};

class AVCodecNapiUtil {
public:
    AVCodecNapiUtil() = delete;
    ~AVCodecNapiUtil() = delete;
    static napi_value CreateInputCodecBuffer(napi_env env, uint32_t index, std::shared_ptr<AVSharedMemory> mem);
    static napi_value CreateOutputCodecBuffer(napi_env env, uint32_t index, std::shared_ptr<AVSharedMemory> memory,
        const AVCodecBufferInfo &info, AVCodecBufferFlag flag);
    static napi_value CreateEmptyEOSBuffer(napi_env env);
    static bool ExtractCodecBuffer(napi_env env, napi_value buffer, int32_t &index, AVCodecBufferInfo &info,
        AVCodecBufferFlag &flag);
    static bool ExtractMediaFormat(napi_env env, napi_value mediaFormat, Format &format);
};
}
}
#endif
