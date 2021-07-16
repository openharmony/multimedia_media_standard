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
#include "recorder_callback_napi.h"
#include "media_log.h"
#include "errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioRecorderNapi"};
}

namespace OHOS {
namespace Media {
napi_ref AudioRecorderNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "AudioRecorder";
const std::string CALLBACK_CONFIGURE_ERROR_TYPE = "configureError ";
const std::string CALLBACK_CONFIGURE_ERROR_CODE = "1002";
const std::string CALLBACK_STATE_CHANGE_ERROR_TYPE = "stateChangeError ";
const std::string CALLBACK_STATE_CHANGE_ERROR_CODE = "1003";
const std::string ERROR_CALLBACK_NAME = "error";
const std::string PREPARE_CALLBACK_NAME = "prepare";
const std::string START_CALLBACK_NAME = "start";
const std::string PAUSE_CALLBACK_NAME = "pause";
const std::string RESUME_CALLBACK_NAME = "resume";
const std::string STOP_CALLBACK_NAME = "stop";
const std::string RESET_CALLBACK_NAME = "reset";
const std::string RELEASE_CALLBACK_NAME = "release";
const int32_t DEFAULT_AUDIO_ENCODER_BIT_RATE = 48000;
const int32_t DEFAULT_AUDIO_SAMPLE_RATE = 48000;
const int32_t DEFAULT_NUMBER_OF_CHANNELS = 2;
const int32_t MAXIMUM_PATH_LENGTH = 128;

AudioRecorderNapi::AudioRecorderNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AudioRecorderNapi::~AudioRecorderNapi()
{
    if (errorCallback_ != nullptr) {
        napi_delete_reference(env_, errorCallback_);
    }
    if (prepareCallback_ != nullptr) {
        napi_delete_reference(env_, prepareCallback_);
    }
    if (startCallback_ != nullptr) {
        napi_delete_reference(env_, startCallback_);
    }
    if (pauseCallback_ != nullptr) {
        napi_delete_reference(env_, pauseCallback_);
    }
    if (resumeCallback_ != nullptr) {
        napi_delete_reference(env_, resumeCallback_);
    }
    if (stopCallback_ != nullptr) {
        napi_delete_reference(env_, stopCallback_);
    }
    if (resetCallback_ != nullptr) {
        napi_delete_reference(env_, resetCallback_);
    }
    if (releaseCallback_ != nullptr) {
        napi_delete_reference(env_, releaseCallback_);
    }
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
    callbackNapi_ = nullptr;
    nativeRecorder_ = nullptr;
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
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
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "define class fail");

    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "create reference fail");

    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "set named property fail");

    status = napi_define_properties(env, exports, sizeof(staticProperty) / sizeof(staticProperty[0]), staticProperty);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "define properties fail");

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
        MEDIA_LOGE("get callback info fail");
        return result;
    }

    AudioRecorderNapi *recorderNapi = new(std::nothrow) AudioRecorderNapi();
    CHECK_AND_RETURN_RET_LOG(recorderNapi != nullptr, nullptr, "recorderNapi no memory");

    recorderNapi->env_ = env;
    recorderNapi->nativeRecorder_ = RecorderFactory::CreateRecorder();
    CHECK_AND_RETURN_RET_LOG(recorderNapi->nativeRecorder_ != nullptr, nullptr, "nativeRecorder_ no memory");

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(recorderNapi),
        AudioRecorderNapi::Destructor, nullptr, &(recorderNapi->wrapper_));
    if (status != napi_ok) {
        napi_get_undefined(env, &result);
        delete recorderNapi;
        MEDIA_LOGE("native wrap fail");
        return result;
    }
    recorderNapi->callbackNapi_ = std::make_shared<RecorderCallbackNapi>(env, *recorderNapi);
    recorderNapi->nativeRecorder_->SetRecorderCallback(recorderNapi->callbackNapi_);

    MEDIA_LOGD("Constructor success");
    return jsThis;
}

void AudioRecorderNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
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
        MEDIA_LOGE("get reference value fail");
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

napi_value AudioRecorderNapi::Prepare(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 1;
    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || argCount != 1) {
        MEDIA_LOGE("napi_get_cb_info error");
        return undefinedResult;
    }

    AudioRecorderNapi *recorder = nullptr;
    status = napi_unwrap(env, jsThis, (void **)&recorder);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_unwrap error");

    napi_valuetype valueType = napi_object;
    status = napi_typeof(env, args[0], &valueType);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "get value type fail");
    CHECK_AND_RETURN_RET_LOG(valueType == napi_object, undefinedResult, "get value type fail");

    int32_t sourceId = 0;
    if (recorder->SetFormat(env, args[0], sourceId) != ERR_OK) {
        recorder->SendErrorCallback(env, info, recorder->errorCallback_,
            CALLBACK_CONFIGURE_ERROR_CODE, CALLBACK_CONFIGURE_ERROR_TYPE);
        MEDIA_LOGD("SetFormat fail");
        return undefinedResult;
    }
    if (recorder->SetAudioProperties(env, args[0], sourceId) != ERR_OK) {
        recorder->SendErrorCallback(env, info, recorder->errorCallback_,
            CALLBACK_CONFIGURE_ERROR_CODE, CALLBACK_CONFIGURE_ERROR_TYPE);
        MEDIA_LOGD("SetAudioProperties fail");
        return undefinedResult;
    }
    if (recorder->SetFilePath(env, args[0]) != ERR_OK) {
        recorder->SendErrorCallback(env, info, recorder->errorCallback_,
            CALLBACK_CONFIGURE_ERROR_CODE, CALLBACK_CONFIGURE_ERROR_TYPE);
        MEDIA_LOGD("SetFilePath fail");
        return undefinedResult;
    }
    CHECK_AND_RETURN_RET_LOG(recorder->nativeRecorder_ != nullptr, undefinedResult, "nativeRecorder_ no memory");
    if (recorder->nativeRecorder_->Prepare() != ERR_OK) {
        recorder->SendErrorCallback(env, info, recorder->errorCallback_,
            CALLBACK_CONFIGURE_ERROR_CODE, CALLBACK_CONFIGURE_ERROR_TYPE);
        MEDIA_LOGD("Prepare fail");
        return undefinedResult;
    }

    recorder->SendCallback(env, info, recorder->prepareCallback_);
    MEDIA_LOGD("Prepare success");
    return undefinedResult;
}

napi_value AudioRecorderNapi::Start(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && argCount == 0, undefinedResult, "napi_get_cb_info error");

    AudioRecorderNapi *recorder = nullptr;
    status = napi_unwrap(env, jsThis, (void **)&recorder);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_unwrap error");

    CHECK_AND_RETURN_RET_LOG(recorder->nativeRecorder_ != nullptr, undefinedResult, "nativeRecorder_ no memory");
    if (recorder->nativeRecorder_->Start() != ERR_OK) {
        recorder->SendErrorCallback(env, info, recorder->errorCallback_,
            CALLBACK_STATE_CHANGE_ERROR_CODE, CALLBACK_STATE_CHANGE_ERROR_TYPE);
        MEDIA_LOGD("Start fail");
        return undefinedResult;
    }

    recorder->SendCallback(env, info, recorder->startCallback_);
    MEDIA_LOGD("Start success");
    return undefinedResult;
}

napi_value AudioRecorderNapi::Pause(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && argCount == 0, undefinedResult, "napi_get_cb_info error");

    AudioRecorderNapi *recorder = nullptr;
    status = napi_unwrap(env, jsThis, (void **)&recorder);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_unwrap error");

    CHECK_AND_RETURN_RET_LOG(recorder->nativeRecorder_ != nullptr, undefinedResult, "nativeRecorder_ no memory");
    if (recorder->nativeRecorder_->Pause() != ERR_OK) {
        recorder->SendErrorCallback(env, info, recorder->errorCallback_,
            CALLBACK_STATE_CHANGE_ERROR_CODE, CALLBACK_STATE_CHANGE_ERROR_TYPE);
        MEDIA_LOGD("Pause fail");
        return undefinedResult;
    }

    recorder->SendCallback(env, info, recorder->pauseCallback_);
    MEDIA_LOGD("Pause success");
    return undefinedResult;
}

napi_value AudioRecorderNapi::Resume(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && argCount == 0, undefinedResult, "napi_get_cb_info error");

    AudioRecorderNapi *recorder = nullptr;
    status = napi_unwrap(env, jsThis, (void **)&recorder);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_unwrap error");

    CHECK_AND_RETURN_RET_LOG(recorder->nativeRecorder_ != nullptr, undefinedResult, "nativeRecorder_ no memory");
    if (recorder->nativeRecorder_->Resume() != ERR_OK) {
        recorder->SendErrorCallback(env, info, recorder->errorCallback_,
            CALLBACK_STATE_CHANGE_ERROR_CODE, CALLBACK_STATE_CHANGE_ERROR_TYPE);
        MEDIA_LOGD("Resume fail");
        return undefinedResult;
    }

    recorder->SendCallback(env, info, recorder->resumeCallback_);
    MEDIA_LOGD("Resume success");
    return undefinedResult;
}

napi_value AudioRecorderNapi::Stop(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && argCount == 0, undefinedResult, "napi_get_cb_info error");

    AudioRecorderNapi *recorder = nullptr;
    status = napi_unwrap(env, jsThis, (void **)&recorder);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_unwrap error");

    CHECK_AND_RETURN_RET_LOG(recorder->nativeRecorder_ != nullptr, undefinedResult, "nativeRecorder_ no memory");
    if (recorder->nativeRecorder_->Stop(false) != ERR_OK) {
        recorder->SendErrorCallback(env, info, recorder->errorCallback_,
            CALLBACK_STATE_CHANGE_ERROR_CODE, CALLBACK_STATE_CHANGE_ERROR_TYPE);
        MEDIA_LOGD("Stop fail");
        return undefinedResult;
    }

    recorder->SendCallback(env, info, recorder->stopCallback_);
    MEDIA_LOGD("Stop success");
    return undefinedResult;
}

napi_value AudioRecorderNapi::Reset(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && argCount == 0, undefinedResult, "napi_get_cb_info error");

    AudioRecorderNapi *recorder = nullptr;
    status = napi_unwrap(env, jsThis, (void **)&recorder);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_unwrap error");

    CHECK_AND_RETURN_RET_LOG(recorder->nativeRecorder_ != nullptr, undefinedResult, "nativeRecorder_ no memory");
    if (recorder->nativeRecorder_->Reset() != ERR_OK) {
        recorder->SendErrorCallback(env, info, recorder->errorCallback_,
            CALLBACK_STATE_CHANGE_ERROR_CODE, CALLBACK_STATE_CHANGE_ERROR_TYPE);
        MEDIA_LOGD("Reset fail");
        return undefinedResult;
    }

    recorder->SendCallback(env, info, recorder->resetCallback_);
    MEDIA_LOGD("Reset success");
    return undefinedResult;
}

napi_value AudioRecorderNapi::Release(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && argCount == 0, undefinedResult, "napi_get_cb_info error");

    AudioRecorderNapi *recorder = nullptr;
    status = napi_unwrap(env, jsThis, (void **)&recorder);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_unwrap error");

    CHECK_AND_RETURN_RET_LOG(recorder->nativeRecorder_ != nullptr, undefinedResult, "nativeRecorder_ no memory");
    if (recorder->nativeRecorder_->Release() != ERR_OK) {
        recorder->SendErrorCallback(env, info, recorder->errorCallback_,
            CALLBACK_STATE_CHANGE_ERROR_CODE, CALLBACK_STATE_CHANGE_ERROR_TYPE);
        MEDIA_LOGD("Release fail");
        return undefinedResult;
    }

    recorder->SendCallback(env, info, recorder->releaseCallback_);
    MEDIA_LOGD("Release success");
    return undefinedResult;
}

static std::string GetStringArgument(napi_env env, napi_value value)
{
    std::string strValue = "";
    size_t bufLength = 0;
    napi_status status = napi_get_value_string_utf8(env, value, nullptr, 0, &bufLength);
    if (status == napi_ok && bufLength > 0) {
        char *buffer = (char *)malloc((bufLength + 1) * sizeof(char));
        CHECK_AND_RETURN_RET_LOG(buffer != nullptr, strValue, "no memory");
        status = napi_get_value_string_utf8(env, value, buffer, bufLength + 1, &bufLength);
        if (status == napi_ok) {
            MEDIA_LOGD("argument = %{public}s", buffer);
            strValue = buffer;
        }
        free(buffer);
        buffer = nullptr;
    }
    return strValue;
}

napi_value AudioRecorderNapi::On(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 2;
    napi_value args[2] = { nullptr };
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && argCount >= 2, nullptr, "invalid argument");

    AudioRecorderNapi *recorder = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&recorder));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "set callback fail");

    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string
        || napi_typeof(env, args[1], &valueType1) != napi_ok || valueType1 != napi_function) {
        MEDIA_LOGE("invalid arguments type");
        return undefinedResult;
    }

    std::string callbackName = GetStringArgument(env, args[0]);
    MEDIA_LOGD("callbackName: %{public}s", callbackName.c_str());
    recorder->SaveCallbackReference(env, *recorder, callbackName, args[1]);
    return undefinedResult;
}

void AudioRecorderNapi::SendCallback(napi_env env, napi_callback_info info, napi_ref callbackRef) const
{
    CHECK_AND_RETURN_LOG(callbackRef != nullptr, "no callback reference");
    napi_value jsCallback = nullptr;
    napi_status status = napi_get_reference_value(env, callbackRef, &jsCallback);
    CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr, "get reference value fail");

    napi_value result = nullptr;
    status = napi_call_function(env, nullptr, jsCallback, 0, nullptr, &result);
    CHECK_AND_RETURN_LOG(status == napi_ok, "send callback fail");

    MEDIA_LOGD("send callback success");
}

void AudioRecorderNapi::SendErrorCallback(napi_env env, napi_callback_info info, napi_ref callbackRef,
    const std::string &errCode, const std::string &errType) const
{
    CHECK_AND_RETURN_LOG(callbackRef != nullptr, "no callback reference");
    napi_value jsCallback = nullptr;
    napi_status status = napi_get_reference_value(env, callbackRef, &jsCallback);
    CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr, "get reference value fail");

    napi_value errorTypeVal = nullptr;
    if (napi_create_string_utf8(env_, errType.c_str(), NAPI_AUTO_LENGTH, &errorTypeVal) != napi_ok) {
        MEDIA_LOGE("get error type value fail");
        return;
    }
    napi_value errorCodeVal = nullptr;
    if (napi_create_string_utf8(env_, errCode.c_str(), NAPI_AUTO_LENGTH, &errorCodeVal) != napi_ok) {
        MEDIA_LOGE("get error code value fail");
        return;
    }

    napi_value args[1] = {nullptr};
    status = napi_create_error(env_, errorCodeVal, errorTypeVal, &args[0]);
    CHECK_AND_RETURN_LOG(status == napi_ok, "create error callback fail");
    size_t argCount = 1;
    napi_value result = nullptr;
    status = napi_call_function(env_, nullptr, jsCallback, argCount, args, &result);
    if (status != napi_ok) {
        MEDIA_LOGE("call error callback fail");
    }

    MEDIA_LOGD("send error callback success");
}

int32_t AudioRecorderNapi::SetFormat(napi_env env, napi_value args, int32_t &sourceId) const
{
    int32_t ret = -1;
    AudioSourceType nativeSouceType = AUDIO_SOURCE_DEFAULT;
    this->GetAudioConfig(env, args, "AudioSourceType", &ret);
    switch (ret) {
        case JS_MIC:
            nativeSouceType = AUDIO_MIC;
            break;
        default:
            break;
    }
    CHECK_AND_RETURN_RET_LOG(this->nativeRecorder_ != nullptr, ERR_INVALID_OPERATION, "nativeRecorder_ no memory");
    CHECK_AND_RETURN_RET(this->nativeRecorder_->SetAudioSource(nativeSouceType, sourceId) == ERR_OK,
        ERR_INVALID_OPERATION);

    OutputFormatType nativeOutputFormatType = FORMAT_DEFAULT;
    this->GetAudioConfig(env, args, "fileFormat", &ret);
    switch (ret) {
        case JS_MP4:
            nativeOutputFormatType = FORMAT_MPEG_4;
            break;
        case JS_M4A:
            nativeOutputFormatType = FORMAT_M4A;
            break;
        default:
            break;
    }
    CHECK_AND_RETURN_RET(this->nativeRecorder_->SetOutputFormat(nativeOutputFormatType) == ERR_OK,
        ERR_INVALID_OPERATION);

    return ERR_OK;
}

int32_t AudioRecorderNapi::SetAudioProperties(napi_env env, napi_value args, int32_t sourceId) const
{
    int32_t ret = -1;
    AudioCodecFormat nativeAudioCodecFormat = AUDIO_DEFAULT;
    this->GetAudioConfig(env, args, "audioEncoder", &ret);
    switch (ret) {
        case JS_AAC_LC:
            nativeAudioCodecFormat = AAC_LC;
            break;
        default:
            break;
    }
    CHECK_AND_RETURN_RET_LOG(this->nativeRecorder_ != nullptr, ERR_INVALID_OPERATION, "nativeRecorder_ no memory");
    CHECK_AND_RETURN_RET(this->nativeRecorder_->SetAudioEncoder(sourceId, nativeAudioCodecFormat) == ERR_OK,
        ERR_INVALID_OPERATION);

    this->GetAudioConfig(env, args, "audioEncodeBitRate", &ret);
    if (ret == -1) {
        ret = DEFAULT_AUDIO_ENCODER_BIT_RATE;
    }
    CHECK_AND_RETURN_RET(this->nativeRecorder_->SetAudioEncodingBitRate(sourceId, ret) == ERR_OK,
        ERR_INVALID_OPERATION);

    this->GetAudioConfig(env, args, "audioSampleRate", &ret);
    if (ret == -1) {
        ret = DEFAULT_AUDIO_SAMPLE_RATE;
    }
    CHECK_AND_RETURN_RET(this->nativeRecorder_->SetAudioSampleRate(sourceId, ret) == ERR_OK,
        ERR_INVALID_OPERATION);

    this->GetAudioConfig(env, args, "numberOfChannels", &ret);
    if (ret == -1) {
        ret = DEFAULT_NUMBER_OF_CHANNELS;
    }
    CHECK_AND_RETURN_RET(this->nativeRecorder_->SetAudioChannels(sourceId, ret) == ERR_OK,
        ERR_INVALID_OPERATION);

    return ERR_OK;
}

int32_t AudioRecorderNapi::SetFilePath(napi_env env, napi_value args) const
{
    bool exist = false;
    napi_status status = napi_has_named_property(env, args, "filePath", &exist);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && exist, ERR_INVALID_OPERATION, "can not find filePath property");

    napi_value configItem = nullptr;
    status = napi_get_named_property(env, args, "filePath", &configItem);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, ERR_INVALID_OPERATION, "can not get filePath property");
    char buffer[MAXIMUM_PATH_LENGTH] = {0};
    size_t bytesToCopy = 0;
    status = napi_get_value_string_latin1(env, configItem, buffer, MAXIMUM_PATH_LENGTH - 1, &bytesToCopy);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, ERR_INVALID_OPERATION, "can not get filePath content");

    std::string filePath = buffer;
    CHECK_AND_RETURN_RET_LOG(this->nativeRecorder_ != nullptr, ERR_INVALID_OPERATION, "nativeRecorder_ no memory");
    CHECK_AND_RETURN_RET(this->nativeRecorder_->SetOutputPath(filePath) == ERR_OK, ERR_INVALID_OPERATION);

    return ERR_OK;
}

void AudioRecorderNapi::GetAudioConfig(napi_env env, napi_value configObj,
    const std::string &type, int32_t *configItem) const
{
    napi_value item = nullptr;
    bool exist = false;
    napi_status status = napi_has_named_property(env, configObj, type.c_str(), &exist);
    if (status != napi_ok || !exist) {
        MEDIA_LOGE("can not find named property");
        return;
    }

    if (napi_get_named_property(env, configObj, type.c_str(), &item) != napi_ok) {
        MEDIA_LOGE("get named property fail");
        return;
    }

    if (napi_get_value_int32(env, item, configItem) != napi_ok) {
        MEDIA_LOGE("get property value fail");
    }
}

void AudioRecorderNapi::SaveCallbackReference(napi_env env, AudioRecorderNapi &recorderNapi,
    const std::string &callbackName, napi_value callback) const
{
    napi_ref *ref = nullptr;
    if (callbackName == ERROR_CALLBACK_NAME) {
        ref = &(recorderNapi.errorCallback_);
    } else if (callbackName == PREPARE_CALLBACK_NAME) {
        ref = &(recorderNapi.prepareCallback_);
    } else if (callbackName == START_CALLBACK_NAME) {
        ref = &(recorderNapi.startCallback_);
    } else if (callbackName == PAUSE_CALLBACK_NAME) {
        ref = &(recorderNapi.pauseCallback_);
    } else if (callbackName == RESUME_CALLBACK_NAME) {
        ref = &(recorderNapi.resumeCallback_);
    } else if (callbackName == STOP_CALLBACK_NAME) {
        ref = &(recorderNapi.stopCallback_);
    } else if (callbackName == RESET_CALLBACK_NAME) {
        ref = &(recorderNapi.resetCallback_);
    } else if (callbackName == RELEASE_CALLBACK_NAME) {
        ref = &(recorderNapi.releaseCallback_);
    } else {
        MEDIA_LOGE("unknown callback: %{public}s", callbackName.c_str());
        return;
    }

    int32_t refCount = 1;
    napi_status status = napi_create_reference(env, callback, refCount, ref);
    CHECK_AND_RETURN_LOG(status == napi_ok, "creating reference for callback fail")
}
}  // namespace Media
}  // namespace OHOS
