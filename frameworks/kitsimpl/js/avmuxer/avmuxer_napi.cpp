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

#include "avmuxer_napi.h"
#include "scope_guard.h"
#include "media_errors.h"
#include "media_log.h"
#include "common_napi.h"
#include "avcodec_napi_utils.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMuxerNapi"};
}

namespace OHOS {
namespace Media {
napi_ref AVMuxerNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "AVMuxer";
const std::string PROPERTY_KEY_SAMPLEINFO = "sampleInfo";
const std::string PROPERTY_KEY_SIZE = "size";
const std::string PROPERTY_KEY_FLAG = "flags";
const std::string PROPERTY_KEY_TIMEMS = "timeMs";
const std::string PROPERTY_KEY_TRACK_ID = "trackIndex";
const int32_t MS_TO_US = 1000;

struct AVMuxerNapiAsyncContext : public MediaAsyncContext {
    explicit AVMuxerNapiAsyncContext(napi_env env) : MediaAsyncContext(env) {}
    ~AVMuxerNapiAsyncContext() = default;
    AVMuxerNapi *jsAVMuxer = nullptr;
    int32_t fd_;
    std::string format_;
    MediaDescription trackDesc_;
    int32_t trackId_;
    void *arrayBuffer_ = nullptr;
    size_t arrayBufferSize_;
    int32_t writeSampleFlag_;
    TrackSampleInfo trackSampleInfo_;
};

AVMuxerNapi::AVMuxerNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVMuxerNapi::~AVMuxerNapi()
{
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
    avmuxerImpl_ = nullptr;
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

napi_value AVMuxerNapi::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("setOutput", SetOutput),
        DECLARE_NAPI_FUNCTION("addTrack", AddTrack),
        DECLARE_NAPI_FUNCTION("start", Start),
        DECLARE_NAPI_FUNCTION("writeTrackSample", WriteTrackSample),
        DECLARE_NAPI_FUNCTION("stop", Stop),
        DECLARE_NAPI_FUNCTION("release", Release),
        DECLARE_NAPI_SETTER("location", SetLocation),
        DECLARE_NAPI_SETTER("orientationHint", SetOrientationHint),
    };
    
    napi_property_descriptor staticProperties[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createAVMuxer", CreateAVMuxer)
    };
    
    napi_value constructor = nullptr;
    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor,
        nullptr, sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define avmuxer class");
    
    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to create reference of constructor");
    
    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set constructor");
    
    status = napi_define_properties(env, exports,
        sizeof(staticProperties) / sizeof(staticProperties[0]), staticProperties);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define static properties");
    
    MEDIA_LOGD("Init success");
    return exports;
}

napi_value AVMuxerNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    napi_value jsThis = nullptr;
    size_t argCount = 0;
    
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, result,
        "Failed to retrieve details about the callback");
    
    AVMuxerNapi *avmuxerNapi = new(std::nothrow) AVMuxerNapi();
    CHECK_AND_RETURN_RET_LOG(avmuxerNapi != nullptr, result, "Failed to create avmuxerNapi");

    ON_SCOPE_EXIT(0) { delete avmuxerNapi; };
    
    avmuxerNapi->env_ = env;
    avmuxerNapi->avmuxerImpl_ = AVMuxerFactory::CreateAVMuxer();
    if (avmuxerNapi->avmuxerImpl_ == nullptr) {
        delete avmuxerNapi;
        MEDIA_LOGE("Failed to create avmuxerImpl");
        return result;
    }
    
    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(avmuxerNapi),
        AVMuxerNapi::Destructor, nullptr, &(avmuxerNapi->wrapper_));
    CHECK_AND_RETURN_RET(status == napi_ok, result);

    CANCEL_SCOPE_EXIT_GUARD(0);
    
    MEDIA_LOGD("Constructor success");
    return jsThis;
}

void AVMuxerNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    (void)env;
    (void)finalize;
    if (nativeObject != nullptr) {
        delete reinterpret_cast<AVMuxerNapi *>(nativeObject);
    }
    MEDIA_LOGD("Destructor success");
}

napi_value AVMuxerNapi::CreateAVMuxer(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("CreateAVMuxer In");

    std::unique_ptr<AVMuxerNapiAsyncContext> asyncContext = std::make_unique<AVMuxerNapiAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create AVMuxerNapiAsyncContext instance");

    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "Failed to call napi_get_cb_info");
    }

    asyncContext->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncContext->deferred = CommonNapi::CreatePromise(env, asyncContext->callbackRef, result);
    asyncContext->JsResult = std::make_unique<MediaJsResultInstance>(constructor_);
    asyncContext->ctorFlag = true;

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "CreateAVMuxer", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void *data) {},
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    MEDIA_LOGD("Success CreateAVMuxer");

    return result;
}

void AVMuxerNapi::AsyncSetOutput(napi_env env, void *data)
{
    MEDIA_LOGD("AsyncSetOutput In");
    auto asyncContext = reinterpret_cast<AVMuxerNapiAsyncContext *>(data);
    CHECK_AND_RETURN_LOG(asyncContext != nullptr, "AVMuxerNapiAsyncContext is nullptr!");

    if (asyncContext->jsAVMuxer == nullptr ||
        asyncContext->jsAVMuxer->avmuxerImpl_ == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "jsAVMuxer or avmuxerImpl_ is nullptr");
        return;
    }

    int32_t ret = asyncContext->jsAVMuxer->avmuxerImpl_->SetOutput(asyncContext->fd_, asyncContext->format_);
    if (ret != MSERR_OK) {
        asyncContext->SignError(MSERR_EXT_OPERATE_NOT_PERMIT, "Failed to call SetOutput");
    }
    MEDIA_LOGD("Success AsyncSetOutput");
}

napi_value AVMuxerNapi::SetOutput(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("SetOutput In");

    std::unique_ptr<AVMuxerNapiAsyncContext> asyncContext = std::make_unique<AVMuxerNapiAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create AVMuxerNapiAsyncContext instance");

    // get args
    napi_value jsThis = nullptr;
    napi_value args[3] = {nullptr};
    size_t argCount = 3;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "Failed to call napi_get_cb_info");
    }
 
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_number) {
        status = napi_get_value_int32(env, args[0], &asyncContext->fd_);
        CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "Failed to get fd");
    }
    if (args[1] != nullptr && napi_typeof(env, args[1], &valueType) == napi_ok && valueType == napi_string) {
        asyncContext->format_ = CommonNapi::GetStringArgument(env, args[1]);
    }
    asyncContext->callbackRef = CommonNapi::CreateReference(env, args[2]);
    asyncContext->deferred = CommonNapi::CreatePromise(env, asyncContext->callbackRef, result);

    // get jsAVMuxer
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->jsAVMuxer));
    if (status != napi_ok || asyncContext->jsAVMuxer == nullptr ||
        asyncContext->jsAVMuxer->avmuxerImpl_ == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "jsAVMuxer or avmuxerImpl_ is nullptr");
    }
    
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "SetOutput", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, AVMuxerNapi::AsyncSetOutput,
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    MEDIA_LOGD("Success SetOutput");
    return result;
}

napi_value AVMuxerNapi::SetLocation(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("SetLocation In");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    napi_value jsThis = nullptr;
    AVMuxerNapi *avmuxer = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr && args[0] != nullptr,
        result, "Failed to retrieve details about the callbacke");
    
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&avmuxer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && avmuxer != nullptr && avmuxer->avmuxerImpl_ != nullptr,
        result, "Failed to retrieve instance");
    
    napi_valuetype valueType = napi_undefined;
    status = napi_typeof(env, args[0], &valueType);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && valueType == napi_object, result,
        "Failed to check argument type");
    
    double latitude = 0;
    double longitude = 0;
    CommonNapi::GetPropertyDouble(env, args[0], "latitude", latitude);
    CommonNapi::GetPropertyDouble(env, args[0], "longitude", longitude);
    
    int32_t ret = avmuxer->avmuxerImpl_->SetLocation(static_cast<float>(latitude), static_cast<float>(longitude));
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, result, "Failed to call SetLocation");

    MEDIA_LOGD("Success SetLocation");
    return result;
}

napi_value AVMuxerNapi::SetOrientationHint(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("SetOrientationHint In");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    napi_value jsThis = nullptr;
    AVMuxerNapi *avmuxer = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr && args[0] != nullptr,
        result, "Failed to retrieve details about the callbacke");
    
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&avmuxer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && avmuxer != nullptr && avmuxer->avmuxerImpl_ != nullptr,
        result, "Failed to retrieve instance");
    
    napi_valuetype valueType = napi_undefined;
    status = napi_typeof(env, args[0], &valueType);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && valueType == napi_number, result,
        "Failed to check argument type");
    
    int32_t degrees;
    status = napi_get_value_int32(env, args[0], &degrees);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "Failed to get degrees");
    
    int32_t ret = avmuxer->avmuxerImpl_->SetOrientationHint(degrees);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, result, "Failed to call SetOrientationHint");

    MEDIA_LOGD("Success SetOrientationHint");
    return result;
}

void AVMuxerNapi::AsyncAddTrack(napi_env env, void *data)
{
    MEDIA_LOGD("AsyncAddTrack In");
    auto asyncContext = reinterpret_cast<AVMuxerNapiAsyncContext *>(data);
    CHECK_AND_RETURN_LOG(asyncContext != nullptr, "AsyncAddTrack is nullptr!");

    if (asyncContext->jsAVMuxer == nullptr ||
        asyncContext->jsAVMuxer->avmuxerImpl_ == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "jsAVMuxer or avmuxerImpl_ is nullptr");
        return;
    }

    int32_t ret = asyncContext->jsAVMuxer->avmuxerImpl_->AddTrack(asyncContext->trackDesc_, asyncContext->trackId_);
    MEDIA_LOGD("asyncContext->trackId_ is: %{public}d", asyncContext->trackId_);
    asyncContext->JsResult = std::make_unique<MediaJsResultInt>(asyncContext->trackId_);
    if (ret != MSERR_OK) {
        asyncContext->SignError(MSERR_EXT_OPERATE_NOT_PERMIT, "Failed to call AddTrack");
    }
    MEDIA_LOGD("Success AsyncAddTrack");
}

napi_value AVMuxerNapi::AddTrack(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("AddTrack In");

    std::unique_ptr<AVMuxerNapiAsyncContext> asyncContext = std::make_unique<AVMuxerNapiAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create AVMuxerNapiAsyncContext instance");

    // get args
    napi_value jsThis = nullptr;
    napi_value args[2] = {nullptr};
    size_t argCount = 2;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "Failed to call napi_get_cb_info");
    }
    
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_object) {
        (void)AVCodecNapiUtil::ExtractMediaFormat(env, args[0], asyncContext->trackDesc_);
    } else {
        asyncContext->SignError(MSERR_EXT_INVALID_VAL, "Illegal argument");
    }
    asyncContext->callbackRef = CommonNapi::CreateReference(env, args[1]);
    asyncContext->deferred = CommonNapi::CreatePromise(env, asyncContext->callbackRef, result);
    
    // get jsAVMuxer
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->jsAVMuxer));
    if (status != napi_ok || asyncContext->jsAVMuxer == nullptr ||
        asyncContext->jsAVMuxer->avmuxerImpl_ == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "jsAVMuxer or avmuxerImpl_ is nullptr");
    }
    
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "AddTrack", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, AVMuxerNapi::AsyncAddTrack,
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    MEDIA_LOGD("Success AddTrack");

    return result;
}

void AVMuxerNapi::AsyncStart(napi_env env, void *data)
{
    MEDIA_LOGD("AsyncStart In");
    auto asyncContext = reinterpret_cast<AVMuxerNapiAsyncContext *>(data);
    CHECK_AND_RETURN_LOG(asyncContext != nullptr, "AsyncStart is nullptr!");

    if (asyncContext->jsAVMuxer == nullptr ||
        asyncContext->jsAVMuxer->avmuxerImpl_ == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "jsAVMuxer or avmuxerImpl_ is nullptr");
        return;
    }

    int32_t ret = asyncContext->jsAVMuxer->avmuxerImpl_->Start();
    if (ret != MSERR_OK) {
        asyncContext->SignError(MSERR_EXT_OPERATE_NOT_PERMIT, "Failed to call Start");
    }
    MEDIA_LOGD("Success AsyncStart");
}

napi_value AVMuxerNapi::Start(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("Start In");

    std::unique_ptr<AVMuxerNapiAsyncContext> asyncContext = std::make_unique<AVMuxerNapiAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create AVMuxerNapiAsyncContext instance");

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "Failed to call napi_get_cb_info");
    }

    asyncContext->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncContext->deferred = CommonNapi::CreatePromise(env, asyncContext->callbackRef, result);
    
    // get jsAVMuxer
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->jsAVMuxer));
    if (status != napi_ok || asyncContext->jsAVMuxer == nullptr ||
        asyncContext->jsAVMuxer->avmuxerImpl_ == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "jsAVMuxer or avmuxerImpl_ is nullptr");
    }
    
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Start", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, AVMuxerNapi::AsyncStart,
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    MEDIA_LOGD("Success Start");

    return result;
}

void AVMuxerNapi::AsyncWriteTrackSample(napi_env env, void *data)
{
    MEDIA_LOGD("AsyncWriteTrackSample In");
    auto asyncContext = reinterpret_cast<AVMuxerNapiAsyncContext *>(data);
    CHECK_AND_RETURN_LOG(asyncContext != nullptr, "AsyncWriteTrackSample is nullptr!");

    if (asyncContext->jsAVMuxer == nullptr ||
        asyncContext->jsAVMuxer->avmuxerImpl_ == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "jsAVMuxer or avmuxerImpl_ is nullptr");
        return;
    }
    MEDIA_LOGD("AsyncWriteTrackSample Out");
}

napi_value AVMuxerNapi::WriteTrackSample(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("WriteTrackSample In");

    std::unique_ptr<AVMuxerNapiAsyncContext> asyncContext = std::make_unique<AVMuxerNapiAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create AVMuxerNapiAsyncContext instance");

    napi_value jsThis = nullptr;
    napi_value args[4] = {nullptr,};
    size_t argCount = 4;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr,
        result, "Failed to retrieve details about the callback");
    
    napi_valuetype valueType = napi_undefined;
    bool isArrayBuffer = false;
    if (args[0] != nullptr && napi_is_arraybuffer(env, args[0], &isArrayBuffer) == napi_ok && isArrayBuffer == true) {
        napi_get_arraybuffer_info(env, args[0], &(asyncContext->arrayBuffer_), &(asyncContext->arrayBufferSize_));
    }
    uint32_t offset;
    if (args[1] != nullptr && napi_typeof(env, args[1], &valueType) == napi_ok && valueType == napi_number) {
        status = napi_get_value_uint32(env, args[1], &offset);
        CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "Failed to get degrees");
    }
    if (args[2] != nullptr && napi_typeof(env, args[2], &valueType) == napi_ok && valueType == napi_object) {
        bool ret;
        napi_status state;
        napi_value trackSampleInfo;
        state = napi_get_named_property(env, args[2], PROPERTY_KEY_SAMPLEINFO.c_str(), &trackSampleInfo);
        CHECK_AND_RETURN_RET_LOG(state == napi_ok, result, "Failed to call napi_get_named_property");
        ret = CommonNapi::GetPropertyUint32(env, trackSampleInfo, PROPERTY_KEY_SIZE, asyncContext->trackSampleInfo_.size);
        CHECK_AND_RETURN_RET_LOG(ret == true, result, "Failed to get PROPERTY_KEY_SIZE");
        int32_t flags;
        ret = CommonNapi::GetPropertyInt32(env, trackSampleInfo, PROPERTY_KEY_FLAG, flags);
        CHECK_AND_RETURN_RET_LOG(ret == true, result, "Failed to get PROPERTY_KEY_FLAG");
        asyncContext->trackSampleInfo_.flags = static_cast<AVCodecBufferFlag>(flags);
        double milliTime;
        ret = CommonNapi::GetPropertyDouble(env, trackSampleInfo, PROPERTY_KEY_TIMEMS, milliTime);
        asyncContext->trackSampleInfo_.timeMs = milliTime * MS_TO_US;
        CHECK_AND_RETURN_RET_LOG(ret == true, result, "Failed to get PROPERTY_KEY_TIMEMS");
        ret = CommonNapi::GetPropertyUint32(env, args[2], PROPERTY_KEY_TRACK_ID, asyncContext->trackSampleInfo_.trackIdx);
        CHECK_AND_RETURN_RET_LOG(ret == true, result, "Failed to get PROPERTY_KEY_TRACK_ID");
    }
    asyncContext->callbackRef = CommonNapi::CreateReference(env, args[3]);
    asyncContext->deferred = CommonNapi::CreatePromise(env, asyncContext->callbackRef, result);

    // get jsAVMuxer
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->jsAVMuxer));
    if (status != napi_ok || asyncContext->jsAVMuxer == nullptr ||
        asyncContext->jsAVMuxer->avmuxerImpl_ == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "jsAVMuxer or avmuxerImpl_ is nullptr");
    } else {
        std::shared_ptr<AVMemory> avMem =
            std::make_shared<AVMemory>(static_cast<uint8_t *>(asyncContext->arrayBuffer_), asyncContext->arrayBufferSize_);
        avMem->SetRange(offset, asyncContext->trackSampleInfo_.size);
        asyncContext->writeSampleFlag_ =
            asyncContext->jsAVMuxer->avmuxerImpl_->WriteTrackSample(avMem, asyncContext->trackSampleInfo_);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "WriteTrackSample", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, AVMuxerNapi::AsyncWriteTrackSample,
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();

    return result;
}

void AVMuxerNapi::AsyncStop(napi_env env, void *data)
{
    MEDIA_LOGD("AsyncStop In");
    auto asyncContext = reinterpret_cast<AVMuxerNapiAsyncContext *>(data);
    CHECK_AND_RETURN_LOG(asyncContext != nullptr, "AsyncStop is nullptr!");

    if (asyncContext->jsAVMuxer == nullptr ||
        asyncContext->jsAVMuxer->avmuxerImpl_ == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "jsAVMuxer or avmuxerImpl_ is nullptr");
        return;
    }

    int32_t ret = asyncContext->jsAVMuxer->avmuxerImpl_->Stop();
    if (ret != MSERR_OK) {
        asyncContext->SignError(MSERR_EXT_OPERATE_NOT_PERMIT, "Failed to call Stop");
    }
    MEDIA_LOGD("Success AsyncStop");
}

napi_value AVMuxerNapi::Stop(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("Stop In");

    std::unique_ptr<AVMuxerNapiAsyncContext> asyncContext = std::make_unique<AVMuxerNapiAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create AVMuxerNapiAsyncContext instance");

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "Failed to call napi_get_cb_info");
    }

    asyncContext->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncContext->deferred = CommonNapi::CreatePromise(env, asyncContext->callbackRef, result);

    // get jsAVMuxer
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->jsAVMuxer));
    if (status != napi_ok || asyncContext->jsAVMuxer == nullptr ||
        asyncContext->jsAVMuxer->avmuxerImpl_ == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "jsAVMuxer or avmuxerImpl_ is nullptr");
    }
    
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Stop", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, AVMuxerNapi::AsyncStop,
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    MEDIA_LOGD("Success Stop");

    return result;
}

napi_value AVMuxerNapi::Release(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("Release In");

    std::unique_ptr<AVMuxerNapiAsyncContext> asyncContext = std::make_unique<AVMuxerNapiAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create AVMuxerNapiAsyncContext instance");

    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "Failed to call napi_get_cb_info");
    }

    asyncContext->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncContext->deferred = CommonNapi::CreatePromise(env, asyncContext->callbackRef, result);

    // get jsAVMuxer
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->jsAVMuxer));
    if (status != napi_ok || asyncContext->jsAVMuxer == nullptr ||
        asyncContext->jsAVMuxer->avmuxerImpl_ == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "jsAVMuxer or avmuxerImpl_ is nullptr");
    } else {
        asyncContext->jsAVMuxer->avmuxerImpl_->Release();
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Release", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void *data) {},
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    MEDIA_LOGD("Success Release");

    return result;
}

}  // namespace Media
}  // namespace OHOS
