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

#ifndef VIDEO_ENCODER_CALLBACK_NAPI_H
#define VIDEO_ENCODER_CALLBACK_NAPI_H

#include "avcodec_video_encoder.h"
#include "common_napi.h"
#include "video_encoder_napi.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace Media {
const std::string ERROR_CALLBACK_NAME = "error";
const std::string FORMAT_CHANGED_CALLBACK_NAME = "streamChanged";
const std::string INPUT_CALLBACK_NAME = "needInputData";
const std::string OUTPUT_CALLBACK_NAME = "newOutputData";

class VideoEncoderCallbackNapi : public AVCodecCallback {
public:
    explicit VideoEncoderCallbackNapi(napi_env env, std::weak_ptr<AVCodecVideoEncoder> venc);
    virtual ~VideoEncoderCallbackNapi();

    void SaveCallbackReference(const std::string &name, std::weak_ptr<AutoRef> ref);
    void SendErrorCallback(MediaServiceExtErrCode errCode);

protected:
    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format &format) override;
    void OnInputBufferAvailable(uint32_t index) override;
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) override;

private:
    struct VideoEncoderJsCallback {
        std::weak_ptr<AutoRef> callback;
        std::string callbackName = "unknown";
        std::string errorMsg = "unknown";
        MediaServiceExtErrCode errorCode = MSERR_EXT_UNKNOWN;
        uint32_t index = 0;
        AVCodecBufferInfo info;
        AVCodecBufferFlag flag;
        std::shared_ptr<AVSharedMemory> memory = nullptr;
        bool isInput = false;
        Format format;
    };
    void OnJsErrorCallBack(VideoEncoderJsCallback *jsCb) const;
    void OnJsBufferCallBack(VideoEncoderJsCallback *jsCb, bool isInput) const;
    void OnJsFormatCallBack(VideoEncoderJsCallback *jsCb) const;

    std::mutex mutex_;
    napi_env env_ = nullptr;
    std::weak_ptr<AVCodecVideoEncoder> venc_;
    std::map<std::string, std::weak_ptr<AutoRef>> refMap_;
};
} // namespace Media
} // namespace OHOS
#endif // VIDEO_ENCODER_CALLBACK_NAPI_H
