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

#ifndef AUDIO_DECODER_CALLBACK_NAPI_H
#define AUDIO_DECODER_CALLBACK_NAPI_H

#include "audio_decoder_napi.h"
#include "avcodec_audio_decoder.h"
#include "common_napi.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "avcodec_napi_helper.h"

namespace OHOS {
namespace Media {
const std::string ERROR_CALLBACK_NAME = "error";
const std::string FORMAT_CHANGED_CALLBACK_NAME = "outputFormatChanged";
const std::string INPUT_CALLBACK_NAME = "inputBufferAvailable";
const std::string OUTPUT_CALLBACK_NAME = "outputBufferAvailable";

class AudioDecoderCallbackNapi : public AVCodecCallback {
public:
    explicit AudioDecoderCallbackNapi(napi_env env, std::weak_ptr<AudioDecoder> adec,
        const std::shared_ptr<AVCodecNapiHelper>& codecHelper);
    virtual ~AudioDecoderCallbackNapi();

    void SaveCallbackReference(const std::string &callbackName, napi_value callback);
    void SendErrorCallback(MediaServiceExtErrCode errCode);

protected:
    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format &format) override;
    void OnInputBufferAvailable(uint32_t index) override;
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) override;

private:
    struct AudioDecoderJsCallback {
        std::shared_ptr<AutoRef> callback = nullptr;
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
    void OnJsErrorCallBack(AudioDecoderJsCallback *jsCb) const;
    void OnJsBufferCallBack(AudioDecoderJsCallback *jsCb, bool isInput) const;
    void OnJsFormatCallBack(AudioDecoderJsCallback *jsCb) const;

    std::mutex mutex_;
    napi_env env_ = nullptr;
    std::weak_ptr<AudioDecoder> adec_;
    std::shared_ptr<AutoRef> errorCallback_ = nullptr;
    std::shared_ptr<AutoRef> formatChangedCallback_ = nullptr;
    std::shared_ptr<AutoRef> inputCallback_ = nullptr;
    std::shared_ptr<AutoRef> outputCallback_ = nullptr;
    std::shared_ptr<AVCodecNapiHelper> codecHelper_ = nullptr;
    std::unordered_map<uint32_t, std::shared_ptr<AVSharedMemory>> inputBufferCaches_;
    std::unordered_map<uint32_t, std::shared_ptr<AVSharedMemory>> outputBufferCaches_;
};
}  // namespace Media
}  // namespace OHOS
#endif // AUDIO_DECODER_CALLBACK_NAPI_H
