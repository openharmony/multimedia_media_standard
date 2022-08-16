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
const std::string BITRATE_COLLECTED_CALLBACK_NAME = "availableBitratesCollect";
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

void VideoCallbackNapi::QueueAsyncWork(VideoPlayerAsyncContext *context)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (contextMap_.find(context->asyncWorkType) == contextMap_.end())  {
        std::queue<VideoPlayerAsyncContext *> contextQue;
        contextQue.push(context);
        contextMap_[context->asyncWorkType] = contextQue;
    } else {
        contextMap_.at(context->asyncWorkType).push(context);
    }
}

void VideoCallbackNapi::ClearAsyncWork(bool error, const std::string &msg)
{
    MEDIA_LOGD("%{public}s", msg.c_str());
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = contextMap_.begin(); it != contextMap_.end(); it++) {
        auto &contextQue = it->second;
        VideoPlayerAsyncContext *context = contextQue.front();
        if (context == nullptr) {
            continue;
        }
        contextQue.pop();
        if (error) {
            context->SignError(MSERR_EXT_OPERATE_NOT_PERMIT, msg);
        }
        VideoCallbackNapi::OnJsCallBack(context);
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
        case INFO_TYPE_BITRATEDONE:
            VideoCallbackNapi::OnBitRateDoneCb(extra);
            break;
        case INFO_TYPE_VOLUME_CHANGE:
            VideoCallbackNapi::OnVolumeDoneCb();
            break;
        case INFO_TYPE_BITRATE_COLLECT:
            VideoCallbackNapi::OnBitRateCollectedCb(infoBody);
            break;
        default:
            // video + audio common info
            PlayerCallbackNapi::OnInfo(type, extra, infoBody);
            break;
    }
    MEDIA_LOGD("send OnInfo callback success");
}

void VideoCallbackNapi::OnError(PlayerErrorType errType, int32_t errCode)
{
    ClearAsyncWork(true, "The request was aborted because en error occurred, please check event(error)");
    return PlayerCallbackNapi::OnError(errType, errCode);
}

void VideoCallbackNapi::OnSeekDoneCb(int32_t position)
{
    if (contextMap_.find(AsyncWorkType::ASYNC_WORK_SEEK) == contextMap_.end())  {
        MEDIA_LOGE("OnSpeedDoneCb is called, But context is empty");
        return;
    }

    VideoPlayerAsyncContext *context = contextMap_.at(AsyncWorkType::ASYNC_WORK_SEEK).front();
    CHECK_AND_RETURN_LOG(context != nullptr, "context is nullptr");
    contextMap_.at(AsyncWorkType::ASYNC_WORK_SEEK).pop();

    context->JsResult = std::make_unique<MediaJsResultInt>(position);
    // Switch Napi threads
    VideoCallbackNapi::OnJsCallBack(context);
}

void VideoCallbackNapi::OnSpeedDoneCb(int32_t speedMode)
{
    if (speedMode < SPEED_FORWARD_0_75_X || speedMode > SPEED_FORWARD_2_00_X) {
        MEDIA_LOGE("OnSpeedDoneCb mode:%{public}d error", speedMode);
    }

    if (contextMap_.find(AsyncWorkType::ASYNC_WORK_SPEED) == contextMap_.end())  {
        MEDIA_LOGE("OnSpeedDoneCb is called, But context is empty");
        return;
    }

    VideoPlayerAsyncContext *context = contextMap_.at(AsyncWorkType::ASYNC_WORK_SPEED).front();
    CHECK_AND_RETURN_LOG(context != nullptr, "context is nullptr");
    contextMap_.at(AsyncWorkType::ASYNC_WORK_SPEED).pop();

    context->JsResult = std::make_unique<MediaJsResultInt>(context->speedMode);
    // Switch Napi threads
    VideoCallbackNapi::OnJsCallBack(context);
}

void VideoCallbackNapi::OnBitRateDoneCb(int32_t bitRate)
{
    if (contextMap_.find(AsyncWorkType::ASYNC_WORK_BITRATE) == contextMap_.end())  {
        MEDIA_LOGE("OnBitRateDoneCb is called, But context is empty");
        return;
    }

    VideoPlayerAsyncContext *context = contextMap_.at(AsyncWorkType::ASYNC_WORK_BITRATE).front();
    CHECK_AND_RETURN_LOG(context != nullptr, "context is nullptr");
    contextMap_.at(AsyncWorkType::ASYNC_WORK_BITRATE).pop();

    context->JsResult = std::make_unique<MediaJsResultInt>(bitRate);
    // Switch Napi threads
    VideoCallbackNapi::OnJsCallBack(context);
}

void VideoCallbackNapi::OnVolumeDoneCb()
{
    if (contextMap_.find(AsyncWorkType::ASYNC_WORK_VOLUME) == contextMap_.end())  {
        MEDIA_LOGE("OnVolumeDoneCb is called, But context is empty");
        return;
    }

    VideoPlayerAsyncContext *context = contextMap_.at(AsyncWorkType::ASYNC_WORK_VOLUME).front();
    CHECK_AND_RETURN_LOG(context != nullptr, "context is nullptr");
    contextMap_.at(AsyncWorkType::ASYNC_WORK_VOLUME).pop();

    // Switch Napi threads
    VideoCallbackNapi::OnJsCallBack(context);
}

void VideoCallbackNapi::OnStartRenderFrameCb() const
{
    MEDIA_LOGD("OnStartRenderFrameCb is called");
    if (refMap_.find(START_RENDER_FRAME_CALLBACK_NAME) == refMap_.end()) {
        MEDIA_LOGW("can not find start render frame callback!");
        return;
    }
    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "No memory");
    cb->callback = refMap_.at(START_RENDER_FRAME_CALLBACK_NAME);
    cb->callbackName = START_RENDER_FRAME_CALLBACK_NAME;
    return PlayerCallbackNapi::OnJsCallBack(cb);
}

void VideoCallbackNapi::OnVideoSizeChangedCb(const Format &infoBody)
{
    (void)infoBody.GetIntValue(PlayerKeys::PLAYER_WIDTH, width_);
    (void)infoBody.GetIntValue(PlayerKeys::PLAYER_HEIGHT, height_);
    MEDIA_LOGD("OnVideoSizeChangedCb is called, width = %{public}d, height = %{public}d", width_, height_);
    if (refMap_.find(VIDEO_SIZE_CHANGED_CALLBACK_NAME) == refMap_.end()) {
        MEDIA_LOGW("can not find video size changed callback!");
        return;
    }
    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "No memory");
    cb->callback = refMap_.at(VIDEO_SIZE_CHANGED_CALLBACK_NAME);
    cb->callbackName = VIDEO_SIZE_CHANGED_CALLBACK_NAME;
    cb->valueVec.push_back(width_);
    cb->valueVec.push_back(height_);
    return PlayerCallbackNapi::OnJsCallBackIntVec(cb);
}

void VideoCallbackNapi::OnPlaybackCompleteCb() const
{
    MEDIA_LOGD("OnPlaybackCompleteCb is called");
    if (refMap_.find(PLAYBACK_COMPLETED_CALLBACK_NAME) == refMap_.end()) {
        MEDIA_LOGW("can not find completed callback!");
        return;
    }

    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "No memory");
    cb->callback = refMap_.at(PLAYBACK_COMPLETED_CALLBACK_NAME);
    cb->callbackName = PLAYBACK_COMPLETED_CALLBACK_NAME;
    return PlayerCallbackNapi::OnJsCallBack(cb);
}

PlayerStates VideoCallbackNapi::GetCurrentState() const
{
    return currentState_;
}

void VideoCallbackNapi::DequeueAsyncWork()
{
    AsyncWorkType asyncWork = AsyncWorkType::ASYNC_WORK_INVALID;
    switch (currentState_) {
        case PLAYER_PREPARED:
            asyncWork = AsyncWorkType::ASYNC_WORK_PREPARE;
            break;
        case PLAYER_STARTED:
            asyncWork = AsyncWorkType::ASYNC_WORK_PLAY;
            break;
        case PLAYER_PAUSED:
            asyncWork = AsyncWorkType::ASYNC_WORK_PAUSE;
            break;
        case PLAYER_STOPPED:
            asyncWork = AsyncWorkType::ASYNC_WORK_STOP;
            break;
        case PLAYER_IDLE:
            asyncWork = AsyncWorkType::ASYNC_WORK_RESET;
            break;
        default:
            break;
    }

    if (contextMap_.find(asyncWork) == contextMap_.end()) {
        MEDIA_LOGE("OnStateChanged(%{public}d) is called, But contextState is empty", currentState_);
        return;
    }

    VideoPlayerAsyncContext *context = contextMap_.at(asyncWork).front();
    CHECK_AND_RETURN_LOG(context != nullptr, "context is nullptr");

    contextMap_.at(asyncWork).pop();
    VideoCallbackNapi::OnJsCallBack(context);
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

void VideoCallbackNapi::OnBitRateCollectedCb(const Format &infoBody) const
{
    if (refMap_.find(BITRATE_COLLECTED_CALLBACK_NAME) == refMap_.end()) {
        MEDIA_LOGW("can not find bitrate collected callback!");
        return;
    }

    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "No memory");
    cb->callback = refMap_.at(BITRATE_COLLECTED_CALLBACK_NAME);
    cb->callbackName = BITRATE_COLLECTED_CALLBACK_NAME;

    uint8_t *addr = nullptr;
    size_t size  = 0;
    uint32_t bitrate = 0;
    if (infoBody.ContainKey(std::string(PlayerKeys::PLAYER_BITRATE))) {
        infoBody.GetBuffer(std::string(PlayerKeys::PLAYER_BITRATE), &addr, size);
        if (addr == nullptr) {
            delete cb;
            return;
        }

        MEDIA_LOGD("bitrate size = %{public}zu", size / sizeof(uint32_t));
        while (size > 0) {
            if ((size - sizeof(uint32_t)) < 0) {
                break;
            }

            bitrate = *(static_cast<uint32_t *>(static_cast<void *>(addr)));
            MEDIA_LOGD("bitrate = %{public}u", bitrate);
            addr += sizeof(uint32_t);
            size -= sizeof(uint32_t);
            cb->valueVec.push_back(static_cast<int32_t>(bitrate));
        }
    }

    return PlayerCallbackNapi::OnJsCallBackIntArray(cb);
}

void VideoCallbackNapi::UvWorkCallBack(uv_work_t *work, int status)
{
    napi_status nstatus = napi_generic_failure;
    switch (status) {
        case 0:
            nstatus = napi_ok;
            break;
        case UV_EINVAL:
            nstatus = napi_invalid_arg;
            break;
        case UV_ECANCELED:
            nstatus = napi_cancelled;
            break;
        default:
            nstatus = napi_generic_failure;
            break;
    }

    auto asyncContext = reinterpret_cast<MediaAsyncContext *>(work->data);
    if (asyncContext != nullptr) {
        MediaAsyncContext::CompleteCallback(asyncContext->env, nstatus, work->data);
    }
    delete work;
}

void VideoCallbackNapi::OnJsCallBack(VideoPlayerAsyncContext *context) const
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (loop != nullptr) {
        uv_work_t *work = new(std::nothrow) uv_work_t;
        if (work != nullptr) {
            work->data = reinterpret_cast<void *>(context);
            int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, VideoCallbackNapi::UvWorkCallBack);
            if (ret != 0) {
                MEDIA_LOGE("Failed to execute libuv work queue");
                delete context;
                delete work;
            }
        } else {
            delete context;
        }
    } else {
        delete context;
    }
}
} // namespace Media
} // namespace OHOS
