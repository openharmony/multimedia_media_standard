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

#ifndef VIDEO_DECODER_NAPI_H
#define VIDEO_DECODER_NAPI_H

#include "avcodec_video_decoder.h"
#include "common_napi.h"
#include "media_errors.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace Media {
struct VideoDecoderAsyncContext;

class VideoDecoderNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);

private:
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static void Destructor(napi_env env, void *nativeObject, void *finalize);
    static napi_value CreateVideoDecoderByMime(napi_env env, napi_callback_info info);
    static napi_value CreateVideoDecoderByName(napi_env env, napi_callback_info info);
    static napi_value Configure(napi_env env, napi_callback_info info);
    static napi_value Prepare(napi_env env, napi_callback_info info);
    static napi_value Start(napi_env env, napi_callback_info info);
    static napi_value Stop(napi_env env, napi_callback_info info);
    static napi_value Flush(napi_env env, napi_callback_info info);
    static napi_value Reset(napi_env env, napi_callback_info info);
    static napi_value QueueInput(napi_env env, napi_callback_info info);
    static napi_value ReleaseOutput(napi_env env, napi_callback_info info);
    static napi_value SetOutputSurface(napi_env env, napi_callback_info info);
    static napi_value SetParameter(napi_env env, napi_callback_info info);
    static napi_value GetOutputMediaDescription(napi_env env, napi_callback_info info);
    static napi_value GetVideoDecoderCaps(napi_env env, napi_callback_info info);
    static napi_value On(napi_env env, napi_callback_info info);

    void ErrorCallback(MediaServiceExtErrCode errCode);

    VideoDecoderNapi();
    ~VideoDecoderNapi();

    static napi_ref constructor_;
    napi_env env_ = nullptr;
    napi_ref wrap_ = nullptr;
    std::shared_ptr<VideoDecoder> vdec_ = nullptr;
    std::shared_ptr<AVCodecCallback> callback_ = nullptr;
    bool isSurfaceMode_ = false;
};

struct VideoDecoderAsyncContext : public MediaAsyncContext {
    explicit VideoDecoderAsyncContext(napi_env env) : MediaAsyncContext(env) {}
    ~VideoDecoderAsyncContext() = default;
    // general variable
    VideoDecoderNapi *napi = nullptr;
    sptr<Surface> surface;
    // used by constructor
    std::string pluginName;
    int32_t createByMime = 1;
    // used by buffer function
    int32_t index = 0;
    bool isRender = false;
    AVCodecBufferInfo info;
    AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_NONE;
    // used by format
    Format format;
};
}  // namespace Media
}  // namespace OHOS
#endif // VIDEO_DECODER_NAPI_H
