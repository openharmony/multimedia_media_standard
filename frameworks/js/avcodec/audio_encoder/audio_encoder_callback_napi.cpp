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

#include "audio_encoder_callback_napi.h"
#include <uv.h>
#include "avcodec_napi_utils.h"
#include "media_errors.h"
#include "media_log.h"
#include "scope_guard.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioEncoderCallbackNapi"};
}

namespace OHOS {
namespace Media {
AudioEncoderCallbackNapi::AudioEncoderCallbackNapi(napi_env env, std::weak_ptr<AVCodecAudioEncoder> aenc,
    const std::shared_ptr<AVCodecNapiHelper>& codecHelper)
    : env_(env),
      aenc_(aenc),
      codecHelper_(codecHelper)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AudioEncoderCallbackNapi::~AudioEncoderCallbackNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void AudioEncoderCallbackNapi::SaveCallbackReference(const std::string &name, std::weak_ptr<AutoRef> ref)
{
    std::lock_guard<std::mutex> lock(mutex_);
    refMap_[name] = ref;
}

void AudioEncoderCallbackNapi::SendErrorCallback(MediaServiceExtErrCode errCode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (refMap_.find(ERROR_CALLBACK_NAME) == refMap_.end()) {
        MEDIA_LOGW("can not find error callback!");
        return;
    }

    AudioEncoderJsCallback *cb = new(std::nothrow) AudioEncoderJsCallback();
    CHECK_AND_RETURN(cb != nullptr);
    cb->callback = refMap_.at(ERROR_CALLBACK_NAME);
    cb->callbackName = ERROR_CALLBACK_NAME;
    cb->errorMsg = MSExtErrorToString(errCode);
    cb->errorCode = errCode;
    return OnJsErrorCallBack(cb);
}

void AudioEncoderCallbackNapi::OnError(AVCodecErrorType errorType, int32_t errCode)
{
    MEDIA_LOGD("OnError is called, name: %{public}d, error message: %{public}d", errorType, errCode);

    MediaServiceExtErrCode err = MSErrorToExtError(static_cast<MediaServiceErrCode>(errCode));
    return SendErrorCallback(err);
}

void AudioEncoderCallbackNapi::OnOutputFormatChanged(const Format &format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("OnOutputFormatChanged is called");
    if (refMap_.find(FORMAT_CHANGED_CALLBACK_NAME) == refMap_.end()) {
        MEDIA_LOGW("can not format changed callback!");
        return;
    }

    AudioEncoderJsCallback *cb = new(std::nothrow) AudioEncoderJsCallback();
    CHECK_AND_RETURN(cb != nullptr);
    cb->callback = refMap_.at(FORMAT_CHANGED_CALLBACK_NAME);
    cb->callbackName = FORMAT_CHANGED_CALLBACK_NAME;
    cb->format = format;
    return OnJsFormatCallBack(cb);
}

void AudioEncoderCallbackNapi::OnInputBufferAvailable(uint32_t index)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (refMap_.find(FORMAT_CHANGED_CALLBACK_NAME) == refMap_.end()) {
        MEDIA_LOGW("can not input buffer callback!");
        return;
    }

    auto aenc = aenc_.lock();
    CHECK_AND_RETURN(aenc != nullptr);

    if (codecHelper_->IsEos() || codecHelper_->IsStop()) {
        MEDIA_LOGD("At eos or stop, no buffer available");
        return;
    }

    auto buffer = aenc->GetInputBuffer(index);
    CHECK_AND_RETURN(buffer != nullptr);

    // cache this buffer for this index to make sure that this buffer is valid until when it be
    // obtained to response to OnInputBufferAvailable at next time.
    auto iter = inputBufferCaches_.find(index);
    if (iter == inputBufferCaches_.end()) {
        inputBufferCaches_.emplace(index, buffer);
    } else {
        iter->second = buffer;
    }

    AudioEncoderJsCallback *cb = new(std::nothrow) AudioEncoderJsCallback();
    CHECK_AND_RETURN(cb != nullptr);

    cb->callback = refMap_.at(INPUT_CALLBACK_NAME);
    cb->callbackName = INPUT_CALLBACK_NAME;
    cb->index = index;
    cb->memory = buffer;
    return OnJsBufferCallBack(cb, true);
}

void AudioEncoderCallbackNapi::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (refMap_.find(OUTPUT_CALLBACK_NAME) == refMap_.end()) {
        MEDIA_LOGW("can not output buffer callback!");
        return;
    }

    auto aenc = aenc_.lock();
    CHECK_AND_RETURN(aenc != nullptr);

    if (codecHelper_->IsStop()) {
        MEDIA_LOGD("At stop state, ignore");
        return;
    }

    std::shared_ptr<AVSharedMemory> buffer = nullptr;
    bool isEos = flag & AVCODEC_BUFFER_FLAG_EOS;
    if (!isEos) {
        buffer = aenc->GetOutputBuffer(index);
        if (buffer == nullptr) {
            MEDIA_LOGW("Failed to get output buffer");
            return;
        }
    }

    // cache this buffer for this index to make sure that this buffer is valid until the buffer of this index
    // obtained to response to OnOutputBufferAvailable at next time.
    auto iter = outputBufferCaches_.find(index);
    if (iter == outputBufferCaches_.end()) {
        outputBufferCaches_.emplace(index, buffer);
    } else {
        iter->second = buffer;
    }

    AudioEncoderJsCallback *cb = new(std::nothrow) AudioEncoderJsCallback();
    CHECK_AND_RETURN(cb != nullptr);

    cb->callback = refMap_.at(OUTPUT_CALLBACK_NAME);
    cb->callbackName = OUTPUT_CALLBACK_NAME;
    cb->memory = buffer;
    cb->index = index;
    cb->info = info;
    cb->flag = flag;
    return OnJsBufferCallBack(cb, false);
}

void AudioEncoderCallbackNapi::OnJsErrorCallBack(AudioEncoderJsCallback *jsCb) const
{
    ON_SCOPE_EXIT(0) { delete jsCb; };

    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    CHECK_AND_RETURN_LOG(loop != nullptr, "Fail to get uv event loop");

    uv_work_t *work = new(std::nothrow) uv_work_t;
    CHECK_AND_RETURN_LOG(work != nullptr, "Fail to new uv_work_t");

    work->data = reinterpret_cast<void *>(jsCb);
    // async callback, jsWork and jsWork->data should be heap object.
    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        CHECK_AND_RETURN_LOG(work != nullptr, "Work thread is nullptr");
        AudioEncoderJsCallback *event = reinterpret_cast<AudioEncoderJsCallback *>(work->data);
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
    CANCEL_SCOPE_EXIT_GUARD(0);
}

void AudioEncoderCallbackNapi::OnJsBufferCallBack(AudioEncoderJsCallback *jsCb, bool isInput) const
{
    ON_SCOPE_EXIT(0) { delete jsCb; };

    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    CHECK_AND_RETURN_LOG(loop != nullptr, "Fail to get uv event loop");

    uv_work_t *work = new(std::nothrow) uv_work_t;
    CHECK_AND_RETURN_LOG(work != nullptr, "Fail to new uv_work_t");

    codecHelper_->PushWork(jsCb);
    jsCb->isInput = isInput;
    jsCb->codecHelper = codecHelper_;
    work->data = reinterpret_cast<void *>(jsCb);
    // async callback, jsWork and jsWork->data should be heap object.
    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        CHECK_AND_RETURN_LOG(work != nullptr, "Work thread is nullptr");
        AudioEncoderJsCallback *event = reinterpret_cast<AudioEncoderJsCallback *>(work->data);
        MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work start", event->callbackName.c_str());
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

void AudioEncoderCallbackNapi::OnJsFormatCallBack(AudioEncoderJsCallback *jsCb) const
{
    ON_SCOPE_EXIT(0) { delete jsCb; };

    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    CHECK_AND_RETURN_LOG(loop != nullptr, "Fail to get uv event loop");

    uv_work_t *work = new(std::nothrow) uv_work_t;
    CHECK_AND_RETURN_LOG(work != nullptr, "Fail to new uv_work_t");

    work->data = reinterpret_cast<void *>(jsCb);
    // async callback, jsWork and jsWork->data should be heap object.
    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        CHECK_AND_RETURN_LOG(work != nullptr, "Work thread is nullptr");
        AudioEncoderJsCallback *event = reinterpret_cast<AudioEncoderJsCallback *>(work->data);
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
    CANCEL_SCOPE_EXIT_GUARD(0);
}
} // namespace Media
} // namespace OHOS
