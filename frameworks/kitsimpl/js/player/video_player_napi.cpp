
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

#include "video_player_napi.h"
#include <climits>
#include "video_callback_napi.h"
#include "media_log.h"
#include "media_errors.h"
#include "media_data_source_napi.h"
#include "media_data_source_callback.h"
#include "media_description.h"
#include "common_napi.h"
#include "media_surface.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoPlayerNapi"};
}

namespace OHOS {
namespace Media {
namespace VideoPlayState {
const std::string STATE_IDLE = "idle";
const std::string STATE_PREPARED = "prepared";
const std::string STATE_PLAYING = "playing";
const std::string STATE_PAUSED = "paused";
const std::string STATE_STOPPED = "stopped";
const std::string STATE_ERROR = "error";
};
napi_ref VideoPlayerNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "VideoPlayer";

VideoPlayerNapi::VideoPlayerNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

VideoPlayerNapi::~VideoPlayerNapi()
{
    jsCallback_ = nullptr;
    nativePlayer_ = nullptr;
    dataSrcCallBack_ = nullptr;
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

napi_value VideoPlayerNapi::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor staticProperty[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createVideoPlayer", CreateVideoPlayer),
    };

    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("setDisplaySurface", SetDisplaySurface),
        DECLARE_NAPI_FUNCTION("getDisplaySurface", GetDisplaySurface), // Informal external interface
        DECLARE_NAPI_FUNCTION("prepare", Prepare),
        DECLARE_NAPI_FUNCTION("play", Play),
        DECLARE_NAPI_FUNCTION("pause", Pause),
        DECLARE_NAPI_FUNCTION("stop", Stop),
        DECLARE_NAPI_FUNCTION("reset", Reset),
        DECLARE_NAPI_FUNCTION("release", Release),
        DECLARE_NAPI_FUNCTION("seek", Seek),
        DECLARE_NAPI_FUNCTION("on", On),
        DECLARE_NAPI_FUNCTION("setVolume", SetVolume),
        DECLARE_NAPI_FUNCTION("getTrackDescription", GetTrackDescription),
        DECLARE_NAPI_FUNCTION("setSpeed", SetSpeed),

        DECLARE_NAPI_GETTER_SETTER("dataSrc", GetDataSrc, SetDataSrc),
        DECLARE_NAPI_GETTER_SETTER("url", GetUrl, SetUrl),
        DECLARE_NAPI_GETTER_SETTER("loop", GetLoop, SetLoop),

        DECLARE_NAPI_GETTER("currentTime", GetCurrentTime),
        DECLARE_NAPI_GETTER("duration", GetDuration),
        DECLARE_NAPI_GETTER("state", GetState),
        DECLARE_NAPI_GETTER("width", GetWidth),
        DECLARE_NAPI_GETTER("height", GetHeight),
    };

    napi_value constructor = nullptr;
    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define AudioPlayer class");

    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to create reference of constructor");

    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set constructor");

    status = napi_define_properties(env, exports, sizeof(staticProperty) / sizeof(staticProperty[0]), staticProperty);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define static function");

    MEDIA_LOGD("Init success");
    return exports;
}

napi_value VideoPlayerNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_value jsThis = nullptr;
    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok) {
        napi_get_undefined(env, &result);
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return result;
    }

    VideoPlayerNapi *jsPlayer = new(std::nothrow) VideoPlayerNapi();
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, nullptr, "failed to new VideoPlayerNapi");

    jsPlayer->env_ = env;
    jsPlayer->nativePlayer_ = PlayerFactory::CreatePlayer();
    CHECK_AND_RETURN_RET_LOG(jsPlayer->nativePlayer_ != nullptr, nullptr, "failed to CreatePlayer");

    if (jsPlayer->jsCallback_ == nullptr) {
        jsPlayer->jsCallback_ = std::make_shared<VideoCallbackNapi>(env);
        (void)jsPlayer->nativePlayer_->SetPlayerCallback(jsPlayer->jsCallback_);
    }

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(jsPlayer),
        VideoPlayerNapi::Destructor, nullptr, &(jsPlayer->wrapper_));
    if (status != napi_ok) {
        napi_get_undefined(env, &result);
        delete jsPlayer;
        MEDIA_LOGE("Failed to wrap native instance");
        return result;
    }

    MEDIA_LOGD("Constructor success");
    return jsThis;
}

void VideoPlayerNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    (void)env;
    (void)finalize;
    if (nativeObject != nullptr) {
        delete reinterpret_cast<VideoPlayerNapi *>(nativeObject);
    }
    MEDIA_LOGD("Destructor success");
}

napi_value VideoPlayerNapi::CreateVideoPlayer(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("CreateVideoPlayer In");

    std::unique_ptr<VideoPlayerAsyncContext> asyncContext = std::make_unique<VideoPlayerAsyncContext>(env);

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok) {
        asyncContext->SignError(MSERR_EXT_INVALID_VAL, "failed to napi_get_cb_info");
    }

    // get callback
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_function) {
        const size_t refCount = 1;
        napi_create_reference(env, args[0], refCount, &asyncContext->callbackRef);
        MEDIA_LOGD("napi_create_reference callbackRef In");
    }
    // get promise
    if (asyncContext->callbackRef == nullptr) {
        MEDIA_LOGD("napi_create_promise In");
        napi_create_promise(env, &asyncContext->deferred, &result);
    }

    asyncContext->JsResult = std::make_unique<MediaJsResultInstance>(constructor_);
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "CreateVideoPlayer", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        MediaAsyncContext::AsyncCallback, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();

    return result;
}

napi_value VideoPlayerNapi::SetUrl(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    MEDIA_LOGD("SetUrl In");
    // get args and jsThis
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    // get VideoPlayerNapi
    VideoPlayerNapi *jsPlayer = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&jsPlayer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsPlayer != nullptr, undefinedResult, "Failed to retrieve instance");

    // get url from js
    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_string) {
        jsPlayer->url_.clear();
        jsPlayer->OnErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }
    jsPlayer->url_ = CommonNapi::GetStringArgument(env, args[0]);
    
    // set url to server
    int32_t ret = jsPlayer->nativePlayer_->SetSource(jsPlayer->url_);
    if (ret != MSERR_OK) {
        jsPlayer->OnErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    MEDIA_LOGD("SetUrl success");
    return undefinedResult;
}

napi_value VideoPlayerNapi::GetUrl(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    // get jsThis
    napi_value jsThis = nullptr;
    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("failed to napi_get_cb_info");
        return undefinedResult;
    }

    // get VideoPlayerNapi
    VideoPlayerNapi *jsPlayer = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&jsPlayer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsPlayer != nullptr, undefinedResult, "Failed to retrieve instance");

    if (jsPlayer->url_.empty()) {
        jsPlayer->OnErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    napi_value jsResult = nullptr;
    status = napi_create_string_utf8(env, jsPlayer->url_.c_str(), NAPI_AUTO_LENGTH, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_create_string_utf8 error");

    MEDIA_LOGD("GetSrc success");
    return jsResult;
}

napi_value VideoPlayerNapi::SetDataSrc(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("SetMediaDataSrc start");
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr && args[0] != nullptr,
        undefinedResult, "Failed to retrieve details about the callback");

    VideoPlayerNapi *jsPlayer = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&jsPlayer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsPlayer != nullptr, undefinedResult, "get player napi error");

    if (jsPlayer->dataSrcCallBack_ != nullptr) {
        jsPlayer->OnErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }
    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_object) {
        jsPlayer->OnErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    jsPlayer->dataSrcCallBack_ = MediaDataSourceCallback::Create(env, args[0]);
    if (jsPlayer->dataSrcCallBack_ == nullptr) {
        jsPlayer->OnErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }
    int32_t ret = jsPlayer->nativePlayer_->SetSource(jsPlayer->dataSrcCallBack_);
    if (ret != MSERR_OK) {
        jsPlayer->OnErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }
    MEDIA_LOGD("SetMediaDataSrc success");
    return undefinedResult;
}

napi_value VideoPlayerNapi::GetDataSrc(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    napi_value jsThis = nullptr;
    napi_value jsResult = nullptr;
    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr,
        undefinedResult, "Failed to retrieve details about the callback");

    VideoPlayerNapi *jsPlayer = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&jsPlayer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsPlayer != nullptr, undefinedResult, "Failed to retrieve instance");

    if (jsPlayer->dataSrcCallBack_ == nullptr) {
        jsPlayer->OnErrorCallback(MSERR_EXT_NO_MEMORY);
        return undefinedResult;
    }

    jsResult = jsPlayer->dataSrcCallBack_->GetDataSrc();
    CHECK_AND_RETURN_RET_LOG(jsResult != nullptr, undefinedResult, "Failed to get the representation of src object");

    MEDIA_LOGD("GetDataSrc success");
    return jsResult;
}

void VideoPlayerNapi::ReleaseDataSource(std::shared_ptr<MediaDataSourceCallback> dataSourceCb)
{
    if (dataSourceCb != nullptr) {
        dataSourceCb->Release();
        dataSourceCb = nullptr;
    }
}

void VideoPlayerNapi::AsyncSetDisplaySurface(napi_env env, void *data)
{
    MEDIA_LOGD("AsyncSetDisplaySurface In");
    auto asyncContext = reinterpret_cast<VideoPlayerAsyncContext *>(data);
    CHECK_AND_RETURN_LOG(asyncContext != nullptr, "VideoPlayerAsyncContext is nullptr!"); 
 
    if (asyncContext->jsPlayer == nullptr ||
        asyncContext->jsPlayer->nativePlayer_ == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "jsPlayer or nativePlayer is nullptr");
        return;
    }

    auto mediaSurface = MediaSurfaceFactory::CreateMediaSurface();
    if (mediaSurface == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "mediaSurface is nullptr");
        return;
    }

    auto surface = mediaSurface->GetSurface(asyncContext->surface);
    if (surface != nullptr) {
        int32_t ret = asyncContext->jsPlayer->nativePlayer_->SetVideoSurface(surface);
        if (ret != MSERR_OK) {
            asyncContext->SignError(MSERR_EXT_OPERATE_NOT_PERMIT, "failed to SetVideoSurface");
        }
    } else {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "failed to get surface");
    }
    MEDIA_LOGD("AsyncSetDisplaySurface Out");
}

napi_value VideoPlayerNapi::SetDisplaySurface(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    MEDIA_LOGD("SetDisplaySurface In");
    std::unique_ptr<VideoPlayerAsyncContext> asyncContext = std::make_unique<VideoPlayerAsyncContext>(env);

    // get args
    napi_value jsThis = nullptr;
    napi_value args[2] = { nullptr };
    size_t argCount = 2;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "failed to napi_get_cb_info");
    }

    // get surface id from js
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_string) {
        asyncContext->surface = CommonNapi::GetStringArgument(env, args[0]);
    }
    // get callback
    if (args[1] != nullptr && napi_typeof(env, args[1], &valueType) == napi_ok && valueType == napi_function) {
        const size_t refCount = 1;
        napi_create_reference(env, args[1], refCount, &asyncContext->callbackRef);
    }
    // get promise
    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &result);
    }
    // get jsPlayer
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->jsPlayer));

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "SetDisplaySurface", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, VideoPlayerNapi::AsyncSetDisplaySurface,
        MediaAsyncContext::AsyncCallback, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();

    return result;
}

void VideoPlayerNapi::AsyncGetDisplaySurface(napi_env env, void *data)
{
    MEDIA_LOGD("AsyncGetDisplaySurface In");
    auto asyncContext = reinterpret_cast<VideoPlayerAsyncContext *>(data);
    CHECK_AND_RETURN_LOG(asyncContext != nullptr, "VideoPlayerAsyncContext is nullptr!"); 

    auto mediaSurface = MediaSurfaceFactory::CreateMediaSurface();
    if (mediaSurface == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "mediaSurface is nullptr");
        return;
    }
    auto surface = mediaSurface->GetSurface();
    if (surface != nullptr) {
        auto surfaceId = mediaSurface->GetSurfaceId(surface);
        asyncContext->JsResult = std::make_unique<MediaJsResultString>(surfaceId);
    } else {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "failed to get surface");
    }
    MEDIA_LOGD("AsyncGetDisplaySurface Out");
}

napi_value VideoPlayerNapi::GetDisplaySurface(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    std::unique_ptr<VideoPlayerAsyncContext> asyncContext = std::make_unique<VideoPlayerAsyncContext>(env);

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncContext->SignError(MSERR_EXT_INVALID_VAL, "failed to napi_get_cb_info");
    }

    // get callback
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_function) {
        const size_t refCount = 1;
        napi_create_reference(env, args[0], refCount, &asyncContext->callbackRef);
    }
    // get promise
    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &result);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "GetDisplaySurface", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, VideoPlayerNapi::AsyncGetDisplaySurface,
        MediaAsyncContext::AsyncCallback, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    return result;
}

void VideoPlayerNapi::CompleteAsyncWork(napi_env env, napi_status status, void *data)
{
    MEDIA_LOGD("CompleteAsyncFunc In");
    auto asyncContext = reinterpret_cast<VideoPlayerAsyncContext *>(data);
    CHECK_AND_RETURN_LOG(asyncContext != nullptr, "VideoPlayerAsyncContext is nullptr!");

    if (status != napi_ok) {
        return MediaAsyncContext::AsyncCallback(env, status, data);
    }

    if (asyncContext->jsPlayer == nullptr || asyncContext->jsPlayer->nativePlayer_ == nullptr ||
        asyncContext->jsPlayer->jsCallback_ == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "jsPlayer or nativePlayer or jsCallback is nullptr");
        return MediaAsyncContext::AsyncCallback(env, status, data);
    }

    asyncContext->env = env;
    auto cb = std::static_pointer_cast<VideoCallbackNapi>(asyncContext->jsPlayer->jsCallback_);
    cb->QueueAsyncWork(asyncContext);

    int32_t ret = MSERR_OK;
    auto player = asyncContext->jsPlayer->nativePlayer_;
    if (asyncContext->asyncWorkType == AsyncWorkType::ASYNC_WORK_PREPARE) {
        ret = player->PrepareAsync();
    } else if (asyncContext->asyncWorkType == AsyncWorkType::ASYNC_WORK_PLAY) {
        ret = player->Play();
    } else if (asyncContext->asyncWorkType == AsyncWorkType::ASYNC_WORK_PAUSE) {
        ret = player->Pause();
    } else if (asyncContext->asyncWorkType == AsyncWorkType::ASYNC_WORK_STOP) {
        ret = player->Stop();
    } else if (asyncContext->asyncWorkType == AsyncWorkType::ASYNC_WORK_RESET) {
        asyncContext->jsPlayer->ReleaseDataSource(asyncContext->jsPlayer->dataSrcCallBack_);
        ret = player->Reset();
    } else if (asyncContext->asyncWorkType == AsyncWorkType::ASYNC_WORK_VOLUME) {
        float volume = static_cast<float>(asyncContext->volume);
        ret = player->SetVolume(volume, volume);
    } else if (asyncContext->asyncWorkType == AsyncWorkType::ASYNC_WORK_SEEK) {
        PlayerSeekMode seekMode = static_cast<PlayerSeekMode>(asyncContext->seekMode);
        ret = player->Seek(asyncContext->seekPosition, seekMode);
    } else if (asyncContext->asyncWorkType == AsyncWorkType::ASYNC_WORK_SPEED) {
        PlaybackRateMode speedMode = static_cast<PlaybackRateMode>(asyncContext->speedMode);
        ret = player->SetPlaybackSpeed(speedMode);
    } else {
        asyncContext->SignError(MSERR_EXT_OPERATE_NOT_PERMIT, "failed to operate playback");
        cb->ClearAsyncWork();
        return MediaAsyncContext::AsyncCallback(env, status, data);
    }

    if (ret != MSERR_OK) {
        asyncContext->SignError(MSERR_EXT_OPERATE_NOT_PERMIT, "failed to operate playback");
        cb->ClearAsyncWork();
        return MediaAsyncContext::AsyncCallback(env, status, data);
    }
}

napi_value VideoPlayerNapi::Prepare(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    MEDIA_LOGD("Prepare In");

    std::unique_ptr<VideoPlayerAsyncContext> asyncContext = std::make_unique<VideoPlayerAsyncContext>(env);
    asyncContext->asyncWorkType = AsyncWorkType::ASYNC_WORK_PREPARE; 

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncContext->SignError(MSERR_EXT_INVALID_VAL, "failed to napi_get_cb_info");
    }

    // get callback
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_function) {
        const size_t refCount = 1;
        (void)napi_create_reference(env, args[0], refCount, &asyncContext->callbackRef);
    }
    // get promise
    if (asyncContext->callbackRef == nullptr) {
        (void)napi_create_promise(env, &asyncContext->deferred, &undefinedResult);
    }

    // get jsPlayer
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->jsPlayer));
    // async work
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Prepare", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        CompleteAsyncWork, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    return undefinedResult;
}

napi_value VideoPlayerNapi::Play(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    MEDIA_LOGD("Play In");

    std::unique_ptr<VideoPlayerAsyncContext> asyncContext = std::make_unique<VideoPlayerAsyncContext>(env);
    asyncContext->asyncWorkType = AsyncWorkType::ASYNC_WORK_PLAY; 
    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncContext->SignError(MSERR_EXT_INVALID_VAL, "failed to napi_get_cb_info");
    }

    // get callback
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_function) {
        const size_t refCount = 1;
        napi_create_reference(env, args[0], refCount, &asyncContext->callbackRef);
    }
    // get promise
    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &undefinedResult);
    }
    // get jsPlayer
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->jsPlayer));
    // async work
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Play", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        CompleteAsyncWork, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    return undefinedResult;
}

napi_value VideoPlayerNapi::Pause(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    MEDIA_LOGD("Pause In");
    std::unique_ptr<VideoPlayerAsyncContext> asyncContext = std::make_unique<VideoPlayerAsyncContext>(env);
    asyncContext->asyncWorkType = AsyncWorkType::ASYNC_WORK_PAUSE; 

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncContext->SignError(MSERR_EXT_INVALID_VAL, "failed to napi_get_cb_info");
    }

    // get callback
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_function) {
        const size_t refCount = 1;
        napi_create_reference(env, args[0], refCount, &asyncContext->callbackRef);
    }
    // get promise
    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &undefinedResult);
    }

    // get jsPlayer
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->jsPlayer));
    // async work
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Pause", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        CompleteAsyncWork, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    return undefinedResult;
}

napi_value VideoPlayerNapi::Stop(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    MEDIA_LOGD("Stop In");
    std::unique_ptr<VideoPlayerAsyncContext> asyncContext = std::make_unique<VideoPlayerAsyncContext>(env);
    asyncContext->asyncWorkType = AsyncWorkType::ASYNC_WORK_STOP; 

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncContext->SignError(MSERR_EXT_INVALID_VAL, "failed to napi_get_cb_info");
    }

    // get callback
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_function) {
        const size_t refCount = 1;
        napi_create_reference(env, args[0], refCount, &asyncContext->callbackRef);
    }
    // get promise
    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &undefinedResult);
    }
    // get jsPlayer
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->jsPlayer));
    // async work
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Stop", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        CompleteAsyncWork, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    return undefinedResult;
}

napi_value VideoPlayerNapi::Reset(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    MEDIA_LOGD("Reset In");
    std::unique_ptr<VideoPlayerAsyncContext> asyncContext = std::make_unique<VideoPlayerAsyncContext>(env);
    asyncContext->asyncWorkType = AsyncWorkType::ASYNC_WORK_RESET; 

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncContext->SignError(MSERR_EXT_INVALID_VAL, "failed to napi_get_cb_info");
    }

    // get callback
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_function) {
        const size_t refCount = 1;
        napi_create_reference(env, args[0], refCount, &asyncContext->callbackRef);
    }
    // get promise
    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &undefinedResult);
    }

    // get jsPlayer
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->jsPlayer));
    // async work
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Reset", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        CompleteAsyncWork, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    return undefinedResult;
}

napi_value VideoPlayerNapi::Release(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    MEDIA_LOGD("Release In");
    std::unique_ptr<VideoPlayerAsyncContext> asyncContext = std::make_unique<VideoPlayerAsyncContext>(env);

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncContext->SignError(MSERR_EXT_INVALID_VAL, "failed to napi_get_cb_info");
    }

    // get callback
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_function) {
        const size_t refCount = 1;
        napi_create_reference(env, args[0], refCount, &asyncContext->callbackRef);
    }
    // get promise
    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &undefinedResult);
    }

    // get jsPlayer
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->jsPlayer));
    if (asyncContext->jsPlayer == nullptr || asyncContext->jsPlayer->nativePlayer_ == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "jsPlayer or nativePlayer_ is nullptr");
    }
    asyncContext->jsPlayer->ReleaseDataSource(asyncContext->jsPlayer->dataSrcCallBack_);
    int32_t ret = asyncContext->jsPlayer->nativePlayer_->Release();
    if (ret != MSERR_OK) {
        asyncContext->SignError(MSERR_EXT_OPERATE_NOT_PERMIT, "failed to release");
    }
    asyncContext->jsPlayer->jsCallback_ = nullptr;
    asyncContext->jsPlayer->url_.clear();
    
    // async work
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Release", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        MediaAsyncContext::AsyncCallback, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    return undefinedResult;
}

napi_value VideoPlayerNapi::Seek(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    MEDIA_LOGD("Seek In");
    std::unique_ptr<VideoPlayerAsyncContext> asyncContext = std::make_unique<VideoPlayerAsyncContext>(env);
    asyncContext->asyncWorkType = AsyncWorkType::ASYNC_WORK_SEEK;     

    // get args
    napi_value jsThis = nullptr;
    napi_value args[3] = { nullptr };
    size_t argCount = 3;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncContext->SignError(MSERR_EXT_INVALID_VAL, "failed to napi_get_cb_info");
    }

    // get seek time 
    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        asyncContext->SignError(MSERR_EXT_INVALID_VAL, "failed to get seek time");
    }
    status = napi_get_value_int32(env, args[0], &asyncContext->seekPosition);
    if (status != napi_ok || asyncContext->seekPosition < 0) {
        asyncContext->SignError(MSERR_EXT_INVALID_VAL, "seek position < 0");
    }
    if (args[1] != nullptr && napi_typeof(env, args[1], &valueType) == napi_ok) {
        if (valueType == napi_number) { // get seek mode
            status = napi_get_value_int32(env, args[1], &asyncContext->seekMode);
            if (status != napi_ok || asyncContext->seekMode < SEEK_PREVIOUS_SYNC || asyncContext->seekMode > SEEK_CLOSEST) {
                asyncContext->SignError(MSERR_EXT_INVALID_VAL, "seek mode invalid");
            }
        } else if (valueType == napi_function) {
            const size_t refCount = 1;
            napi_create_reference(env, args[1], refCount, &asyncContext->callbackRef);
        }
    }
    if (args[2] != nullptr && napi_typeof(env, args[2], &valueType) == napi_ok && valueType == napi_function) {
        const size_t refCount = 1;
        napi_create_reference(env, args[1], refCount, &asyncContext->callbackRef);      
    }
    // get promise
    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &undefinedResult);
    }
    // get jsPlayer
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->jsPlayer));
    // async work
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Seek", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        CompleteAsyncWork, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    return undefinedResult;
}

napi_value VideoPlayerNapi::SetSpeed(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    MEDIA_LOGD("SetSpeed In");
    std::unique_ptr<VideoPlayerAsyncContext> asyncContext = std::make_unique<VideoPlayerAsyncContext>(env);
    asyncContext->asyncWorkType = AsyncWorkType::ASYNC_WORK_SPEED; 

    // get args
    napi_value jsThis = nullptr;
    napi_value args[2] = { nullptr };
    size_t argCount = 2;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncContext->SignError(MSERR_EXT_INVALID_VAL, "failed to napi_get_cb_info");
    }

    // get speed mode 
    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        asyncContext->SignError(MSERR_EXT_INVALID_VAL, "failed get speed mode");
    }
    status = napi_get_value_int32(env, args[0], &asyncContext->speedMode);
    if (status != napi_ok || asyncContext->speedMode < SPEED_FORWARD_0_75_X || asyncContext->speedMode > SPEED_FORWARD_2_00_X) {
        asyncContext->SignError(MSERR_EXT_INVALID_VAL, "speed mode invalid");
    }
    // get callback
    if (args[1] != nullptr && napi_typeof(env, args[1], &valueType) == napi_ok && valueType == napi_function) {
        const size_t refCount = 1;
        napi_create_reference(env, args[1], refCount, &asyncContext->callbackRef);      
    }
    // get promise
    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &undefinedResult);
    }

    // get jsPlayer
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->jsPlayer));
    // async work
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "SetSpeed", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        CompleteAsyncWork, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    return undefinedResult;
}

void VideoPlayerNapi::AsyncGetTrackDescription(napi_env env, void *data)
{
    auto asyncContext = reinterpret_cast<VideoPlayerAsyncContext *>(data);
    CHECK_AND_RETURN_LOG(asyncContext != nullptr, "VideoPlayerAsyncContext is nullptr!");  

    if (asyncContext->jsPlayer == nullptr || asyncContext->jsPlayer->nativePlayer_ == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "jsPlayer or nativePlayer is nullptr");
        return;
    }

    auto player = asyncContext->jsPlayer->nativePlayer_;
    std::vector<Format> &videoInfo = asyncContext->jsPlayer->videoTranckInfoVec_;
    videoInfo.clear();
    int32_t ret = player->GetVideoTrackInfo(videoInfo);
    if (ret != MSERR_OK) {
        asyncContext->SignError(MSERR_EXT_UNKNOWN, "failed to operate playback");
        return;
    }

    ret = player->GetAudioTrackInfo(videoInfo);
    if (ret != MSERR_OK) {
        asyncContext->SignError(MSERR_EXT_UNKNOWN, "failed to operate playback");
        return;
    }

    if (videoInfo.empty()) {
        asyncContext->SignError(MSERR_EXT_UNKNOWN, "video tranck info is empty");
        return;
    }

    // create Description
    napi_value videoArray = nullptr;
    napi_status status = napi_create_array(env, &videoArray);
    if (status != napi_ok) {
        asyncContext->SignError(MSERR_EXT_UNKNOWN, "failed to napi_create_array");
        return;
    }

    auto vecSize = videoInfo.size();
    for (size_t index = 0; index < vecSize; ++index) {        
        napi_value description = nullptr;
        description = MediaDescriptionNapi::CreateMediaDescription(env, videoInfo[index]);
        if (description == nullptr || napi_set_element(env, videoArray, index, description) != napi_ok) {
            asyncContext->SignError(MSERR_EXT_UNKNOWN, "failed to CreateMediaDescription");
            return;
        }
    }
    // asyncContext->asyncResult = videoArray;
}

napi_value VideoPlayerNapi::GetTrackDescription(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    MEDIA_LOGD("GetTrackDescription In");
    std::unique_ptr<VideoPlayerAsyncContext> asyncContext = std::make_unique<VideoPlayerAsyncContext>(env);

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncContext->SignError(MSERR_EXT_INVALID_VAL, "failed to napi_get_cb_info");
    }

    // get callback
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_function) {
        const size_t refCount = 1;
        napi_create_reference(env, args[0], refCount, &asyncContext->callbackRef);
    }
    // get promise
    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &undefinedResult);
    }
    // get jsPlayer
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->jsPlayer));
    // async work
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "GetTrackDescription", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, VideoPlayerNapi::AsyncGetTrackDescription,
        MediaAsyncContext::AsyncCallback, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    return undefinedResult;
}

napi_value VideoPlayerNapi::SetVolume(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    MEDIA_LOGD("SetVolume In");
    std::unique_ptr<VideoPlayerAsyncContext> asyncContext = std::make_unique<VideoPlayerAsyncContext>(env);
    asyncContext->asyncWorkType = AsyncWorkType::ASYNC_WORK_VOLUME; 

    // get args
    napi_value jsThis = nullptr;
    napi_value args[2] = { nullptr };
    size_t argCount = 2;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncContext->SignError(MSERR_EXT_INVALID_VAL, "failed to napi_get_cb_info");
    }

    // get volume
    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        asyncContext->SignError(MSERR_EXT_INVALID_VAL, "get volume napi_typeof is't napi_number");
    } else {
        status = napi_get_value_double(env, args[0], &asyncContext->volume);
        if (status != napi_ok || asyncContext->volume < 0.0f || asyncContext->volume > 1.0f) {
            asyncContext->SignError(MSERR_EXT_INVALID_VAL, "get volume input volume < 0.0f or > 1.0f");
        }
    }
    // get callback
    if (args[1] != nullptr && napi_typeof(env, args[1], &valueType) == napi_ok && valueType == napi_function) {
        const size_t refCount = 1;
        napi_create_reference(env, args[1], refCount, &asyncContext->callbackRef);
    }
    // get promise
    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &undefinedResult);
    }
    // get jsPlayer
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->jsPlayer));
    // async work
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "SetVolume", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        CompleteAsyncWork, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    return undefinedResult;
}

napi_value VideoPlayerNapi::On(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    static const size_t MIN_REQUIRED_ARG_COUNT = 2;
    size_t argCount = MIN_REQUIRED_ARG_COUNT;
    napi_value args[MIN_REQUIRED_ARG_COUNT] = { nullptr, nullptr };
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr || args[0] == nullptr || args[1] == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    VideoPlayerNapi *jsPlayer = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&jsPlayer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsPlayer != nullptr, undefinedResult, "Failed to retrieve instance");

    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string ||
        napi_typeof(env, args[1], &valueType1) != napi_ok || valueType1 != napi_function) {
        jsPlayer->OnErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    std::string callbackName = CommonNapi::GetStringArgument(env, args[0]);
    MEDIA_LOGD("callbackName: %{public}s", callbackName.c_str());

    CHECK_AND_RETURN_RET_LOG(jsPlayer->jsCallback_ != nullptr, undefinedResult, "jsCallback_ is nullptr");
    auto cb = std::static_pointer_cast<VideoCallbackNapi>(jsPlayer->jsCallback_);
    cb->SaveCallbackReference(callbackName, args[1]);
    return undefinedResult;
}

napi_value VideoPlayerNapi::SetLoop(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    MEDIA_LOGD("SetLoop In");
    size_t argCount = 1;
    napi_value args[1] = { nullptr };
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr || args[0] == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    VideoPlayerNapi *jsPlayer = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&jsPlayer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsPlayer != nullptr, undefinedResult, "Failed to retrieve instance");

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_boolean) {
        jsPlayer->OnErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    bool loopFlag = false;
    status = napi_get_value_bool(env, args[0], &loopFlag);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_get_value_bool error");

    CHECK_AND_RETURN_RET_LOG(jsPlayer->nativePlayer_ != nullptr, undefinedResult, "No memory");
    int32_t ret = jsPlayer->nativePlayer_->SetLooping(loopFlag);
    if (ret != MSERR_OK) {
        jsPlayer->OnErrorCallback(MSERR_EXT_UNKNOWN);
        return undefinedResult;
    }
    MEDIA_LOGD("SetLoop success");
    return undefinedResult;
}


napi_value VideoPlayerNapi::GetLoop(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    napi_value jsThis = nullptr;
    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    VideoPlayerNapi *jsPlayer = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&jsPlayer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsPlayer != nullptr, undefinedResult, "Failed to retrieve instance");

    CHECK_AND_RETURN_RET_LOG(jsPlayer->nativePlayer_ != nullptr, undefinedResult, "No memory");
    bool loopFlag = jsPlayer->nativePlayer_->IsLooping();

    napi_value jsResult = nullptr;
    status = napi_get_boolean(env, loopFlag, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_get_boolean error");
    MEDIA_LOGD("GetSrc success loop Status: %{public}d", loopFlag);
    return jsResult;
}

napi_value VideoPlayerNapi::GetCurrentTime(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    VideoPlayerNapi *jsPlayer = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&jsPlayer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsPlayer != nullptr, undefinedResult, "Failed to retrieve instance");

    CHECK_AND_RETURN_RET_LOG(jsPlayer->nativePlayer_ != nullptr, undefinedResult, "No memory");
    int32_t currentTime = -1;
    int32_t ret = jsPlayer->nativePlayer_->GetCurrentTime(currentTime);
    if (ret != MSERR_OK || currentTime < 0) {
        jsPlayer->OnErrorCallback(MSERR_EXT_UNKNOWN);
        return undefinedResult;
    }

    napi_value jsResult = nullptr;
    status = napi_create_int32(env, currentTime, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_create_int32 error");
    MEDIA_LOGD("GetCurrenTime success, Current time: %{public}d", currentTime);
    return jsResult;
}

napi_value VideoPlayerNapi::GetDuration(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    napi_value jsThis = nullptr;
    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    VideoPlayerNapi *jsPlayer = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&jsPlayer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsPlayer != nullptr, undefinedResult, "Failed to retrieve instance");

    CHECK_AND_RETURN_RET_LOG(jsPlayer->nativePlayer_ != nullptr, undefinedResult, "No memory");
    int32_t duration = -1;
    int32_t ret = jsPlayer->nativePlayer_->GetDuration(duration);
    if (ret != MSERR_OK) {
        jsPlayer->OnErrorCallback(MSERR_EXT_UNKNOWN);
        return undefinedResult;
    }

    napi_value jsResult = nullptr;
    status = napi_create_int32(env, duration, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_create_int32 error");

    MEDIA_LOGD("GetDuration success, Current time: %{public}d", duration);
    return jsResult;
}

static std::string GetJSState(PlayerStates currentState)
{
    std::string result;
    MEDIA_LOGD("GetJSState()! is called!, %{public}d", currentState);
    switch (currentState) {
        case PLAYER_IDLE:
        case PLAYER_INITIALIZED:
            result = VideoPlayState::STATE_IDLE;
            break;
        case PLAYER_PREPARED:
            result = VideoPlayState::STATE_PREPARED;
            break;
        case PLAYER_STARTED:
            result = VideoPlayState::STATE_PLAYING;
            break;
        case PLAYER_PAUSED:
        case PLAYER_PLAYBACK_COMPLETE:
            result = VideoPlayState::STATE_PAUSED;
            break;
        case PLAYER_STOPPED:
            result = VideoPlayState::STATE_STOPPED;
            break;
        default:
            // Considering default state as stopped
            MEDIA_LOGE("Unknown state!, %{public}d", currentState);
            result = VideoPlayState::STATE_ERROR;
            break;
    }
    return result;
}

napi_value VideoPlayerNapi::GetState(napi_env env, napi_callback_info info)
{
    napi_value jsThis = nullptr;
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "Failed to retrieve details about the callback");

    VideoPlayerNapi *jsPlayer = nullptr;
    status = napi_unwrap(env, jsThis, (void **)&jsPlayer);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "Failed to retrieve instance");

    std::string curState = VideoPlayState::STATE_ERROR;
    if (jsPlayer->jsCallback_ != nullptr) {
        auto cb = std::static_pointer_cast<VideoCallbackNapi>(jsPlayer->jsCallback_);
        curState = GetJSState(cb->GetCurrentState());
        MEDIA_LOGD("GetState success, State: %{public}s", curState.c_str());
    }

    napi_value jsResult = nullptr;
    status = napi_create_string_utf8(env, curState.c_str(), NAPI_AUTO_LENGTH, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_create_string_utf8 error");
    return jsResult;
}

napi_value VideoPlayerNapi::GetWidth(napi_env env, napi_callback_info info)
{
    napi_value jsThis = nullptr;
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    MEDIA_LOGD("GetWidth In");
    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "Failed to retrieve details about the callback");

    VideoPlayerNapi *jsPlayer = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&jsPlayer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsPlayer != nullptr, undefinedResult, "Failed to retrieve instance");

    int32_t width = 0;
    if (jsPlayer->jsCallback_ != nullptr) {
        auto cb = std::static_pointer_cast<VideoCallbackNapi>(jsPlayer->jsCallback_);
        width = cb->GetVideoWidth();
    }

    napi_value jsResult = nullptr;
    status = napi_create_int32(env, width, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_create_int32 error");
    return jsResult;
}

napi_value VideoPlayerNapi::GetHeight(napi_env env, napi_callback_info info)
{
    napi_value jsThis = nullptr;
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    MEDIA_LOGD("GetHeight In");
    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "Failed to retrieve details about the callback");

    VideoPlayerNapi *jsPlayer = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&jsPlayer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsPlayer != nullptr, undefinedResult, "Failed to retrieve instance");

    int32_t height = 0;
    if (jsPlayer->jsCallback_ != nullptr) {
        auto cb = std::static_pointer_cast<VideoCallbackNapi>(jsPlayer->jsCallback_);
        height = cb->GetVideoHeight();
    }

    napi_value jsResult = nullptr;
    status = napi_create_int32(env, height, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_create_int32 error");
    return jsResult;
}

void VideoPlayerNapi::OnErrorCallback(MediaServiceExtErrCode errCode)
{
    if (jsCallback_ != nullptr) {
        auto cb = std::static_pointer_cast<VideoCallbackNapi>(jsCallback_);
        cb->SendErrorCallback(errCode);
    }
}
}  // namespace Media
}  // namespace OHOS