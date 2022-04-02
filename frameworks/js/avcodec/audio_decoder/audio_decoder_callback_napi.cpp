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

#include "audio_decoder_callback_napi.h"
#include <uv.h>
#include "avcodec_napi_utils.h"
#include "media_errors.h"
#include "media_log.h"
#include "scope_guard.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioDecoderCallbackNapi"};
}

namespace OHOS {
namespace Media {
AudioDecoderCallbackNapi::AudioDecoderCallbackNapi(napi_env env, std::weak_ptr<AudioDecoder> adec,
    const std::shared_ptr<AVCodecNapiHelper>& codecHelper)
    : env_(env),
      adec_(adec),
      codecHelper_(codecHelper)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AudioDecoderCallbackNapi::~AudioDecoderCallbackNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void AudioDecoderCallbackNapi::SaveCallbackReference(const std::string &callbackName, napi_value args)
{
    std::lock_guard<std::mutex> lock(mutex_);

    napi_ref callback = nullptr;
    napi_status status = napi_create_reference(env_, args, 1, &callback);
    CHECK_AND_RETURN_LOG(status == napi_ok && callback != nullptr, "Failed to create callback reference");

    std::shared_ptr<AutoRef> cb = std::make_shared<AutoRef>(env_, callback);
    if (callbackName == ERROR_CALLBACK_NAME) {
        errorCallback_ = cb;
    } else if (callbackName == FORMAT_CHANGED_CALLBACK_NAME) {
        formatChangedCallback_ = cb;
    } else if (callbackName == INPUT_CALLBACK_NAME) {
        inputCallback_ = cb;
    } else if (callbackName == OUTPUT_CALLBACK_NAME) {
        outputCallback_ = cb;
    } else {
        MEDIA_LOGW("Unknown callback type: %{public}s", callbackName.c_str());
        return;
    }
}

void AudioDecoderCallbackNapi::SendErrorCallback(MediaServiceExtErrCode errCode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN(errorCallback_ != nullptr);

    AudioDecoderJsCallback *cb = new(std::nothrow) AudioDecoderJsCallback();
    CHECK_AND_RETURN(cb != nullptr);
    cb->callback = errorCallback_;
    cb->callbackName = ERROR_CALLBACK_NAME;
    cb->errorMsg = MSExtErrorToString(errCode);
    cb->errorCode = errCode;
    return OnJsErrorCallBack(cb);
}

void AudioDecoderCallbackNapi::OnError(AVCodecErrorType errorType, int32_t errCode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("OnError is called, name: %{public}d, error message: %{public}d", errorType, errCode);
    CHECK_AND_RETURN(errorCallback_ != nullptr);

    AudioDecoderJsCallback *cb = new(std::nothrow) AudioDecoderJsCallback();
    CHECK_AND_RETURN(cb != nullptr);
    cb->callback = errorCallback_;
    cb->callbackName = ERROR_CALLBACK_NAME;
    cb->errorMsg = MSErrorToExtErrorString(static_cast<MediaServiceErrCode>(errCode));
    cb->errorCode = MSErrorToExtError(static_cast<MediaServiceErrCode>(errCode));
    return OnJsErrorCallBack(cb);
}

void AudioDecoderCallbackNapi::OnOutputFormatChanged(const Format &format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("OnOutputFormatChanged is called");
    CHECK_AND_RETURN(formatChangedCallback_ != nullptr);

    AudioDecoderJsCallback *cb = new(std::nothrow) AudioDecoderJsCallback();
    CHECK_AND_RETURN(cb != nullptr);
    cb->callback = formatChangedCallback_;
    cb->callbackName = FORMAT_CHANGED_CALLBACK_NAME;
    cb->format = format;
    return OnJsFormatCallBack(cb);
}

void AudioDecoderCallbackNapi::OnInputBufferAvailable(uint32_t index)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN(inputCallback_ != nullptr);

    auto adec = adec_.lock();
    CHECK_AND_RETURN(adec != nullptr);

    if (codecHelper_->IsEos() || codecHelper_->IsStop()) {
        MEDIA_LOGD("At eos or stop, no buffer available");
        return;
    }

    auto buffer = adec->GetInputBuffer(index);
    CHECK_AND_RETURN(buffer != nullptr);

    // cache this buffer for this index to make sure that this buffer is valid until when it be
    // obtained to response to OnInputBufferAvailable at next time.
    auto iter = inputBufferCaches_.find(index);
    if (iter == inputBufferCaches_.end()) {
        inputBufferCaches_.emplace(index, buffer);
    } else {
        iter->second = buffer;
    }

    AudioDecoderJsCallback *cb = new(std::nothrow) AudioDecoderJsCallback();
    CHECK_AND_RETURN(cb != nullptr);

    cb->callback = inputCallback_;
    cb->callbackName = INPUT_CALLBACK_NAME;
    cb->index = index;
    cb->memory = buffer;
    return OnJsBufferCallBack(cb, true);
}

void AudioDecoderCallbackNapi::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN(outputCallback_ != nullptr);

    auto adec = adec_.lock();
    CHECK_AND_RETURN(adec != nullptr);

    if (codecHelper_->IsStop()) {
        MEDIA_LOGD("At stop state, ignore");
        return;
    }

    auto buffer = adec->GetOutputBuffer(index);
    bool isEos = flag & AVCODEC_BUFFER_FLAG_EOS;
    if (buffer == nullptr && !isEos) {
        MEDIA_LOGW("Failed to get output buffer");
        return;
    }

    // cache this buffer for this index to make sure that this buffer is valid until the buffer of this index
    // obtained to response to OnInputBufferAvailable at next time.
    auto iter = outputBufferCaches_.find(index);
    if (iter == outputBufferCaches_.end()) {
        outputBufferCaches_.emplace(index, buffer);
    } else {
        iter->second = buffer;
    }

    AudioDecoderJsCallback *cb = new(std::nothrow) AudioDecoderJsCallback();
    CHECK_AND_RETURN(cb != nullptr);

    cb->callback = outputCallback_;
    cb->callbackName = OUTPUT_CALLBACK_NAME;
    cb->memory = buffer;
    cb->index = index;
    cb->info = info;
    cb->flag = flag;
    return OnJsBufferCallBack(cb, false);
}

void AudioDecoderCallbackNapi::OnJsErrorCallBack(AudioDecoderJsCallback *jsCb) const
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (loop == nullptr) {
        MEDIA_LOGE("Fail to get uv event loop");
        delete jsCb;
        return;
    }

    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        MEDIA_LOGE("No memory");
        delete jsCb;
        return;
    }

    work->data = reinterpret_cast<void *>(jsCb);
    // async callback, jsWork and jsWork->data should be heap object.
    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        CHECK_AND_RETURN_LOG(work != nullptr, "Work thread is nullptr");
        AudioDecoderJsCallback *event = reinterpret_cast<AudioDecoderJsCallback *>(work->data);
        napi_env env = event->callback->env_;
        napi_ref callback = event->callback->cb_;
        MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work start", event->callbackName.c_str());
        do {
            CHECK_AND_BREAK(status != UV_ECANCELED);
            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(env, callback, &jsCallback);
            CHECK_AND_BREAK(nstatus == napi_ok && jsCallback != nullptr);

            napi_value msgValStr = nullptr;
            nstatus = napi_create_string_utf8(env, event->errorMsg.c_str(), NAPI_AUTO_LENGTH, &msgValStr);
            CHECK_AND_BREAK(nstatus == napi_ok && msgValStr != nullptr);

            napi_value args[1] = { nullptr };
            nstatus = napi_create_error(env, nullptr, msgValStr, &args[0]);
            CHECK_AND_BREAK(nstatus == napi_ok && args[0] != nullptr);

            nstatus = CommonNapi::FillErrorArgs(env, static_cast<int32_t>(event->errorCode), args[0]);
            CHECK_AND_RETURN(nstatus == napi_ok);

            napi_value result = nullptr;
            nstatus = napi_call_function(env, nullptr, jsCallback, 1, args, &result);
            CHECK_AND_BREAK(nstatus == napi_ok);
        } while (0);
        delete event;
        delete work;
    });
    if (ret != 0) {
        MEDIA_LOGE("Failed to execute libuv work queue");
        delete jsCb;
        delete work;
    }
}

void AudioDecoderCallbackNapi::OnJsBufferCallBack(AudioDecoderJsCallback *jsCb, bool isInput) const
{
    ON_SCOPE_EXIT(0) { delete jsCb; };

    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    CHECK_AND_RETURN_LOG(loop != nullptr, "Fail to get uv event loop");

    uv_work_t *work = new(std::nothrow) uv_work_t;
    CHECK_AND_RETURN_LOG(work != nullptr, "No memory");

    codecHelper_->PushWork(jsCb);
    jsCb->isInput = isInput;
    jsCb->codecHelper = codecHelper_;
    work->data = reinterpret_cast<void *>(jsCb);
    // async callback, jsWork and jsWork->data should be heap object.
    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        CHECK_AND_RETURN_LOG(work != nullptr, "Work thread is nullptr");
        AudioDecoderJsCallback *event = reinterpret_cast<AudioDecoderJsCallback *>(work->data);
        napi_env env = event->callback->env_;
        MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work start, index: %{public}u",
            event->callbackName.c_str(), event->index);
        do {
            CHECK_AND_BREAK(!event->cancelled && status != UV_ECANCELED);
            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(env, event->callback->cb_, &jsCallback);
            CHECK_AND_BREAK(nstatus == napi_ok && jsCallback != nullptr);

            napi_value args[1] = { nullptr };
            if (event->isInput) {
                args[0] = AVCodecNapiUtil::CreateInputCodecBuffer(env, event->index, event->memory);
            } else {
                args[0] = AVCodecNapiUtil::CreateOutputCodecBuffer(env, event->index,
                    event->memory, event->info, event->flag);
            }
            CHECK_AND_BREAK(args[0] != nullptr);

            napi_value result = nullptr;
            nstatus = napi_call_function(env, nullptr, jsCallback, 1, args, &result);
            CHECK_AND_BREAK(nstatus == napi_ok);
        } while (0);
        auto codecHelper = event->codecHelper.lock();
        if (codecHelper != nullptr) {
            codecHelper->RemoveWork(event);
        }
        delete event;
        delete work;
    });
    if (ret != 0) {
        MEDIA_LOGE("Failed to execute libuv work queue");
        codecHelper_->RemoveWork(jsCb);
        delete jsCb;
        delete work;
    }
    CANCEL_SCOPE_EXIT_GUARD(0);
}

void AudioDecoderCallbackNapi::OnJsFormatCallBack(AudioDecoderJsCallback *jsCb) const
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (loop == nullptr) {
        MEDIA_LOGE("Fail to get uv event loop");
        delete jsCb;
        return;
    }

    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        MEDIA_LOGE("No memory");
        delete jsCb;
        return;
    }

    work->data = reinterpret_cast<void *>(jsCb);
    // async callback, jsWork and jsWork->data should be heap object.
    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        CHECK_AND_RETURN_LOG(work != nullptr, "Work thread is nullptr");
        AudioDecoderJsCallback *event = reinterpret_cast<AudioDecoderJsCallback *>(work->data);
        napi_env env = event->callback->env_;
        MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work start", event->callbackName.c_str());
        do {
            CHECK_AND_BREAK(status != UV_ECANCELED);
            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(env, event->callback->cb_, &jsCallback);
            CHECK_AND_BREAK(nstatus == napi_ok && jsCallback != nullptr);

            napi_value args[1] = { nullptr };
            args[0] = CommonNapi::CreateFormatBuffer(env, event->format);
            CHECK_AND_BREAK(args[0] != nullptr);

            napi_value result = nullptr;
            nstatus = napi_call_function(env, nullptr, jsCallback, 1, args, &result);
            CHECK_AND_BREAK(nstatus == napi_ok);
        } while (0);
        delete event;
        delete work;
    });
    if (ret != 0) {
        MEDIA_LOGE("Failed to execute libuv work queue");
        delete jsCb;
        delete work;
    }
}
} // namespace Media
} // namespace OHOS
