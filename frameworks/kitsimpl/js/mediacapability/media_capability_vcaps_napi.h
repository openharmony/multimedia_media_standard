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

#ifndef MEDIA_CAPABILITY_VCAPS_NAPI_H
#define MEDIA_CAPABILITY_VCAPS_NAPI_H

#include "avcodec_list.h"
#include "common_napi.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace Media {
struct MediaVideoCapsAsyncCtx;

class MediaVideoCapsNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);
    static napi_value Create(napi_env env, std::shared_ptr<VideoCaps> caps);

private:
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static void Destructor(napi_env env, void *nativeObject, void *finalize);
    static napi_value IsSizeSupported(napi_env env, napi_callback_info info);
    static napi_value GetSupportedFrameRate(napi_env env, napi_callback_info info);
    static napi_value GetPreferredFrameRate(napi_env env, napi_callback_info info);

    static napi_value GetCodecInfo(napi_env env, napi_callback_info info);
    static napi_value SupportedBitrate(napi_env env, napi_callback_info info);
    static napi_value SupportedFormats(napi_env env, napi_callback_info info);
    static napi_value SupportedHeightAlignment(napi_env env, napi_callback_info info);
    static napi_value SupportedWidthAlignment(napi_env env, napi_callback_info info);
    static napi_value SupportedWidth(napi_env env, napi_callback_info info);
    static napi_value SupportedHeight(napi_env env, napi_callback_info info);
    static napi_value SupportedProfiles(napi_env env, napi_callback_info info);
    static napi_value SupportedLevels(napi_env env, napi_callback_info info);
    static napi_value SupportedBitrateMode(napi_env env, napi_callback_info info);
    static napi_value SupportedQuality(napi_env env, napi_callback_info info);
    static napi_value SupportedComplexity(napi_env env, napi_callback_info info);

    MediaVideoCapsNapi();
    ~MediaVideoCapsNapi();

    static napi_ref constructor_;
    std::shared_ptr<VideoCaps> caps_;
    napi_env env_ = nullptr;
    napi_ref wrap_ = nullptr;
    struct CapsWrap {
        explicit CapsWrap(std::shared_ptr<VideoCaps> caps) : caps_(caps) {}
        ~CapsWrap() = default;
        std::shared_ptr<VideoCaps> caps_;
    };
};

struct MediaVideoCapsAsyncCtx : public MediaAsyncContext {
    explicit MediaVideoCapsAsyncCtx(napi_env env) : MediaAsyncContext(env) {}
    ~MediaVideoCapsAsyncCtx() = default;

    MediaVideoCapsNapi *napi_ = nullptr;
    int32_t width_ = 0;
    int32_t height_ = 0;
};
}  // namespace Media
}  // namespace OHOS
#endif // MEDIA_CAPABILITY_VCAPS_NAPI_H
