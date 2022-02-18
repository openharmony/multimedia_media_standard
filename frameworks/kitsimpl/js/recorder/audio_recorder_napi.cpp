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

#include "audio_recorder_napi.h"
#include <unistd.h>
#include <fcntl.h>
#include <climits>
#include "recorder_callback_napi.h"
#include "media_log.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "string_ex.h"
#include "common_napi.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioRecorderNapi"};
}

namespace OHOS {
namespace Media {
napi_ref AudioRecorderNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "AudioRecorder";
const int32_t DEFAULT_AUDIO_ENCODER_BIT_RATE = 48000;
const int32_t DEFAULT_AUDIO_SAMPLE_RATE = 48000;
const int32_t DEFAULT_NUMBER_OF_CHANNELS = 2;

AudioRecorderNapi::AudioRecorderProperties::AudioRecorderProperties()
    : sourceType(AUDIO_SOURCE_DEFAULT),
      outputFormatType(FORMAT_DEFAULT),
      audioCodecFormat(AUDIO_DEFAULT),
      encodeBitRate(DEFAULT_AUDIO_ENCODER_BIT_RATE),
      audioSampleRate(DEFAULT_AUDIO_SAMPLE_RATE),
      numberOfChannels(DEFAULT_NUMBER_OF_CHANNELS)
{
}

AudioRecorderNapi::AudioRecorderProperties::~AudioRecorderProperties() = default;

AudioRecorderNapi::AudioRecorderNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AudioRecorderNapi::~AudioRecorderNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy in ", FAKE_POINTER(this));
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
    if (taskQue_ != nullptr) {
        (void)taskQue_->Stop();
    }
    callbackNapi_ = nullptr;
    recorderImpl_ = nullptr;
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy out ", FAKE_POINTER(this));
}

napi_value AudioRecorderNapi::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("prepare", Prepare),
        DECLARE_NAPI_FUNCTION("start", Start),
        DECLARE_NAPI_FUNCTION("pause", Pause),
        DECLARE_NAPI_FUNCTION("resume", Resume),
        DECLARE_NAPI_FUNCTION("stop", Stop),
        DECLARE_NAPI_FUNCTION("reset", Reset),
        DECLARE_NAPI_FUNCTION("release", Release),
        DECLARE_NAPI_FUNCTION("on", On)
    };
    napi_property_descriptor staticProperty[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createAudioRecorder", CreateAudioRecorder),
    };

    napi_value constructor = nullptr;
    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define AudioRecorder class");

    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to create reference of constructor");

    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set constructor");

    status = napi_define_properties(env, exports, sizeof(staticProperty) / sizeof(staticProperty[0]), staticProperty);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define static function");

    MEDIA_LOGD("Init success");
    return exports;
}

napi_value AudioRecorderNapi::Constructor(napi_env env, napi_callback_info info)
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

    AudioRecorderNapi *recorderNapi = new(std::nothrow) AudioRecorderNapi();
    CHECK_AND_RETURN_RET_LOG(recorderNapi != nullptr, nullptr, "No memory");

    recorderNapi->env_ = env;
    recorderNapi->recorderImpl_ = RecorderFactory::CreateRecorder();
    CHECK_AND_RETURN_RET_LOG(recorderNapi->recorderImpl_ != nullptr, nullptr, "No memory");

    recorderNapi->taskQue_ = std::make_unique<TaskQueue>("RecorderNapi");
    (void)recorderNapi->taskQue_->Start();

    recorderNapi->callbackNapi_ = std::make_shared<RecorderCallbackNapi>(env);
    (void)recorderNapi->recorderImpl_->SetRecorderCallback(recorderNapi->callbackNapi_);

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(recorderNapi),
        AudioRecorderNapi::Destructor, nullptr, &(recorderNapi->wrapper_));
    if (status != napi_ok) {
        napi_get_undefined(env, &result);
        delete recorderNapi;
        MEDIA_LOGE("Failed to wrap native instance");
        return result;
    }

    MEDIA_LOGD("Constructor success");
    return jsThis;
}

void AudioRecorderNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    (void)env;
    (void)finalize;
    if (nativeObject != nullptr) {
        delete reinterpret_cast<AudioRecorderNapi *>(nativeObject);
    }
    MEDIA_LOGD("Destructor success");
}

napi_value AudioRecorderNapi::CreateAudioRecorder(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_value constructor = nullptr;
    napi_status status = napi_get_reference_value(env, constructor_, &constructor);
    if (status != napi_ok) {
        MEDIA_LOGE("Failed to get the representation of constructor object");
        napi_get_undefined(env, &result);
        return result;
    }

    status = napi_new_instance(env, constructor, 0, nullptr, &result);
    if (status != napi_ok) {
        MEDIA_LOGE("new instance fail");
        napi_get_undefined(env, &result);
        return result;
    }

    MEDIA_LOGD("CreateAudioRecorder success");
    return result;
}

napi_value AudioRecorderNapi::CreateAudioRecorderAsync(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("CreateAudioRecorderAsync In");

    auto asyncCtx = std::make_unique<MediaAsyncContext>(env);

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "failed to napi_get_cb_info");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);
    asyncCtx->JsResult = std::make_unique<MediaJsResultInstance>(constructor_);
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "CreateAudioRecorderAsync", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value AudioRecorderNapi::Prepare(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};

    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr || args[0] == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    AudioRecorderNapi *recorderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&recorderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && recorderNapi != nullptr, undefinedResult,
        "Failed to retrieve instance");

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_object) {
        recorderNapi->ErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    std::string uriPath = "invalid uri";
    AudioRecorderProperties audioProperties;
    if (recorderNapi->GetAudioUriPath(env, args[0], uriPath) != MSERR_OK) {
        recorderNapi->ErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    if (recorderNapi->GetAudioProperties(env, args[0], audioProperties) != MSERR_OK) {
        recorderNapi->ErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    CHECK_AND_RETURN_RET_LOG(recorderNapi->taskQue_ != nullptr, undefinedResult, "No TaskQue");
    auto task = std::make_shared<TaskHandler<void>>([recorderNapi, uriPath, audioProperties]() {
        int32_t ret = recorderNapi->OnPrepare(uriPath, audioProperties);
        if (ret == MSERR_OK) {
            recorderNapi->StateCallback(PREPARE_CALLBACK_NAME);
        } else {
            recorderNapi->ErrorCallback(MSERR_EXT_INVALID_VAL);
        }
        MEDIA_LOGD("Prepare success");
    });
    (void)recorderNapi->taskQue_->EnqueueTask(task);

    return undefinedResult;
}

int32_t AudioRecorderNapi::GetAudioApi8Properties(napi_env env, napi_value args, AudioRecorderProperties &properties)
{
    bool ret = false;
    napi_status status = napi_has_named_property(env, args, "fileFormat", &ret);
    if (status == napi_ok && ret) {
        ContainerFormatType tempCFT;
        std::string outputFile = CommonNapi::GetPropertyString(env, args, "fileFormat");
        MapStringToContainerFormat(outputFile, tempCFT);
        MapContainerFormatToOutputFormat(tempCFT, properties.outputFormatType);
    }

    status = napi_has_named_property(env, args, "audioEncoderMime", &ret);
    if (status == napi_ok && ret) {
        CodecMimeType tempCMT;
        std::string audioCodec = CommonNapi::GetPropertyString(env, args, "audioEncoderMime");
        MapStringToCodecMime(audioCodec, tempCMT);
        MapCodecMimeToAudioCodec(tempCMT, properties.audioCodecFormat);
    }

    return MSERR_OK;
}

int32_t AudioRecorderNapi::GetAudioProperties(napi_env env, napi_value args, AudioRecorderProperties &properties)
{
    properties.sourceType = AUDIO_MIC;

    int32_t fileFormat = 0;
    bool ret = CommonNapi::GetPropertyInt32(env, args, "format", fileFormat);
    if (ret == false) {
        fileFormat = JS_DEFAULT_FILE_FORMAT;
    }
    switch (fileFormat) {
        case JS_DEFAULT_FILE_FORMAT:
        case JS_MPEG_4:
            properties.outputFormatType = FORMAT_MPEG_4;
            break;
        case JS_AAC_ADTS:
            properties.outputFormatType = FORMAT_M4A;
            break;
        default:
            return MSERR_INVALID_VAL;
    }

    int32_t audioEncoder = 0;
    ret = CommonNapi::GetPropertyInt32(env, args, "audioEncoder", audioEncoder);
    if (ret == false) {
        fileFormat = JS_DEFAULT_ENCORD_TYPE;
    }
    switch (audioEncoder) {
        case JS_DEFAULT_ENCORD_TYPE:
        case JS_AAC_LC:
            properties.audioCodecFormat = AAC_LC;
            break;
        default:
            return MSERR_INVALID_VAL;
    }

    (void)GetAudioApi8Properties(env, args, properties);

    napi_value geoLocation = nullptr;
    napi_get_named_property(env, args, "location", &geoLocation);
    double tempLatitude = 0;
    double tempLongitude = 0;
    (void)CommonNapi::GetPropertyDouble(env, geoLocation, "latitude", tempLatitude);
    (void)CommonNapi::GetPropertyDouble(env, geoLocation, "longitude", tempLongitude);
    properties.location.latitude = static_cast<float>(tempLatitude);
    properties.location.longitude = static_cast<float>(tempLongitude);

    ret = CommonNapi::GetPropertyInt32(env, args, "audioEncodeBitRate", properties.encodeBitRate);
    CHECK_AND_RETURN_RET_LOG(ret, MSERR_INVALID_VAL, "get audioEncodeBitRate failed");
    ret = CommonNapi::GetPropertyInt32(env, args, "audioSampleRate", properties.audioSampleRate);
    CHECK_AND_RETURN_RET_LOG(ret, MSERR_INVALID_VAL, "get audioSampleRate failed");
    ret = CommonNapi::GetPropertyInt32(env, args, "numberOfChannels", properties.numberOfChannels);
    CHECK_AND_RETURN_RET_LOG(ret, MSERR_INVALID_VAL, "get numberOfChannels failed");
    return MSERR_OK;
}

int32_t AudioRecorderNapi::GetAudioUriPath(napi_env env, napi_value args, std::string &uriPath)
{
    bool exist = false;
    napi_status status = napi_has_named_property(env, args, "uri", &exist);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && exist, MSERR_INVALID_OPERATION, "can not find uri property");

    napi_value configItem = nullptr;
    status = napi_get_named_property(env, args, "uri", &configItem);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, MSERR_INVALID_OPERATION, "can not get uri property");

    char buffer[PATH_MAX] = {0};
    size_t bytesToCopy = 0;
    status = napi_get_value_string_latin1(env, configItem, buffer, PATH_MAX - 1, &bytesToCopy);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, MSERR_INVALID_OPERATION, "can not get uri content");

    uriPath = buffer;
    return MSERR_OK;
}

int32_t AudioRecorderNapi::OnPrepare(const std::string &uriPath, const AudioRecorderProperties &properties)
{
    CHECK_AND_RETURN_RET_LOG(recorderImpl_ != nullptr, MSERR_INVALID_OPERATION, "No memory");
    int32_t sourceId = -1;
    int32_t ret = recorderImpl_->SetAudioSource(properties.sourceType, sourceId);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Fail to SetAudioSource");

    ret = recorderImpl_->SetOutputFormat(properties.outputFormatType);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Fail to SetOutputFormat");

    ret = recorderImpl_->SetAudioEncoder(sourceId, properties.audioCodecFormat);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Fail to SetAudioEncoder");

    ret = recorderImpl_->SetAudioEncodingBitRate(sourceId, properties.encodeBitRate);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Fail to SetAudioEncodingBitRate");

    ret = recorderImpl_->SetAudioSampleRate(sourceId, properties.audioSampleRate);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Fail to SetAudioSampleRate");

    ret = recorderImpl_->SetAudioChannels(sourceId, properties.numberOfChannels);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Fail to SetAudioChannels");

    recorderImpl_->SetLocation(properties.location.latitude, properties.location.longitude);

    ret = SetUri(uriPath);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Fail to SetUri");

    ret = recorderImpl_->Prepare();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Fail to Prepare");
    return MSERR_OK;
}

napi_value AudioRecorderNapi::Start(napi_env env, napi_callback_info info)
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

    AudioRecorderNapi *recorderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&recorderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && recorderNapi != nullptr, undefinedResult,
        "Failed to retrieve instance");

    CHECK_AND_RETURN_RET_LOG(recorderNapi->recorderImpl_ != nullptr, undefinedResult, "No memory");
    CHECK_AND_RETURN_RET_LOG(recorderNapi->taskQue_ != nullptr, undefinedResult, "No TaskQue");
    auto task = std::make_shared<TaskHandler<void>>([napi = recorderNapi]() {
        int32_t ret = napi->recorderImpl_->Start();
        if (ret == MSERR_OK) {
            napi->StateCallback(START_CALLBACK_NAME);
        } else {
            napi->ErrorCallback(MSERR_EXT_UNKNOWN);
        }
        MEDIA_LOGD("Start success");
    });
    (void)recorderNapi->taskQue_->EnqueueTask(task);

    return undefinedResult;
}

napi_value AudioRecorderNapi::Pause(napi_env env, napi_callback_info info)
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

    AudioRecorderNapi *recorderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&recorderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && recorderNapi != nullptr, undefinedResult,
        "Failed to retrieve instance");

    CHECK_AND_RETURN_RET_LOG(recorderNapi->recorderImpl_ != nullptr, undefinedResult, "No memory");
    CHECK_AND_RETURN_RET_LOG(recorderNapi->taskQue_ != nullptr, undefinedResult, "No TaskQue");
    auto task = std::make_shared<TaskHandler<void>>([napi = recorderNapi]() {
        int32_t ret = napi->recorderImpl_->Pause();
        if (ret == MSERR_OK) {
            napi->StateCallback(PAUSE_CALLBACK_NAME);
        } else {
            napi->ErrorCallback(MSERR_EXT_UNKNOWN);
        }
        MEDIA_LOGD("Pause success");
    });
    (void)recorderNapi->taskQue_->EnqueueTask(task);
    return undefinedResult;
}

napi_value AudioRecorderNapi::Resume(napi_env env, napi_callback_info info)
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

    AudioRecorderNapi *recorderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&recorderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && recorderNapi != nullptr, undefinedResult,
        "Failed to retrieve instance");

    CHECK_AND_RETURN_RET_LOG(recorderNapi->recorderImpl_ != nullptr, undefinedResult, "No memory");
    CHECK_AND_RETURN_RET_LOG(recorderNapi->taskQue_ != nullptr, undefinedResult, "No TaskQue");
    auto task = std::make_shared<TaskHandler<void>>([napi = recorderNapi]() {
        int32_t ret = napi->recorderImpl_->Resume();
        if (ret == MSERR_OK) {
            napi->StateCallback(RESUME_CALLBACK_NAME);
        } else {
            napi->ErrorCallback(MSERR_EXT_UNKNOWN);
        }
        MEDIA_LOGD("Resume success");
    });
    (void)recorderNapi->taskQue_->EnqueueTask(task);
    return undefinedResult;
}

napi_value AudioRecorderNapi::Stop(napi_env env, napi_callback_info info)
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

    AudioRecorderNapi *recorderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&recorderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && recorderNapi != nullptr, undefinedResult,
        "Failed to retrieve instance");

    CHECK_AND_RETURN_RET_LOG(recorderNapi->recorderImpl_ != nullptr, undefinedResult, "No memory");
    CHECK_AND_RETURN_RET_LOG(recorderNapi->taskQue_ != nullptr, undefinedResult, "No TaskQue");
    auto task = std::make_shared<TaskHandler<void>>([napi = recorderNapi]() {
        int32_t ret = napi->recorderImpl_->Stop(false);
        if (ret == MSERR_OK) {
            napi->StateCallback(STOP_CALLBACK_NAME);
        } else {
            napi->ErrorCallback(MSERR_EXT_UNKNOWN);
        }
        MEDIA_LOGD("Stop success");
    });
    (void)recorderNapi->taskQue_->EnqueueTask(task);
    return undefinedResult;
}

napi_value AudioRecorderNapi::Reset(napi_env env, napi_callback_info info)
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

    AudioRecorderNapi *recorderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&recorderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && recorderNapi != nullptr, undefinedResult,
        "Failed to retrieve instance");

    CHECK_AND_RETURN_RET_LOG(recorderNapi->recorderImpl_ != nullptr, undefinedResult, "No memory");
    CHECK_AND_RETURN_RET_LOG(recorderNapi->taskQue_ != nullptr, undefinedResult, "No TaskQue");
    auto task = std::make_shared<TaskHandler<void>>([napi = recorderNapi]() {
        int32_t ret = napi->recorderImpl_->Reset();
        if (ret == MSERR_OK) {
            napi->StateCallback(RESET_CALLBACK_NAME);
        } else {
            napi->ErrorCallback(MSERR_EXT_UNKNOWN);
        }
        MEDIA_LOGD("Reset success");
    });
    (void)recorderNapi->taskQue_->EnqueueTask(task);
    return undefinedResult;
}

napi_value AudioRecorderNapi::Release(napi_env env, napi_callback_info info)
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

    AudioRecorderNapi *recorderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&recorderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && recorderNapi != nullptr, undefinedResult,
        "Failed to retrieve instance");

    CHECK_AND_RETURN_RET_LOG(recorderNapi->recorderImpl_ != nullptr, undefinedResult, "No memory");
    CHECK_AND_RETURN_RET_LOG(recorderNapi->taskQue_ != nullptr, undefinedResult, "No TaskQue");
    auto task = std::make_shared<TaskHandler<void>>([napi = recorderNapi]() {
        int32_t ret = napi->recorderImpl_->Release();
        if (ret == MSERR_OK) {
            napi->StateCallback(RELEASE_CALLBACK_NAME);
        } else {
            napi->ErrorCallback(MSERR_EXT_UNKNOWN);
        }
        MEDIA_LOGD("Release success");
    });
    (void)recorderNapi->taskQue_->EnqueueTask(task);
    return undefinedResult;
}

napi_value AudioRecorderNapi::On(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    static const size_t MIN_REQUIRED_ARG_COUNT = 2;
    size_t argCount = MIN_REQUIRED_ARG_COUNT;
    napi_value args[MIN_REQUIRED_ARG_COUNT] = { nullptr };
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr || args[0] == nullptr || args[1] == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    AudioRecorderNapi *recorderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&recorderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && recorderNapi != nullptr, undefinedResult,
        "Failed to retrieve instance");

    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string ||
        napi_typeof(env, args[1], &valueType1) != napi_ok || valueType1 != napi_function) {
        recorderNapi->ErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    std::string callbackName = CommonNapi::GetStringArgument(env, args[0]);
    MEDIA_LOGD("callbackName: %{public}s", callbackName.c_str());

    CHECK_AND_RETURN_RET_LOG(recorderNapi->callbackNapi_ != nullptr, undefinedResult, "callbackNapi_ is nullptr");
    auto cb = std::static_pointer_cast<RecorderCallbackNapi>(recorderNapi->callbackNapi_);
    cb->SaveCallbackReference(callbackName, args[1]);
    return undefinedResult;
}

int32_t AudioRecorderNapi::CheckValidPath(const std::string &filePath, std::string &realPath)
{
    if (!PathToRealPath(filePath, realPath)) {
        MEDIA_LOGE("Configured output filePath invalid, ignore !");
        return MSERR_INVALID_VAL;
    }
    struct stat s;
    if (stat(realPath.c_str(), &s) != 0) {
        MEDIA_LOGE("Configured output filePath invalid, ignore !");
        return MSERR_INVALID_VAL;
    }
    if ((s.st_mode & S_IFREG) == 0) {
        MEDIA_LOGE("Configured output filePath invalid, ignore !");
        return MSERR_INVALID_VAL;
    }
    return MSERR_OK;
}

int32_t AudioRecorderNapi::SetUri(const std::string &uriPath)
{
    CHECK_AND_RETURN_RET_LOG(recorderImpl_ != nullptr, MSERR_INVALID_OPERATION, "No memory");
    const std::string fileHead = "file://";
    const std::string fdHead = "fd://";
    int32_t fd = -1;

    if (uriPath.find(fileHead) != std::string::npos) {
        std::string filePath = uriPath.substr(fileHead.size());
        std::string realPath = "invalid";
        CHECK_AND_RETURN_RET(CheckValidPath(filePath, realPath) == MSERR_OK, MSERR_INVALID_VAL);
        CHECK_AND_RETURN_RET(!realPath.empty(), MSERR_INVALID_VAL);
        int32_t ret = recorderImpl_->SetOutputPath(realPath);
        CHECK_AND_RETURN_RET(ret == MSERR_OK, MSERR_INVALID_OPERATION);
    } else if (uriPath.find(fdHead) != std::string::npos) {
        std::string inputFd = uriPath.substr(fdHead.size());
        CHECK_AND_RETURN_RET(StrToInt(inputFd, fd) == true, MSERR_INVALID_VAL);
        CHECK_AND_RETURN_RET(fd >= 0, MSERR_INVALID_OPERATION);
        CHECK_AND_RETURN_RET(recorderImpl_->SetOutputFile(fd) == MSERR_OK, MSERR_INVALID_OPERATION);
    } else {
        MEDIA_LOGE("invalid input uri, neither file nor fd!");
        return MSERR_INVALID_OPERATION;
    }

    return MSERR_OK;
}

void AudioRecorderNapi::ErrorCallback(MediaServiceExtErrCode errCode)
{
    if (callbackNapi_ != nullptr) {
        std::shared_ptr<RecorderCallbackNapi> napiCb = std::static_pointer_cast<RecorderCallbackNapi>(callbackNapi_);
        napiCb->SendErrorCallback(errCode);
    }
}

void AudioRecorderNapi::StateCallback(const std::string &callbackName)
{
    if (callbackNapi_ != nullptr) {
        std::shared_ptr<RecorderCallbackNapi> napiCb = std::static_pointer_cast<RecorderCallbackNapi>(callbackNapi_);
        napiCb->SendStateCallback(callbackName);
    }
}
}  // namespace Media
}  // namespace OHOS
