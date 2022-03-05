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

#include "video_callback_napi.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoCallbackNapi"};
const std::string START_RENDER_FRAME_CALLBACK_NAME = "startRenderFrame";
const std::string VIDEO_SIZE_CHANGED_CALLBACK_NAME = "videoSizeChanged";
const std::string PLAYBACK_COMPLETED_CALLBACK_NAME = "playbackCompleted";
}

namespace OHOS {
namespace Media {
VideoCallbackNapi::VideoCallbackNapi(napi_env env)
    : PlayerCallbackNapi(env), env_(env)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

VideoCallbackNapi::~VideoCallbackNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void VideoCallbackNapi::SaveCallbackReference(const std::string &callbackName, napi_value args)
{
    std::lock_guard<std::mutex> lock(mutex_);
    // only video ref callback
    if ((callbackName == START_RENDER_FRAME_CALLBACK_NAME) ||
        (callbackName == VIDEO_SIZE_CHANGED_CALLBACK_NAME) ||
        (callbackName == PLAYBACK_COMPLETED_CALLBACK_NAME)) {
        napi_ref callback = nullptr;
        napi_status status = napi_create_reference(env_, args, 1, &callback);
        CHECK_AND_RETURN_LOG(status == napi_ok && callback != nullptr, "creating reference for callback fail");
        std::shared_ptr<AutoRef> cb = std::make_shared<AutoRef>(env_, callback);
        if (callbackName == START_RENDER_FRAME_CALLBACK_NAME) {
            startRenderFrameCallback_ = cb;
        } else if (callbackName == VIDEO_SIZE_CHANGED_CALLBACK_NAME) {
            videoSizeChangedCallback_ = cb;
        } else if (callbackName == PLAYBACK_COMPLETED_CALLBACK_NAME) {
            playbackCompletedCallback_= cb;
        } else {
            MEDIA_LOGW("Unknown callback type: %{public}s", callbackName.c_str());
        }
    } else {
        // video + audio ref callback
        PlayerCallbackNapi::SaveCallbackReference(callbackName, args);
    }
}

void VideoCallbackNapi::QueueAsyncWork(VideoPlayerAsyncContext *context)
{
    std::lock_guard<std::mutex> lock(mutex_);
    switch (context->asyncWorkType) {
        case AsyncWorkType::ASYNC_WORK_PREPARE:
        case AsyncWorkType::ASYNC_WORK_PLAY:
        case AsyncWorkType::ASYNC_WORK_PAUSE:
        case AsyncWorkType::ASYNC_WORK_STOP:
        case AsyncWorkType::ASYNC_WORK_RESET:
            contextStateQue_.push(context);
            break;
        case AsyncWorkType::ASYNC_WORK_SEEK:
            contextSeekQue_.push(context);
            break;
        case AsyncWorkType::ASYNC_WORK_SPEED:
            contextSpeedQue_.push(context);
            break;
        case AsyncWorkType::ASYNC_WORK_VOLUME:
            contextVolumeQue_.push(context);
            break;
        default:
            MEDIA_LOGE("QueueAsyncWork type:%{public}d error", context->asyncWorkType);
            break;
    }
}

void VideoCallbackNapi::ClearAsyncWork()
{
    std::lock_guard<std::mutex> lock(mutex_);
    while (!contextStateQue_.empty()) {
        VideoPlayerAsyncContext *context = contextStateQue_.front();
        contextStateQue_.pop();
        delete context;
        context = nullptr;
    }
    while (!contextSeekQue_.empty()) {
        VideoPlayerAsyncContext *context = contextSeekQue_.front();
        contextSeekQue_.pop();
        delete context;
        context = nullptr;
    }
    while (!contextSpeedQue_.empty()) {
        VideoPlayerAsyncContext *context = contextSpeedQue_.front();
        contextSpeedQue_.pop();
        delete context;
        context = nullptr;
    }
    while (!contextVolumeQue_.empty()) {
        VideoPlayerAsyncContext *context = contextVolumeQue_.front();
        contextVolumeQue_.pop();
        delete context;
        context = nullptr;
    }
}

void VideoCallbackNapi::OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGI("OnInfo is called, PlayerOnInfoType: %{public}d", type);
    switch (type) {
        case INFO_TYPE_MESSAGE:
            if (extra == PlayerMessageType::PLAYER_INFO_VIDEO_RENDERING_START) {
                VideoCallbackNapi::OnStartRenderFrameCb();
            }
            break;
        case INFO_TYPE_RESOLUTION_CHANGE:
            VideoCallbackNapi::OnVideoSizeChangedCb(infoBody);
            break;
        case INFO_TYPE_STATE_CHANGE:
            VideoCallbackNapi::OnStateChangeCb(static_cast<PlayerStates>(extra));
            break;
        case INFO_TYPE_SEEKDONE:
            VideoCallbackNapi::OnSeekDoneCb(extra);
            break;
        case INFO_TYPE_SPEEDDONE:
            VideoCallbackNapi::OnSpeedDoneCb(extra);
            break;
        case INFO_TYPE_VOLUME_CHANGE:
            VideoCallbackNapi::OnVolumeDoneCb();
            break;
        default:
            // video + audio common info
            PlayerCallbackNapi::OnInfo(type, extra, infoBody);
            break;
    }
    MEDIA_LOGD("send OnInfo callback success");
}

void VideoCallbackNapi::OnSeekDoneCb(int32_t position)
{
    if (contextSeekQue_.empty()) {
        MEDIA_LOGD("OnSeekDoneCb is called, But contextSeekQue_ is empty");
        return;
    }

    VideoPlayerAsyncContext *context = contextSeekQue_.front();
    CHECK_AND_RETURN_LOG(context != nullptr, "context is nullptr");
    contextSeekQue_.pop();
    context->JsResult = std::make_unique<MediaJsResultInt>(position);

    // Switch Napi threads
    napi_value resource = nullptr;
    (void)napi_create_string_utf8(context->env, "SeekDone", NAPI_AUTO_LENGTH, &resource);
    (void)napi_create_async_work(context->env, nullptr, resource, [](napi_env env, void* data) {},
        MediaAsyncContext::CompleteCallback, static_cast<void *>(context), &context->work);
    (void)napi_queue_async_work(context->env, context->work);
}

void VideoCallbackNapi::OnSpeedDoneCb(int32_t speedMode)
{
    if (contextSpeedQue_.empty()) {
        MEDIA_LOGD("OnSpeedDoneCb is called, But contextSpeedQue_ is empty");
        return;
    }

    VideoPlayerAsyncContext *context = contextSpeedQue_.front();
    CHECK_AND_RETURN_LOG(context != nullptr, "context is nullptr");
    contextSpeedQue_.pop();

    if (speedMode < SPEED_FORWARD_0_75_X || speedMode > SPEED_FORWARD_2_00_X) {
        MEDIA_LOGE("OnSpeedDoneCb mode:%{public}d error", speedMode);
    }

    context->JsResult = std::make_unique<MediaJsResultInt>(context->speedMode);
    // Switch Napi threads
    napi_value resource = nullptr;
    (void)napi_create_string_utf8(context->env, "SpeedDone", NAPI_AUTO_LENGTH, &resource);
    (void)napi_create_async_work(context->env, nullptr, resource, [](napi_env env, void* data) {},
        MediaAsyncContext::CompleteCallback, static_cast<void *>(context), &context->work);
    (void)napi_queue_async_work(context->env, context->work);
}

void VideoCallbackNapi::OnVolumeDoneCb()
{
    if (contextVolumeQue_.empty()) {
        MEDIA_LOGD("OnVolumeDoneCb is called, But contextVolumeQue_ is empty");
        return;
    }

    VideoPlayerAsyncContext *context = contextVolumeQue_.front();
    CHECK_AND_RETURN_LOG(context != nullptr, "context is nullptr");
    contextVolumeQue_.pop();

    // Switch Napi threads
    napi_value resource = nullptr;
    (void)napi_create_string_utf8(context->env, "VolumeDone", NAPI_AUTO_LENGTH, &resource);
    (void)napi_create_async_work(context->env, nullptr, resource, [](napi_env env, void* data) {},
        MediaAsyncContext::CompleteCallback, static_cast<void *>(context), &context->work);
    (void)napi_queue_async_work(context->env, context->work);
}

void VideoCallbackNapi::OnStartRenderFrameCb() const
{
    MEDIA_LOGD("OnStartRenderFrameCb is called");
    CHECK_AND_RETURN_LOG(startRenderFrameCallback_ != nullptr,
        "Cannot find the reference of startRenderFrame callback");

    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "No memory");
    cb->callback = startRenderFrameCallback_;
    cb->callbackName = START_RENDER_FRAME_CALLBACK_NAME;
    return PlayerCallbackNapi::OnJsCallBack(cb);
}

void VideoCallbackNapi::OnVideoSizeChangedCb(const Format &infoBody)
{
    MEDIA_LOGD("OnVideoSizeChangedCb is called");
    (void)infoBody.GetIntValue(PlayerKeys::PLAYER_WIDTH, width_);
    (void)infoBody.GetIntValue(PlayerKeys::PLAYER_HEIGHT, height_);

    CHECK_AND_RETURN_LOG(videoSizeChangedCallback_ != nullptr,
        "Cannot find the reference of videoSizeChanged callback");
    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "No memory");
    cb->callback = videoSizeChangedCallback_;
    cb->callbackName = VIDEO_SIZE_CHANGED_CALLBACK_NAME;
    cb->valueVec.push_back(width_);
    cb->valueVec.push_back(height_);
    return PlayerCallbackNapi::OnJsCallBackIntVec(cb);
}

void VideoCallbackNapi::OnPlaybackCompleteCb() const
{
    MEDIA_LOGD("OnPlaybackCompleteCb is called");
    CHECK_AND_RETURN_LOG(playbackCompletedCallback_ != nullptr,
        "Cannot find the reference of startRenderFrame callback");

    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "No memory");
    cb->callback = playbackCompletedCallback_;
    cb->callbackName = PLAYBACK_COMPLETED_CALLBACK_NAME;
    return PlayerCallbackNapi::OnJsCallBack(cb);
}

PlayerStates VideoCallbackNapi::GetCurrentState() const
{
    return currentState_;
}

void VideoCallbackNapi::DequeueAsyncWork()
{
    if (contextStateQue_.empty()) {
        MEDIA_LOGW("OnStateChanged is called, But contextStateQue_ is empty");
        return;
    }

    VideoPlayerAsyncContext *context = contextStateQue_.front();
    CHECK_AND_RETURN_LOG(context != nullptr, "context is nullptr");

    bool needCb = false;
    switch (currentState_) {
        case PLAYER_PREPARED:
            needCb = context->asyncWorkType == AsyncWorkType::ASYNC_WORK_PREPARE ? true : false;
            break;
        case PLAYER_STARTED:
            needCb = context->asyncWorkType == AsyncWorkType::ASYNC_WORK_PLAY ? true : false;
            break;
        case PLAYER_PAUSED:
            needCb = context->asyncWorkType == AsyncWorkType::ASYNC_WORK_PAUSE ? true : false;
            break;
        case PLAYER_STOPPED:
            needCb = context->asyncWorkType == AsyncWorkType::ASYNC_WORK_STOP ? true : false;
            break;
        case PLAYER_IDLE:
            needCb = context->asyncWorkType == AsyncWorkType::ASYNC_WORK_RESET ? true : false;
            break;
        default:
            break;
    }

    if (needCb) {
        contextStateQue_.pop();
        // Switch Napi threads
        napi_value resource = nullptr;
        (void)napi_create_string_utf8(context->env, "OnStateChanged", NAPI_AUTO_LENGTH, &resource);
        (void)napi_create_async_work(context->env, nullptr, resource, [](napi_env env, void* data) {},
            MediaAsyncContext::CompleteCallback, static_cast<void *>(context), &context->work);
        (void)napi_queue_async_work(context->env, context->work);
    } else {
        MEDIA_LOGD("state:%{public}d is called, But context is empty", currentState_);
    }
}

void VideoCallbackNapi::OnStateChangeCb(PlayerStates state)
{
    MEDIA_LOGD("OnStateChanged is called, current state: %{public}d", state);
    currentState_ = state;

    switch (state) {
        case PLAYER_PREPARED:
        case PLAYER_STARTED:
        case PLAYER_PAUSED:
        case PLAYER_STOPPED:
        case PLAYER_IDLE:
            return DequeueAsyncWork();
        case PLAYER_PLAYBACK_COMPLETE:
            return OnPlaybackCompleteCb();
        default:
            break;
    }
}
}  // namespace Media
}  // namespace OHOS
