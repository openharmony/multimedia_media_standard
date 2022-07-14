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
AudioDecoderCallbackNapi::AudioDecoderCallbackNapi(napi_env env, std::weak_ptr<AVCodecAudioDecoder> adec,
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

void AudioDecoderCallbackNapi::SaveCallbackReference(const std::string &name, std::weak_ptr<AutoRef> ref)
{
    std::lock_guard<std::mutex> lock(mutex_);
    refMap_[name] = ref;
}

void AudioDecoderCallbackNapi::SendErrorCallback(MediaServiceExtErrCode errCode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (refMap_.find(ERROR_CALLBACK_NAME) == refMap_.end()) {
        MEDIA_LOGW("can not find error callback!");
        return;
    }

    AudioDecoderJsCallback *cb = new(std::nothrow) AudioDecoderJsCallback();
    CHECK_AND_RETURN(cb != nullptr);
    cb->callback = refMap_.at(ERROR_CALLBACK_NAME);
    cb->callbackName = ERROR_CALLBACK_NAME;
    cb->errorMsg = MSExtErrorToString(errCode);
    cb->errorCode = errCode;
    return OnJsErrorCallBack(cb);
}

void AudioDecoderCallbackNapi::OnError(AVCodecErrorType errorType, int32_t errCode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("OnError is called, name: %{public}d, error message: %{public}d", errorType, errCode);

    MediaServiceExtErrCode err = MSErrorToExtError(static_cast<MediaServiceErrCode>(errCode));
    return SendErrorCallback(err);
}

void AudioDecoderCallbackNapi::OnOutputFormatChanged(const Format &format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("OnOutputFormatChanged is called");
    if (refMap_.find(FORMAT_CHANGED_CALLBACK_NAME) == refMap_.end()) {
        MEDIA_LOGW("can not find output format changed callback!");
        return;
    }

    AudioDecoderJsCallback *cb = new(std::nothrow) AudioDecoderJsCallback();
    CHECK_AND_RETURN(cb != nullptr);
    cb->callback = refMap_.at(FORMAT_CHANGED_CALLBACK_NAME);
    cb->callbackName = FORMAT_CHANGED_CALLBACK_NAME;
    cb->format = format;
    return OnJsFormatCallBack(cb);
}

void AudioDecoderCallbackNapi::OnInputBufferAvailable(uint32_t index)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (refMap_.find(INPUT_CALLBACK_NAME) == refMap_.end()) {
        MEDIA_LOGW("can not find inputbuffer callback!");
        return;
    }

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

    cb->callback = refMap_.at(INPUT_CALLBACK_NAME);
    cb->callbackName = INPUT_CALLBACK_NAME;
    cb->index = index;
    cb->memory = buffer;
    return OnJsBufferCallBack(cb, true);
}

void AudioDecoderCallbackNapi::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (refMap_.find(OUTPUT_CALLBACK_NAME) == refMap_.end()) {
        MEDIA_LOGW("can not find outputbuffer callback!");
        return;
    }

    auto adec = adec_.lock();
    CHECK_AND_RETURN(adec != nullptr);

    if (codecHelper_->IsStop()) {
        MEDIA_LOGD("At stop state, ignore");
        return;
    }

    std::shared_ptr<AVSharedMemory> buffer = nullptr;
    bool isEos = flag & AVCODEC_BUFFER_FLAG_EOS;
    if (!isEos) {
        buffer = adec->GetOutputBuffer(index);
        if (buffer == nullptr) {
            MEDIA_LOGW("Failed to get output buffer");
            return;
        }
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

    cb->callback = refMap_.at(OUTPUT_CALLBACK_NAME);
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
        MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work start", event->callbackName.c_str());
        do {
            CHECK_AND_BREAK(status != UV_ECANCELED);
            std::shared_ptr<AutoRef> ref = event->callback.lock();
            CHECK_AND_BREAK_LOG(ref != nullptr, "%{public}s AutoRef is nullptr", event->callbackName.c_str());

            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_BREAK(nstatus == napi_ok && jsCallback != nullptr);

            napi_value msgValStr = nullptr;
            nstatus = napi_create_string_utf8(ref->env_, event->errorMsg.c_str(), NAPI_AUTO_LENGTH, &msgValStr);
            CHECK_AND_BREAK(nstatus == napi_ok && msgValStr != nullptr);

            napi_value args[1] = { nullptr };
            nstatus = napi_create_error(ref->env_, nullptr, msgValStr, &args[0]);
            CHECK_AND_BREAK(nstatus == napi_ok && args[0] != nullptr);

            nstatus = CommonNapi::FillErrorArgs(ref->env_, static_cast<int32_t>(event->errorCode), args[0]);
            CHECK_AND_RETURN(nstatus == napi_ok);

            napi_value result = nullptr;
            nstatus = napi_call_function(ref->env_, nullptr, jsCallback, 1, args, &result);
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
        MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work start, index: %{public}u",
            event->callbackName.c_str(), event->index);
        do {
            CHECK_AND_BREAK(!event->cancelled && status != UV_ECANCELED);
            std::shared_ptr<AutoRef> ref = event->callback.lock();
            CHECK_AND_BREAK_LOG(ref != nullptr, "%{public}s AutoRef is nullptr", event->callbackName.c_str());

            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_BREAK(nstatus == napi_ok && jsCallback != nullptr);

            napi_value args[1] = { nullptr };
            if (event->isInput) {
                args[0] = AVCodecNapiUtil::CreateInputCodecBuffer(ref->env_, event->index, event->memory);
            } else {
                args[0] = AVCodecNapiUtil::CreateOutputCodecBuffer(ref->env_, event->index,
                    event->memory, event->info, event->flag);
            }
            CHECK_AND_BREAK(args[0] != nullptr);

            napi_value result = nullptr;
            nstatus = napi_call_function(ref->env_, nullptr, jsCallback, 1, args, &result);
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
        MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work start", event->callbackName.c_str());
        do {
            CHECK_AND_BREAK(status != UV_ECANCELED);
            std::shared_ptr<AutoRef> ref = event->callback.lock();
            CHECK_AND_BREAK_LOG(ref != nullptr, "%{public}s AutoRef is nullptr", event->callbackName.c_str());

            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_BREAK(nstatus == napi_ok && jsCallback != nullptr);

            napi_value args[1] = { nullptr };
            args[0] = CommonNapi::CreateFormatBuffer(ref->env_, event->format);
            CHECK_AND_BREAK(args[0] != nullptr);

            napi_value result = nullptr;
            nstatus = napi_call_function(ref->env_, nullptr, jsCallback, 1, args, &result);
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
