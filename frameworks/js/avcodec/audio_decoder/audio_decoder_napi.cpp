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

#include "audio_decoder_napi.h"
#include <climits>
#include "audio_decoder_callback_napi.h"
#include "avcodec_napi_utils.h"
#include "media_capability_utils.h"
#include "media_log.h"
#include "media_errors.h"
#include "scope_guard.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioDecoderNapi"};
}

namespace OHOS {
namespace Media {
thread_local napi_ref AudioDecoderNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "AudioDecodeProcessor";

AudioDecoderNapi::AudioDecoderNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AudioDecoderNapi::~AudioDecoderNapi()
{
    if (adec_ != nullptr) {
        (void)adec_->SetCallback(nullptr);
    }
    adec_ = nullptr;
    callback_ = nullptr;
    if (wrap_ != nullptr) {
        napi_delete_reference(env_, wrap_);
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

napi_value AudioDecoderNapi::Init(napi_env env, napi_value exports)
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
        DECLARE_NAPI_FUNCTION("setParameter", SetParameter),
        DECLARE_NAPI_FUNCTION("getOutputMediaDescription", GetOutputMediaDescription),
        DECLARE_NAPI_FUNCTION("getAudioDecoderCaps", GetAudioDecoderCaps),
        DECLARE_NAPI_FUNCTION("on", On),
    };
    napi_property_descriptor staticProperty[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createAudioDecoderByMime", CreateAudioDecoderByMime),
        DECLARE_NAPI_STATIC_FUNCTION("createAudioDecoderByName", CreateAudioDecoderByName),
    };

    napi_value constructor = nullptr;
    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define AudioDecodeProcessor class");

    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to create reference of constructor");

    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set constructor");

    status = napi_define_properties(env, exports, sizeof(staticProperty) / sizeof(staticProperty[0]), staticProperty);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define static function");

    MEDIA_LOGD("Init success");
    return exports;
}

napi_value AudioDecoderNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    napi_value jsThis = nullptr;
    size_t argCount = 2;
    napi_value args[2] = {0};
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET(status == napi_ok && args[0] != nullptr && args[1] != nullptr, result);

    AudioDecoderNapi *adecNapi = new(std::nothrow) AudioDecoderNapi();
    CHECK_AND_RETURN_RET(adecNapi != nullptr, result);

    ON_SCOPE_EXIT(0) { delete adecNapi; };

    adecNapi->env_ = env;
    std::string name = CommonNapi::GetStringArgument(env, args[0]);

    int32_t useMime = 0;
    status = napi_get_value_int32(env, args[1], &useMime);
    CHECK_AND_RETURN_RET(status == napi_ok, result);
    if (useMime == 1) {
        adecNapi->adec_ = AudioDecoderFactory::CreateByMime(name);
    } else {
        adecNapi->adec_ = AudioDecoderFactory::CreateByName(name);
    }
    CHECK_AND_RETURN_RET(adecNapi->adec_ != nullptr, result);
    adecNapi->codecHelper_ = std::make_shared<AVCodecNapiHelper>();

    if (adecNapi->callback_ == nullptr) {
        adecNapi->callback_ = std::make_shared<AudioDecoderCallbackNapi>(env, adecNapi->adec_, adecNapi->codecHelper_);
        (void)adecNapi->adec_->SetCallback(adecNapi->callback_);
    }

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(adecNapi),
        AudioDecoderNapi::Destructor, nullptr, &(adecNapi->wrap_));
    CHECK_AND_RETURN_RET(status == napi_ok, result);

    CANCEL_SCOPE_EXIT_GUARD(0);

    MEDIA_LOGD("Constructor success");
    return jsThis;
}

void AudioDecoderNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    (void)env;
    (void)finalize;
    if (nativeObject != nullptr) {
        delete reinterpret_cast<AudioDecoderNapi *>(nativeObject);
    }
    MEDIA_LOGD("Destructor success");
}

napi_value AudioDecoderNapi::CreateAudioDecoderByMime(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter CreateAudioDecoderByMime");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AudioDecoderAsyncContext>(env);

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
    napi_create_string_utf8(env, "CreateAudioDecoderByMime", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value AudioDecoderNapi::CreateAudioDecoderByName(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter CreateAudioDecoderByName");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AudioDecoderAsyncContext>(env);

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
    napi_create_string_utf8(env, "CreateAudioDecoderByName", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value AudioDecoderNapi::Configure(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter Configure");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AudioDecoderAsyncContext>(env);

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
            auto asyncCtx = reinterpret_cast<AudioDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr || asyncCtx->napi->adec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            if (asyncCtx->napi->adec_->Configure(asyncCtx->format) != MSERR_OK) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "Failed to Configure");
            }
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value AudioDecoderNapi::Prepare(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter Prepare");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AudioDecoderAsyncContext>(env);

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
            auto asyncCtx = reinterpret_cast<AudioDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr || asyncCtx->napi->adec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            if (asyncCtx->napi->adec_->Prepare() != MSERR_OK) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "Failed to Prepare");
            }
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value AudioDecoderNapi::Start(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter Start");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AudioDecoderAsyncContext>(env);

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
            auto asyncCtx = reinterpret_cast<AudioDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr || asyncCtx->napi->adec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            if (asyncCtx->napi->adec_->Start() != MSERR_OK) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "Failed to Start");
            }
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value AudioDecoderNapi::Stop(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter Stop");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AudioDecoderAsyncContext>(env);

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
        asyncCtx->napi->codecHelper_->CancelAllWorks();
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Stop", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<AudioDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr || asyncCtx->napi->adec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            if (asyncCtx->napi->adec_->Stop() != MSERR_OK) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "Failed to Stop");
            }
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value AudioDecoderNapi::Flush(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter Flush");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AudioDecoderAsyncContext>(env);

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
        asyncCtx->napi->codecHelper_->SetFlushing(true);
        asyncCtx->napi->codecHelper_->CancelAllWorks();
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Flush", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<AudioDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr || asyncCtx->napi->adec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            if (asyncCtx->napi->adec_->Flush() != MSERR_OK) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "Failed to Flush");
            }
            if (asyncCtx->napi->codecHelper_ != nullptr) {
                asyncCtx->napi->codecHelper_->SetFlushing(false);
            }
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value AudioDecoderNapi::Reset(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter Reset");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AudioDecoderAsyncContext>(env);

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
        asyncCtx->napi->codecHelper_->CancelAllWorks();
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Reset", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<AudioDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr || asyncCtx->napi->adec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            if (asyncCtx->napi->adec_->Reset() != MSERR_OK) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "Failed to Reset");
            }
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value AudioDecoderNapi::Release(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter Release");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AudioDecoderAsyncContext>(env);

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
        asyncCtx->napi->codecHelper_->CancelAllWorks();
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Release", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<AudioDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr || asyncCtx->napi->adec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            if (asyncCtx->napi->adec_->Release() != MSERR_OK) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "Failed to Release");
            }
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value AudioDecoderNapi::QueueInput(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AudioDecoderAsyncContext>(env);

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

    if (asyncCtx->napi == nullptr || asyncCtx->napi->codecHelper_ == nullptr || asyncCtx->napi->codecHelper_->IsEos()
        || asyncCtx->napi->codecHelper_->IsStop() || asyncCtx->napi->codecHelper_->IsFlushing()) {
        MEDIA_LOGD("Eos or stop or flushing, queue buffer failed");
        return result;
    }

    if (asyncCtx->flag & AVCODEC_BUFFER_FLAG_EOS) {
        asyncCtx->napi->codecHelper_->SetEos(true);
    }

    if (asyncCtx->index < 0 || asyncCtx->napi == nullptr || asyncCtx->napi->adec_ == nullptr) {
        asyncCtx->SignError(MSERR_EXT_OPERATE_NOT_PERMIT, "nullptr");
    } else {
        if (asyncCtx->napi->adec_->QueueInputBuffer(asyncCtx->index, asyncCtx->info, asyncCtx->flag) != MSERR_OK) {
            asyncCtx->SignError(MSERR_EXT_OPERATE_NOT_PERMIT, "Failed to QueueInput");
        }
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "QueueInput", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value AudioDecoderNapi::ReleaseOutput(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AudioDecoderAsyncContext>(env);

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

    if (asyncCtx->napi == nullptr || asyncCtx->napi->codecHelper_ == nullptr ||
        asyncCtx->napi->codecHelper_->IsStop() || asyncCtx->napi->codecHelper_->IsFlushing()) {
        MEDIA_LOGD("Stop already or flushing, release output failed");
        return result;
    }

    if (asyncCtx->index < 0 || asyncCtx->napi == nullptr || asyncCtx->napi->adec_ == nullptr) {
        asyncCtx->SignError(MSERR_EXT_OPERATE_NOT_PERMIT, "nullptr");
    } else {
        if (asyncCtx->napi->adec_->ReleaseOutputBuffer(asyncCtx->index) != MSERR_OK) {
            asyncCtx->SignError(MSERR_EXT_OPERATE_NOT_PERMIT, "Failed to ReleaseOutput");
        }
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "ReleaseOutput", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value AudioDecoderNapi::SetParameter(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AudioDecoderAsyncContext>(env);

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
            auto asyncCtx = reinterpret_cast<AudioDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr || asyncCtx->napi->adec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            if (asyncCtx->napi->adec_->SetParameter(asyncCtx->format) != MSERR_OK) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "Failed to SetParameter");
            }
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value AudioDecoderNapi::GetOutputMediaDescription(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter GetOutputMediaDescription");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AudioDecoderAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;

    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    AudioDecoderNapi *napi = nullptr;
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&napi));
    Format format;
    if (napi->adec_ != nullptr) {
        (void)napi->adec_->GetOutputFormat(format);
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

napi_value AudioDecoderNapi::GetAudioDecoderCaps(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter GetAudioDecoderCaps");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AudioDecoderAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;

    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    AudioDecoderNapi *napi = nullptr;
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&napi));
    std::string name = "";
    if (napi->adec_ != nullptr) {
        Format format;
        (void)napi->adec_->GetOutputFormat(format);
        (void)format.GetStringValue("plugin_name", name);
    } else {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to unwrap");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);
    asyncCtx->JsResult = std::make_unique<MediaJsAudioCapsDynamic>(name, true);

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "GetAudioDecoderCaps", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value AudioDecoderNapi::On(napi_env env, napi_callback_info info)
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

    AudioDecoderNapi *audioDecoderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&audioDecoderNapi));
    CHECK_AND_RETURN_RET(status == napi_ok && audioDecoderNapi != nullptr, result);

    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string ||
        napi_typeof(env, args[1], &valueType1) != napi_ok || valueType1 != napi_function) {
        audioDecoderNapi->ErrorCallback(MSERR_EXT_INVALID_VAL);
        return result;
    }

    std::string callbackName = CommonNapi::GetStringArgument(env, args[0]);
    MEDIA_LOGD("callbackName: %{public}s", callbackName.c_str());

    napi_ref ref = nullptr;
    status = napi_create_reference(env, args[1], 1, &ref);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && ref != nullptr, result, "failed to create reference!");

    std::shared_ptr<AutoRef> autoRef = std::make_shared<AutoRef>(env, ref);
    audioDecoderNapi->SetCallbackReference(callbackName, autoRef);
    return result;
}

void AudioDecoderNapi::ErrorCallback(MediaServiceExtErrCode errCode)
{
    if (callback_ != nullptr) {
        auto napiCb = std::static_pointer_cast<AudioDecoderCallbackNapi>(callback_);
        napiCb->SendErrorCallback(errCode);
    }
}

void AudioDecoderNapi::SetCallbackReference(const std::string &callbackName, std::shared_ptr<AutoRef> ref)
{
    refMap_[callbackName] = ref;
    if (callback_ != nullptr) {
        auto napiCb = std::static_pointer_cast<AudioDecoderCallbackNapi>(callback_);
        napiCb->SaveCallbackReference(callbackName, ref);
    }
}
} // namespace Media
} // namespace OHOS
