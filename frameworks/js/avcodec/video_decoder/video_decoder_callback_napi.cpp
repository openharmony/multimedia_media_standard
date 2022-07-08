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

#include "video_decoder_callback_napi.h"
#include <uv.h>
#include "avcodec_napi_utils.h"
#include "media_errors.h"
#include "media_log.h"
#include "scope_guard.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoDecoderCallbackNapi"};
}

namespace OHOS {
namespace Media {
VideoDecoderCallbackNapi::VideoDecoderCallbackNapi(napi_env env, std::weak_ptr<AVCodecVideoDecoder> vdec,
    const std::shared_ptr<AVCodecNapiHelper>& codecHelper)
    : env_(env),
      vdec_(vdec),
      codecHelper_(codecHelper)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

VideoDecoderCallbackNapi::~VideoDecoderCallbackNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void VideoDecoderCallbackNapi::SaveCallbackReference(const std::string &name, std::weak_ptr<AutoRef> ref)
{
    std::lock_guard<std::mutex> lock(mutex_);
    refMap_[name] = ref;
}

void VideoDecoderCallbackNapi::SendErrorCallback(MediaServiceExtErrCode errCode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (refMap_.find(ERROR_CALLBACK_NAME) == refMap_.end()) {
        MEDIA_LOGW("can not find error callback!");
        return;
    }

    VideoDecoderJsCallback *cb = new(std::nothrow) VideoDecoderJsCallback();
    CHECK_AND_RETURN(cb != nullptr);
    cb->callback = refMap_.at(ERROR_CALLBACK_NAME);
    cb->callbackName = ERROR_CALLBACK_NAME;
    cb->errorMsg = MSExtErrorToString(errCode);
    cb->errorCode = errCode;
    return OnJsErrorCallBack(cb);
}

void VideoDecoderCallbackNapi::OnError(AVCodecErrorType errorType, int32_t errCode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("OnError is called, name: %{public}d, error message: %{public}d", errorType, errCode);

    MediaServiceExtErrCode err = MSErrorToExtError(static_cast<MediaServiceErrCode>(errCode));
    return SendErrorCallback(err);
}

void VideoDecoderCallbackNapi::OnOutputFormatChanged(const Format &format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("OnOutputFormatChanged is called");
    if (refMap_.find(FORMAT_CHANGED_CALLBACK_NAME) == refMap_.end()) {
        MEDIA_LOGW("can not find ouput format changed callback!");
        return;
    }
    VideoDecoderJsCallback *cb = new(std::nothrow) VideoDecoderJsCallback();
    CHECK_AND_RETURN(cb != nullptr);
    cb->callback = refMap_.at(FORMAT_CHANGED_CALLBACK_NAME);
    cb->callbackName = FORMAT_CHANGED_CALLBACK_NAME;
    cb->format = format;
    return OnJsFormatCallBack(cb);
}

void VideoDecoderCallbackNapi::OnInputBufferAvailable(uint32_t index)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (refMap_.find(INPUT_CALLBACK_NAME) == refMap_.end()) {
        MEDIA_LOGW("can not find input buffer callback!");
        return;
    }
    auto vdec = vdec_.lock();
    CHECK_AND_RETURN(vdec != nullptr);
    if (codecHelper_->IsEos() || codecHelper_->IsStop()) {
        MEDIA_LOGD("At eos or Stop, no buffer available");
        return;
    }

    auto buffer = vdec->GetInputBuffer(index);
    CHECK_AND_RETURN(buffer != nullptr);

    VideoDecoderJsCallback *cb = new(std::nothrow) VideoDecoderJsCallback();
    CHECK_AND_RETURN(cb != nullptr);

    cb->callback = refMap_.at(INPUT_CALLBACK_NAME);
    cb->callbackName = INPUT_CALLBACK_NAME;
    cb->index = index;
    cb->memory = buffer;
    return OnJsBufferCallBack(cb, true);
}

void VideoDecoderCallbackNapi::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (refMap_.find(OUTPUT_CALLBACK_NAME) == refMap_.end()) {
        MEDIA_LOGW("can not find output buffer callback!");
        return;
    }
    auto vdec = vdec_.lock();
    CHECK_AND_RETURN(vdec != nullptr);

    VideoDecoderJsCallback *cb = new(std::nothrow) VideoDecoderJsCallback();
    CHECK_AND_RETURN(cb != nullptr);

    cb->callback = refMap_.at(OUTPUT_CALLBACK_NAME);
    cb->callbackName = OUTPUT_CALLBACK_NAME;
    cb->index = index;
    cb->info = info;
    cb->flag = flag;
    return OnJsBufferCallBack(cb, false);
}

void VideoDecoderCallbackNapi::OnJsErrorCallBack(VideoDecoderJsCallback *jsCb) const
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
        VideoDecoderJsCallback *event = reinterpret_cast<VideoDecoderJsCallback *>(work->data);
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

void VideoDecoderCallbackNapi::OnJsBufferCallBack(VideoDecoderJsCallback *jsCb, bool isInput) const
{
    ON_SCOPE_EXIT(0) { delete jsCb; };

    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    CHECK_AND_RETURN_LOG(loop != nullptr, "Fail to get uv event loop");

    uv_work_t *work = new(std::nothrow) uv_work_t;
    CHECK_AND_RETURN_LOG(work != nullptr, "Fail to new uv_work_t");

    jsCb->isInput = isInput;
    work->data = reinterpret_cast<void *>(jsCb);
    // async callback, jsWork and jsWork->data should be heap object.
    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        CHECK_AND_RETURN_LOG(work != nullptr, "Work thread is nullptr");
        VideoDecoderJsCallback *event = reinterpret_cast<VideoDecoderJsCallback *>(work->data);
        MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work start", event->callbackName.c_str());
        do {
            CHECK_AND_BREAK(status != UV_ECANCELED);
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
                    nullptr, event->info, event->flag);
            }
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

void VideoDecoderCallbackNapi::OnJsFormatCallBack(VideoDecoderJsCallback *jsCb) const
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
        VideoDecoderJsCallback *event = reinterpret_cast<VideoDecoderJsCallback *>(work->data);
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
