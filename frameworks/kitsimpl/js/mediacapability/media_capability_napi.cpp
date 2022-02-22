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

#include "media_capability_napi.h"
#include <climits>
#include "avcodec_list.h"
#include "avcodec_napi_utils.h"
#include "media_log.h"
#include "media_errors.h"
#include "common_napi.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaCapsNapi"};
}

namespace OHOS {
namespace Media {
napi_ref MediaCapsNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "MediaCapability";

MediaCapsNapi::MediaCapsNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaCapsNapi::~MediaCapsNapi()
{
    if (wrap_ != nullptr) {
        napi_delete_reference(env_, wrap_);
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

napi_value MediaCapsNapi::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("getAudioDecoderCaps", GetAudioDecoderCaps),
        DECLARE_NAPI_FUNCTION("findAudioDecoder", FindAudioDecoder),
        DECLARE_NAPI_FUNCTION("getAudioEncoderCaps", GetAudioEncoderCaps),
        DECLARE_NAPI_FUNCTION("findAudioEncoder", FindAudioEncoder),
        DECLARE_NAPI_FUNCTION("getVideoDecoderCaps", GetVideoDecoderCaps),
        DECLARE_NAPI_FUNCTION("findVideoDecoder", FindVideoDecoder),
        DECLARE_NAPI_FUNCTION("getVideoEncoderCaps", GetVideoEncoderCaps),
        DECLARE_NAPI_FUNCTION("findVideoEncoder", FindVideoEncoder),
    };
    napi_property_descriptor staticProperty[] = {
        DECLARE_NAPI_STATIC_FUNCTION("getMediaCapability", GetMediaCapability),
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

napi_value MediaCapsNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    napi_value jsThis = nullptr;
    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET(status == napi_ok, result);

    MediaCapsNapi *napi = new(std::nothrow) MediaCapsNapi();
    CHECK_AND_RETURN_RET(napi != nullptr, result);

    napi->env_ = env;

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(napi),
        MediaCapsNapi::Destructor, nullptr, &(napi->wrap_));
    if (status != napi_ok) {
        delete napi;
        MEDIA_LOGE("Failed to wrap native instance");
        return result;
    }

    MEDIA_LOGD("Constructor success");
    return jsThis;
}

void MediaCapsNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    (void)env;
    (void)finalize;
    if (nativeObject != nullptr) {
        delete reinterpret_cast<MediaCapsNapi *>(nativeObject);
    }
    MEDIA_LOGD("Destructor success");
}

napi_value MediaCapsNapi::GetMediaCapability(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("GetMediaCapability");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<MediaCapsAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);
    asyncCtx->JsResult = std::make_unique<MediaJsResultInstance>(constructor_);

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "GetMediaCapability", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value MediaCapsNapi::GetAudioDecoderCaps(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<MediaCapsAsyncContext>(env);

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
    napi_create_string_utf8(env, "GetAudioDecoderCaps", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<MediaCapsAsyncContext *>(data);
            if (asyncCtx == nullptr || asyncCtx->napi == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            asyncCtx->JsResult = std::make_unique<MediaJsAudioCapsStatic>(true);
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value MediaCapsNapi::FindAudioDecoder(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<MediaCapsAsyncContext>(env);

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
    napi_create_string_utf8(env, "FindAudioDecoder", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<MediaCapsAsyncContext *>(data);
            if (asyncCtx == nullptr || asyncCtx->napi == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            auto codecList = AVCodecListFactory::CreateAVCodecList();
            if (codecList == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "No memory");
                return;
            }
            std::string decoder = codecList->FindAudioDecoder(asyncCtx->format);
            asyncCtx->JsResult = std::make_unique<MediaJsResultString>(decoder);
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value MediaCapsNapi::GetAudioEncoderCaps(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<MediaCapsAsyncContext>(env);

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
    napi_create_string_utf8(env, "GetAudioEncoderCaps", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<MediaCapsAsyncContext *>(data);
            if (asyncCtx == nullptr || asyncCtx->napi == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            asyncCtx->JsResult = std::make_unique<MediaJsAudioCapsStatic>(false);
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value MediaCapsNapi::FindAudioEncoder(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<MediaCapsAsyncContext>(env);

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
    napi_create_string_utf8(env, "FindAudioEncoder", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<MediaCapsAsyncContext *>(data);
            if (asyncCtx == nullptr || asyncCtx->napi == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            auto codecList = AVCodecListFactory::CreateAVCodecList();
            if (codecList == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "No memory");
                return;
            }
            std::string encoder = codecList->FindAudioEncoder(asyncCtx->format);
            asyncCtx->JsResult = std::make_unique<MediaJsResultString>(encoder);
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value MediaCapsNapi::GetVideoDecoderCaps(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<MediaCapsAsyncContext>(env);

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
    napi_create_string_utf8(env, "GetVideoDecoderCaps", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<MediaCapsAsyncContext *>(data);
            if (asyncCtx == nullptr || asyncCtx->napi == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            asyncCtx->JsResult = std::make_unique<MediaJsVideoCapsStatic>(true);
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value MediaCapsNapi::FindVideoDecoder(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<MediaCapsAsyncContext>(env);

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
    napi_create_string_utf8(env, "FindVideoDecoder", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<MediaCapsAsyncContext *>(data);
            if (asyncCtx == nullptr || asyncCtx->napi == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            auto codecList = AVCodecListFactory::CreateAVCodecList();
            if (codecList == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "No memory");
                return;
            }
            std::string decoder = codecList->FindVideoDecoder(asyncCtx->format);
            asyncCtx->JsResult = std::make_unique<MediaJsResultString>(decoder);
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value MediaCapsNapi::GetVideoEncoderCaps(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<MediaCapsAsyncContext>(env);

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
    napi_create_string_utf8(env, "GetVideoEncoderCaps", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<MediaCapsAsyncContext *>(data);
            if (asyncCtx == nullptr || asyncCtx->napi == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            asyncCtx->JsResult = std::make_unique<MediaJsVideoCapsStatic>(false);
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value MediaCapsNapi::FindVideoEncoder(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<MediaCapsAsyncContext>(env);

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
    napi_create_string_utf8(env, "FindVideoEncoder", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<MediaCapsAsyncContext *>(data);
            if (asyncCtx == nullptr || asyncCtx->napi == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            auto codecList = AVCodecListFactory::CreateAVCodecList();
            if (codecList == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "No memory");
                return;
            }
            std::string encoder = codecList->FindVideoEncoder(asyncCtx->format);
            asyncCtx->JsResult = std::make_unique<MediaJsResultString>(encoder);
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}
}  // namespace Media
}  // namespace OHOS
