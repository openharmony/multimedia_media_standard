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

#include "video_decoder_napi.h"
#include <climits>
#include "avcodec_napi_utils.h"
#include "media_capability_utils.h"
#include "media_log.h"
#include "media_errors.h"
#include "video_decoder_callback_napi.h"
#include "scope_guard.h"
#include "surface_utils.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoDecoderNapi"};
}

namespace OHOS {
namespace Media {
thread_local napi_ref VideoDecoderNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "VideoDecodeProcessor";

VideoDecoderNapi::VideoDecoderNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

VideoDecoderNapi::~VideoDecoderNapi()
{
    if (vdec_ != nullptr) {
        (void)vdec_->SetCallback(nullptr);
    }
    vdec_ = nullptr;
    callback_ = nullptr;
    if (wrap_ != nullptr) {
        napi_delete_reference(env_, wrap_);
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

napi_value VideoDecoderNapi::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("configure", Configure),
        DECLARE_NAPI_FUNCTION("prepare", Prepare),
        DECLARE_NAPI_FUNCTION("start", Start),
        DECLARE_NAPI_FUNCTION("stop", Stop),
        DECLARE_NAPI_FUNCTION("flush", Flush),
        DECLARE_NAPI_FUNCTION("reset", Reset),
        DECLARE_NAPI_FUNCTION("release", Release),
        DECLARE_NAPI_FUNCTION("pushInputData", QueueInput),
        DECLARE_NAPI_FUNCTION("freeOutputBuffer", ReleaseOutput),
        DECLARE_NAPI_FUNCTION("renderOutputData", RenderOutputData),
        DECLARE_NAPI_FUNCTION("setOutputSurface", SetOutputSurface),
        DECLARE_NAPI_FUNCTION("setParameter", SetParameter),
        DECLARE_NAPI_FUNCTION("getOutputMediaDescription", GetOutputMediaDescription),
        DECLARE_NAPI_FUNCTION("getVideoDecoderCaps", GetVideoDecoderCaps),
        DECLARE_NAPI_FUNCTION("on", On),
    };
    napi_property_descriptor staticProperty[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createVideoDecoderByMime", CreateVideoDecoderByMime),
        DECLARE_NAPI_STATIC_FUNCTION("createVideoDecoderByName", CreateVideoDecoderByName)
    };

    napi_value constructor = nullptr;
    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define VideoDecoder class");

    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to create reference of constructor");

    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set constructor");

    status = napi_define_properties(env, exports, sizeof(staticProperty) / sizeof(staticProperty[0]), staticProperty);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define static function");

    MEDIA_LOGD("Init success");
    return exports;
}

napi_value VideoDecoderNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    napi_value jsThis = nullptr;
    size_t argCount = 2;
    napi_value args[2] = {0};
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET(status == napi_ok && args[0] != nullptr && args[1] != nullptr, result);

    VideoDecoderNapi *vdecNapi = new(std::nothrow) VideoDecoderNapi();
    CHECK_AND_RETURN_RET(vdecNapi != nullptr, result);

    ON_SCOPE_EXIT(0) { delete vdecNapi; };

    vdecNapi->env_ = env;
    std::string name = CommonNapi::GetStringArgument(env, args[0]);

    int32_t useMime = 0;
    status = napi_get_value_int32(env, args[1], &useMime);
    CHECK_AND_RETURN_RET(status == napi_ok, result);
    if (useMime == 1) {
        vdecNapi->vdec_ = VideoDecoderFactory::CreateByMime(name);
    } else {
        vdecNapi->vdec_ = VideoDecoderFactory::CreateByName(name);
    }
    CHECK_AND_RETURN_RET(vdecNapi->vdec_ != nullptr, result);
    vdecNapi->codecHelper_ = std::make_shared<AVCodecNapiHelper>();

    if (vdecNapi->callback_ == nullptr) {
        vdecNapi->callback_ = std::make_shared<VideoDecoderCallbackNapi>(env, vdecNapi->vdec_, vdecNapi->codecHelper_);
        (void)vdecNapi->vdec_->SetCallback(vdecNapi->callback_);
    }

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(vdecNapi),
        VideoDecoderNapi::Destructor, nullptr, &(vdecNapi->wrap_));
    CHECK_AND_RETURN_RET(status == napi_ok, result);

    CANCEL_SCOPE_EXIT_GUARD(0);

    MEDIA_LOGD("Constructor success");
    return jsThis;
}

void VideoDecoderNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    (void)env;
    (void)finalize;
    if (nativeObject != nullptr) {
        delete reinterpret_cast<VideoDecoderNapi *>(nativeObject);
    }
    MEDIA_LOGD("Destructor success");
}

napi_value VideoDecoderNapi::CreateVideoDecoderByMime(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter CreateVideoDecoderByMime");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<VideoDecoderAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[2] = {nullptr};
    size_t argCount = 2;

    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "failed to napi_get_cb_info");
    }

    std::string name;
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_string) {
        name = CommonNapi::GetStringArgument(env, args[0]);
    } else {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Illegal argument");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[1]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);
    asyncCtx->JsResult = std::make_unique<AVCodecJsResultCtor>(constructor_, 1, name);
    asyncCtx->ctorFlag = true;

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "CreateVideoDecoderByMime", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value VideoDecoderNapi::CreateVideoDecoderByName(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter CreateVideoDecoderByName");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<VideoDecoderAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[2] = {nullptr};
    size_t argCount = 2;

    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "failed to napi_get_cb_info");
    }

    std::string name;
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_string) {
        name = CommonNapi::GetStringArgument(env, args[0]);
    } else {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Illegal argument");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[1]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);
    asyncCtx->JsResult = std::make_unique<AVCodecJsResultCtor>(constructor_, 0, name);
    asyncCtx->ctorFlag = true;

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "CreateVideoDecoderByName", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value VideoDecoderNapi::Configure(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter Configure");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<VideoDecoderAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[2] = {nullptr};
    size_t argCount = 2;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_object) {
        (void)AVCodecNapiUtil::ExtractMediaFormat(env, args[0], asyncCtx->format);
    } else {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Illegal argument");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[1]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Configure", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<VideoDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr || asyncCtx->napi->vdec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            if (asyncCtx->napi->vdec_->Configure(asyncCtx->format) != MSERR_OK) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "Failed to Configure");
            }
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value VideoDecoderNapi::Prepare(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter Prepare");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<VideoDecoderAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Prepare", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<VideoDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr || asyncCtx->napi->vdec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            if (!asyncCtx->napi->isSurfaceMode_) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "Need to setOutputSurface before prepare");
                return;
            }
            if (asyncCtx->napi->vdec_->Prepare() != MSERR_OK) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "Failed to Prepare");
            }
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value VideoDecoderNapi::Start(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter Start");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<VideoDecoderAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));
    if (asyncCtx->napi->codecHelper_ != nullptr) {
        asyncCtx->napi->codecHelper_->SetStop(false);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Start", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<VideoDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr || asyncCtx->napi->vdec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            if (asyncCtx->napi->vdec_->Start() != MSERR_OK) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "Failed to Start");
            }
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value VideoDecoderNapi::Stop(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter Stop");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<VideoDecoderAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));
    if (asyncCtx->napi->codecHelper_ != nullptr) {
        asyncCtx->napi->codecHelper_->SetStop(true);
        asyncCtx->napi->codecHelper_->SetEos(false);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Stop", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<VideoDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr || asyncCtx->napi->vdec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            if (asyncCtx->napi->vdec_->Stop() != MSERR_OK) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "Failed to Stop");
            }
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value VideoDecoderNapi::Flush(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter Flush");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<VideoDecoderAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));
    if (asyncCtx->napi->codecHelper_ != nullptr) {
        asyncCtx->napi->codecHelper_->SetEos(false);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Flush", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<VideoDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr || asyncCtx->napi->vdec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            if (asyncCtx->napi->vdec_->Flush() != MSERR_OK) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "Failed to Flush");
            }
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value VideoDecoderNapi::Reset(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter Reset");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<VideoDecoderAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));
    if (asyncCtx->napi->codecHelper_ != nullptr) {
        asyncCtx->napi->codecHelper_->SetStop(true);
        asyncCtx->napi->codecHelper_->SetEos(false);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Reset", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<VideoDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr || asyncCtx->napi->vdec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            if (asyncCtx->napi->vdec_->Reset() != MSERR_OK) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "Failed to Reset");
            }
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value VideoDecoderNapi::Release(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter Release");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<VideoDecoderAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));
    if (asyncCtx->napi->codecHelper_ != nullptr) {
        asyncCtx->napi->codecHelper_->SetStop(true);
        asyncCtx->napi->codecHelper_->SetEos(false);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Release", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<VideoDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr || asyncCtx->napi->vdec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            if (asyncCtx->napi->vdec_->Release() != MSERR_OK) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "Failed to Release");
            }
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value VideoDecoderNapi::QueueInput(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<VideoDecoderAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[2] = {nullptr};
    size_t argCount = 2;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_object) {
        (void)AVCodecNapiUtil::ExtractCodecBuffer(env, args[0], asyncCtx->index, asyncCtx->info, asyncCtx->flag);
    } else {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Illegal argument");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[1]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));
    if (asyncCtx->napi->codecHelper_->IsEos() || asyncCtx->napi->codecHelper_->IsStop()) {
        MEDIA_LOGD("Eos or stop, queue buffer failed");
        return result;
    }
    if (asyncCtx->flag & AVCODEC_BUFFER_FLAG_EOS) {
        asyncCtx->napi->codecHelper_->SetEos(true);
    }
    if (asyncCtx->napi->vdec_->QueueInputBuffer(asyncCtx->index, asyncCtx->info, asyncCtx->flag) != MSERR_OK) {
        asyncCtx->SignError(MSERR_EXT_UNKNOWN, "Failed to QueueInput");
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "QueueInput", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<VideoDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr || asyncCtx->napi->vdec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value VideoDecoderNapi::ReleaseOutput(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<VideoDecoderAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[2] = {nullptr};
    size_t argCount = 2;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_object) {
        (void)CommonNapi::GetPropertyInt32(env, args[0], "index", asyncCtx->index);
    } else {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Illegal argument");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[1]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));

    if (asyncCtx->napi->codecHelper_->IsStop() || asyncCtx->napi->codecHelper_->IsFlushing()) {
        MEDIA_LOGD("Stop already or flushing, release output failed");
        return result;
    }

    if (asyncCtx->napi->vdec_->ReleaseOutputBuffer(asyncCtx->index, false) != MSERR_OK) {
        asyncCtx->SignError(MSERR_EXT_UNKNOWN, "Failed to ReleaseOutput");
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "ReleaseOutput", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<VideoDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr || asyncCtx->napi->vdec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value VideoDecoderNapi::RenderOutputData(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<VideoDecoderAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[2] = {nullptr};
    size_t argCount = 2;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_object) {
        (void)CommonNapi::GetPropertyInt32(env, args[0], "index", asyncCtx->index);
    } else {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Illegal argument");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[1]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));

    if (asyncCtx->napi->codecHelper_->IsStop() || asyncCtx->napi->codecHelper_->IsFlushing()) {
        MEDIA_LOGD("Stop already or flushing, release output failed");
        return result;
    }

    if (asyncCtx->napi->vdec_->ReleaseOutputBuffer(asyncCtx->index, true) != MSERR_OK) {
        asyncCtx->SignError(MSERR_EXT_UNKNOWN, "Failed to ReleaseOutput");
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "ReleaseOutput", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<VideoDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr || asyncCtx->napi->vdec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value VideoDecoderNapi::SetOutputSurface(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter SetOutputSurface");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<VideoDecoderAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[3] = {nullptr};
    size_t argCount = 3;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_string &&
        args[1] != nullptr && napi_typeof(env, args[1], &valueType) == napi_ok && valueType == napi_boolean) {
        std::string idStr = CommonNapi::GetStringArgument(env, args[0]);
        if (idStr == "" || idStr[0] < '0' || idStr[0] > '9') {
            asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Illegal argument");
        } else {
            int32_t numBase = 10;
            uint64_t id = strtoull(idStr.c_str(), nullptr, numBase);
            asyncCtx->surface = SurfaceUtils::GetInstance()->GetSurface(id);
        }
    } else {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Illegal argument");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[2]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "SetOutputSurface", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<VideoDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr || asyncCtx->napi->vdec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            if (asyncCtx->napi->vdec_->SetOutputSurface(asyncCtx->surface) != MSERR_OK) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "Failed to SetOutputSurface");
            } else {
                asyncCtx->napi->isSurfaceMode_ = true;
            }
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value VideoDecoderNapi::SetParameter(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<VideoDecoderAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[2] = {nullptr};
    size_t argCount = 2;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_object) {
        (void)AVCodecNapiUtil::ExtractMediaFormat(env, args[0], asyncCtx->format);
    } else {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Illegal argument");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[1]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "SetParameter", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<VideoDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr || asyncCtx->napi->vdec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            if (asyncCtx->napi->vdec_->SetParameter(asyncCtx->format) != MSERR_OK) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "Failed to SetParameter");
            }
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value VideoDecoderNapi::GetOutputMediaDescription(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<VideoDecoderAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;

    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    VideoDecoderNapi *napi = nullptr;
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&napi));
    Format format;
    if (napi->vdec_ != nullptr) {
        (void)napi->vdec_->GetOutputFormat(format);
    } else {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to unwrap");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);
    asyncCtx->JsResult = std::make_unique<AVCodecJsResultFormat>(format);

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "GetOutputMediaDescription", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value VideoDecoderNapi::GetVideoDecoderCaps(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter GetVideoDecoderCaps");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<VideoDecoderAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    VideoDecoderNapi *napi = nullptr;
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&napi));
    std::string name = "";
    if (napi->vdec_ != nullptr) {
        Format format;
        (void)napi->vdec_->GetOutputFormat(format);
        (void)format.GetStringValue("plugin_name", name);
    } else {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to unwrap");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);
    asyncCtx->JsResult = std::make_unique<MediaJsVideoCapsDynamic>(name, true);

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "GetVideoDecoderCaps", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value VideoDecoderNapi::On(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    static constexpr size_t minArgCount = 2;
    size_t argCount = minArgCount;
    napi_value args[minArgCount] = { nullptr };
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr || args[0] == nullptr || args[1] == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return result;
    }

    VideoDecoderNapi *videoDecoderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&videoDecoderNapi));
    CHECK_AND_RETURN_RET(status == napi_ok && videoDecoderNapi != nullptr, result);

    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string ||
        napi_typeof(env, args[1], &valueType1) != napi_ok || valueType1 != napi_function) {
        videoDecoderNapi->ErrorCallback(MSERR_EXT_INVALID_VAL);
        return result;
    }

    std::string callbackName = CommonNapi::GetStringArgument(env, args[0]);
    MEDIA_LOGD("callbackName: %{public}s", callbackName.c_str());

    napi_ref ref = nullptr;
    status = napi_create_reference(env, args[1], 1, &ref);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && ref != nullptr, result, "failed to create reference!");

    std::shared_ptr<AutoRef> autoRef = std::make_shared<AutoRef>(env, ref);
    videoDecoderNapi->SetCallbackReference(callbackName, autoRef);
    return result;
}

void VideoDecoderNapi::ErrorCallback(MediaServiceExtErrCode errCode)
{
    if (callback_ != nullptr) {
        auto napiCb = std::static_pointer_cast<VideoDecoderCallbackNapi>(callback_);
        napiCb->SendErrorCallback(errCode);
    }
}

void VideoDecoderNapi::SetCallbackReference(const std::string &callbackName, std::shared_ptr<AutoRef> ref)
{
    refMap_[callbackName] = ref;
    if (callback_ != nullptr) {
        auto napiCb = std::static_pointer_cast<VideoDecoderCallbackNapi>(callback_);
        napiCb->SaveCallbackReference(callbackName, ref);
    }
}
} // namespace Media
} // namespace OHOS
