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
const int32_t MAXIMUM_PATH_LENGTH = 128;

AudioRecorderNapi::AudioRecorderNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AudioRecorderNapi::~AudioRecorderNapi()
{
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

    if (recorderNapi->callbackNapi_ == nullptr) {
        recorderNapi->callbackNapi_ = std::make_shared<RecorderCallbackNapi>(env);
        (void)recorderNapi->nativeRecorder_->SetRecorderCallback(recorderNapi->callbackNapi_);
    }

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(recorderNapi),
        AudioRecorderNapi::Destructor, nullptr, &(recorderNapi->wrapper_));
    if (status != napi_ok) {
        napi_get_undefined(env, &result);
        delete recorderNapi;
        MEDIA_LOGE("native wrap fail");
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
    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};

    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr || args[0] == nullptr) {
        MEDIA_LOGE("Prepare fail to get napi_get_cb_info");
        return undefinedResult;
    }

    AudioRecorderNapi *recorder = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&recorder));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && recorder != nullptr, undefinedResult, "get recorder napi error");

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_object) {
        recorder->ErrorCallback(env, MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    int32_t sourceId = 0;
    if (recorder->SetFormat(env, args[0], sourceId) != MSERR_OK) {
        recorder->ErrorCallback(env, MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }
    if (recorder->SetAudioProperties(env, args[0], sourceId) != MSERR_OK) {
        recorder->ErrorCallback(env, MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }
    if (recorder->SetUri(env, args[0]) != MSERR_OK) {
        recorder->ErrorCallback(env, MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }
    CHECK_AND_RETURN_RET_LOG(recorder->nativeRecorder_ != nullptr, undefinedResult, "nativeRecorder_ no memory");
    if (recorder->nativeRecorder_->Prepare() != MSERR_OK) {
        recorder->ErrorCallback(env, MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    recorder->StateCallback(env, PREPARE_CALLBACK_NAME);
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
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("Start fail to napi_get_cb_info");
        return undefinedResult;
    }

    AudioRecorderNapi *recorder = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&recorder));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && recorder != nullptr, undefinedResult, "get recorder napi error");

    CHECK_AND_RETURN_RET_LOG(recorder->nativeRecorder_ != nullptr, undefinedResult, "nativeRecorder_ no memory");
    if (recorder->nativeRecorder_->Start() != MSERR_OK) {
        recorder->ErrorCallback(env, MSERR_EXT_UNKNOWN);
        return undefinedResult;
    }

    recorder->StateCallback(env, START_CALLBACK_NAME);
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
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("Pause fail to napi_get_cb_info");
        return undefinedResult;
    }

    AudioRecorderNapi *recorder = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&recorder));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && recorder != nullptr, undefinedResult, "get recorder napi error");

    CHECK_AND_RETURN_RET_LOG(recorder->nativeRecorder_ != nullptr, undefinedResult, "nativeRecorder_ no memory");
    if (recorder->nativeRecorder_->Pause() != MSERR_OK) {
        recorder->ErrorCallback(env, MSERR_EXT_UNKNOWN);
        return undefinedResult;
    }

    recorder->StateCallback(env, PAUSE_CALLBACK_NAME);
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
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("Resume fail to napi_get_cb_info");
        return undefinedResult;
    }

    AudioRecorderNapi *recorder = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&recorder));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && recorder != nullptr, undefinedResult, "get recorder napi error");

    CHECK_AND_RETURN_RET_LOG(recorder->nativeRecorder_ != nullptr, undefinedResult, "nativeRecorder_ no memory");
    if (recorder->nativeRecorder_->Resume() != MSERR_OK) {
        recorder->ErrorCallback(env, MSERR_EXT_UNKNOWN);
        return undefinedResult;
    }

    recorder->StateCallback(env, RESUME_CALLBACK_NAME);
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
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("Stop fail to napi_get_cb_info");
        return undefinedResult;
    }

    AudioRecorderNapi *recorder = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&recorder));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && recorder != nullptr, undefinedResult, "get recorder napi error");

    CHECK_AND_RETURN_RET_LOG(recorder->nativeRecorder_ != nullptr, undefinedResult, "nativeRecorder_ no memory");
    if (recorder->nativeRecorder_->Stop(false) != MSERR_OK) {
        recorder->ErrorCallback(env, MSERR_EXT_UNKNOWN);
        return undefinedResult;
    }

    recorder->StateCallback(env, STOP_CALLBACK_NAME);
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
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("Reset fail to napi_get_cb_info");
        return undefinedResult;
    }

    AudioRecorderNapi *recorder = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&recorder));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && recorder != nullptr, undefinedResult, "get recorder napi error");

    CHECK_AND_RETURN_RET_LOG(recorder->nativeRecorder_ != nullptr, undefinedResult, "nativeRecorder_ no memory");
    if (recorder->nativeRecorder_->Reset() != MSERR_OK) {
        recorder->ErrorCallback(env, MSERR_EXT_UNKNOWN);
        return undefinedResult;
    }

    recorder->StateCallback(env, RESET_CALLBACK_NAME);

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
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("Reset fail to napi_get_cb_info");
        return undefinedResult;
    }

    AudioRecorderNapi *recorder = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&recorder));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && recorder != nullptr, undefinedResult, "get recorder napi error");

    CHECK_AND_RETURN_RET_LOG(recorder->nativeRecorder_ != nullptr, undefinedResult, "nativeRecorder_ no memory");
    if (recorder->nativeRecorder_->Release() != MSERR_OK) {
        recorder->ErrorCallback(env, MSERR_EXT_UNKNOWN);
        return undefinedResult;
    }

    recorder->StateCallback(env, RELEASE_CALLBACK_NAME);

    recorder->callbackNapi_ = nullptr;
    recorder->filePath_ = "";

    MEDIA_LOGD("Release success");
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
        MEDIA_LOGE("On fail to napi_get_cb_info");
        return undefinedResult;
    }

    AudioRecorderNapi *recorder = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&recorder));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && recorder != nullptr, undefinedResult, "get recorder napi error");

    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string ||
        napi_typeof(env, args[1], &valueType1) != napi_ok || valueType1 != napi_function) {
        recorder->ErrorCallback(env, MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    std::string callbackName = CommonNapi::GetStringArgument(env, args[0]);
    MEDIA_LOGD("callbackName: %{public}s", callbackName.c_str());

    CHECK_AND_RETURN_RET_LOG(recorder->callbackNapi_ != nullptr, undefinedResult, "callbackNapi_ is nullptr");
    std::shared_ptr<RecorderCallbackNapi> cb = std::static_pointer_cast<RecorderCallbackNapi>(recorder->callbackNapi_);
    cb->SaveCallbackReference(callbackName, args[1]);
    return undefinedResult;
}

int32_t AudioRecorderNapi::SetFormat(napi_env env, napi_value args, int32_t &sourceId)
{
    int32_t ret = -1;
    AudioSourceType nativeSourceType = AUDIO_SOURCE_DEFAULT;
    CommonNapi::GetPropertyInt32(env, args, "AudioSourceType", ret);
    switch (ret) {
        case JS_MIC:
            nativeSourceType = AUDIO_MIC;
            break;
        default:
            break;
    }
    CHECK_AND_RETURN_RET_LOG(this->nativeRecorder_ != nullptr, MSERR_INVALID_OPERATION, "nativeRecorder_ no memory");

    if (this->nativeRecorder_->SetAudioSource(nativeSourceType, sourceId) != MSERR_OK) {
        this->ErrorCallback(env, MSERR_EXT_INVALID_VAL);
        return MSERR_INVALID_OPERATION;
    }

    OutputFormatType nativeOutputFormatType = FORMAT_DEFAULT;
    CommonNapi::GetPropertyInt32(env, args, "fileFormat", ret);
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
    if (this->nativeRecorder_->SetOutputFormat(nativeOutputFormatType) != MSERR_OK) {
        this->ErrorCallback(env, MSERR_EXT_INVALID_VAL);
        return MSERR_INVALID_OPERATION;
    }
    return MSERR_OK;
}

int32_t AudioRecorderNapi::SetAudioProperties(napi_env env, napi_value args, int32_t sourceId)
{
    int32_t ret = -1;
    AudioCodecFormat nativeAudioCodecFormat = AUDIO_DEFAULT;
    CommonNapi::GetPropertyInt32(env, args, "audioEncoder", ret);
    switch (ret) {
        case JS_AAC_LC:
            nativeAudioCodecFormat = AAC_LC;
            break;
        default:
            break;
    }
    CHECK_AND_RETURN_RET_LOG(this->nativeRecorder_ != nullptr, MSERR_INVALID_OPERATION, "nativeRecorder_ no memory");
    if (this->nativeRecorder_->SetAudioEncoder(sourceId, nativeAudioCodecFormat) != MSERR_OK) {
        this->ErrorCallback(env, MSERR_EXT_INVALID_VAL);
        return MSERR_INVALID_OPERATION;
    }

    CommonNapi::GetPropertyInt32(env, args, "audioEncodeBitRate", ret);
    if (ret == -1) {
        ret = DEFAULT_AUDIO_ENCODER_BIT_RATE;
    }
    if (this->nativeRecorder_->SetAudioEncodingBitRate(sourceId, ret) != MSERR_OK) {
        this->ErrorCallback(env, MSERR_EXT_INVALID_VAL);
        return MSERR_INVALID_OPERATION;
    }

    CommonNapi::GetPropertyInt32(env, args, "audioSampleRate", ret);
    if (ret == -1) {
        ret = DEFAULT_AUDIO_SAMPLE_RATE;
    }
    if (this->nativeRecorder_->SetAudioSampleRate(sourceId, ret) != MSERR_OK) {
        this->ErrorCallback(env, MSERR_EXT_INVALID_VAL);
        return MSERR_INVALID_OPERATION;
    }

    CommonNapi::GetPropertyInt32(env, args, "numberOfChannels", ret);
    if (ret == -1) {
        ret = DEFAULT_NUMBER_OF_CHANNELS;
    }
    if (this->nativeRecorder_->SetAudioChannels(sourceId, ret) != MSERR_OK) {
        this->ErrorCallback(env, MSERR_EXT_INVALID_VAL);
        return MSERR_INVALID_OPERATION;
    }

    return MSERR_OK;
}

int32_t AudioRecorderNapi::CheckValidPath(const std::string &path)
{
    std::string realPath;
    if (!PathToRealPath(path, realPath)) {
        MEDIA_LOGE("Configured output path invalid, ignore !");
        return MSERR_INVALID_VAL;
    }
    struct stat s;
    if (stat(realPath.c_str(), &s) != 0) {
        MEDIA_LOGE("Configured output path invalid, ignore !");
        return MSERR_INVALID_VAL;
    }
    if ((s.st_mode & S_IFREG) == 0) {
        MEDIA_LOGE("Configured output path invalid, ignore !");
        return MSERR_INVALID_VAL;
    }
    filePath_ = realPath;
    return MSERR_OK;
}

int32_t AudioRecorderNapi::SetUri(napi_env env, napi_value args)
{
    bool exist = false;
    napi_status status = napi_has_named_property(env, args, "uri", &exist);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && exist, MSERR_INVALID_OPERATION, "can not find uri property");

    napi_value configItem = nullptr;
    status = napi_get_named_property(env, args, "uri", &configItem);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, MSERR_INVALID_OPERATION, "can not get uri property");
    char buffer[MAXIMUM_PATH_LENGTH] = {0};
    size_t bytesToCopy = 0;
    status = napi_get_value_string_latin1(env, configItem, buffer, MAXIMUM_PATH_LENGTH - 1, &bytesToCopy);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, MSERR_INVALID_OPERATION, "can not get uri content");
    CHECK_AND_RETURN_RET_LOG(nativeRecorder_ != nullptr, MSERR_INVALID_OPERATION, "nativeRecorder_ no memory");

    std::string uriPath = buffer;
    const std::string fileHead = "file://";
    const std::string fdHead = "fd://";
    int32_t fdNum = -1;

    if (uriPath.find(fileHead) != std::string::npos) {
        std::string filePath = uriPath.substr(fileHead.size());
        CHECK_AND_RETURN_RET(CheckValidPath(filePath) == MSERR_OK, MSERR_INVALID_VAL);
        CHECK_AND_RETURN_RET(!filePath_.empty(), MSERR_INVALID_VAL);
        fdNum = open(filePath_.c_str(), O_CREAT | O_WRONLY, 0664); // rw-rw-r
        CHECK_AND_RETURN_RET(fdNum >= 0, MSERR_INVALID_OPERATION);
        int32_t ret = nativeRecorder_->SetOutputFile(fdNum);
        if (ret != MSERR_OK) {
            close(fdNum);
            return MSERR_INVALID_OPERATION;
        }
        close(fdNum);
    } else if (uriPath.find(fdHead) != std::string::npos) {
        std::string inputFd = uriPath.substr(fdHead.size());
        CHECK_AND_RETURN_RET(StrToInt(inputFd, fdNum) == true, MSERR_INVALID_VAL);
        CHECK_AND_RETURN_RET(fdNum >= 0, MSERR_INVALID_OPERATION);
        CHECK_AND_RETURN_RET(nativeRecorder_->SetOutputFile(fdNum) == MSERR_OK, MSERR_INVALID_OPERATION);
    } else {
        MEDIA_LOGE("invalid input uri, neither file nor fd!");
        return MSERR_INVALID_OPERATION;
    }

    return MSERR_OK;
}

void AudioRecorderNapi::ErrorCallback(napi_env env, MediaServiceExtErrCode errCode)
{
    if (callbackNapi_ != nullptr) {
        std::shared_ptr<RecorderCallbackNapi> napiCb = std::static_pointer_cast<RecorderCallbackNapi>(callbackNapi_);
        napiCb->SendErrorCallback(env, errCode);
    }
}

void AudioRecorderNapi::StateCallback(napi_env env, const std::string &callbackName)
{
    if (callbackNapi_ != nullptr) {
        std::shared_ptr<RecorderCallbackNapi> napiCb = std::static_pointer_cast<RecorderCallbackNapi>(callbackNapi_);
        napiCb->SendCallback(env, callbackName);
    }
}
}  // namespace Media
}  // namespace OHOS
