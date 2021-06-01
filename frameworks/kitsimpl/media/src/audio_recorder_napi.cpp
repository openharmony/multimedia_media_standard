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
#include "hilog/log.h"

using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

namespace {
    const int PARAM0 = 0;
    const int PARAM1 = 1;
    const int ARGS_ONE = 1;
    const int ARGS_TWO = 2;
    constexpr HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioRecorderNapi"};
}

napi_ref AudioRecorderNapi::sConstructor_ = nullptr;
OHOS::Media::CameraKit* AudioRecorderNapi::camKit_ = nullptr;

AudioRecorderNapi::AudioRecorderNapi(napi_env env, napi_value thisVar)
{
    uint32_t initialRefCount = 1;
    env_ = env;
    first_ = last_ = nullptr;
    thisVar_ = nullptr;
    wrapper_ = nullptr;
    recorderObj_ = nullptr;
    audioConfig_ = nullptr;
    callbackObj_ = nullptr;
    napi_create_reference(env, thisVar, initialRefCount, &thisVar_);
}

AudioRecorderNapi::~AudioRecorderNapi()
{
    EventListener* temp = nullptr;
    for (EventListener* i = first_; i != nullptr; i = temp) {
        temp = i->next;
        if (i == first_) {
            first_ = first_->next;
        } else if (i == last_) {
            last_ = last_->back;
        } else {
            i->next->back = i->back;
            i->back->next = i->next;
        }
        napi_delete_reference(env_, i->handlerRef);
        delete i;
    }

    if (audioConfig_ != nullptr) {
        delete audioConfig_;
        audioConfig_ = nullptr;
    }

    if (camKit_ != nullptr) {
        delete camKit_;
        camKit_ = nullptr;
    }

    if (callbackObj_ != nullptr) {
        delete (RecorderCallback *)callbackObj_;
        callbackObj_ = nullptr;
    }

    if (thisVar_ != nullptr) {
        napi_delete_reference(env_, thisVar_);
    }

    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }

    if (recorderObj_ != nullptr) {
        delete recorderObj_;
        recorderObj_ = nullptr;
    }
}

RecorderCallback::RecorderCallback(napi_env env, AudioRecorderNapi *recorderWrapper)
    : env_(env), recorderWrapper_(recorderWrapper)
{
}

void RecorderCallback::OnError(int32_t errorType, int32_t errCode)
{
    napi_value status = recorderWrapper_->SendErrorCallback(env_, errCode, "OnError", "");
    if (status != nullptr) {
        return;
    }

    HiLog::Error(LABEL, "Failed to call error callback!");
}

void AudioRecorderNapi::Destructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    if (nativeObject != nullptr) {
        reinterpret_cast<AudioRecorderNapi*>(nativeObject)->~AudioRecorderNapi();
    }
}

napi_value AudioRecorderNapi::Init(napi_env env, napi_value exports)
{
    const char className[] = "AudioRecorder";
    napi_value constructor;
    napi_status status;
    uint32_t initialRefCount = 1;

    HiLog::Debug(LABEL, "Inside Recorder Init ");
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("start", JS_Start),
        DECLARE_NAPI_FUNCTION("pause", JS_Pause),
        DECLARE_NAPI_FUNCTION("resume", JS_Resume),
        DECLARE_NAPI_FUNCTION("stop", JS_Stop),
        DECLARE_NAPI_FUNCTION("release", JS_Release),
        DECLARE_NAPI_FUNCTION("on", JS_On)
    };

    napi_property_descriptor static_prop[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createAudioRecorder", CreateAudioRecorder),
    };

    status = napi_define_class(env, className, sizeof(className), JS_Constructor, nullptr,
        sizeof(properties) / sizeof(properties[PARAM0]), properties, &constructor);
    if (status == napi_ok) {
        status = napi_create_reference(env, constructor, initialRefCount, &sConstructor_);
        if (status == napi_ok) {
            napi_set_named_property(env, exports, "AudioRecorder", constructor);
            if (status == napi_ok) {
                status = napi_define_properties(env, exports, sizeof(static_prop) / sizeof(static_prop[PARAM0]),
                    static_prop);
                if (status == napi_ok) {
                    HiLog::Info(LABEL, "All props and functions are configured..");
                    return exports;
                }
            }
        }
    }

    HiLog::Error(LABEL, "Failure in AudioRecorderNapi::Init()");
    return exports;
}

napi_value AudioRecorderNapi::JS_Constructor(napi_env env, napi_callback_info cbInfo)
{
    napi_status status;
    napi_value args[ARGS_ONE], thisVar;
    size_t argCount = 0;

    HiLog::Debug(LABEL, "Inside JS_Constructor");
    status = napi_get_cb_info(env, cbInfo, &argCount, args, &thisVar, nullptr);
    if (status == napi_ok) {
        AudioRecorderNapi* audioRecorder = new AudioRecorderNapi(env, thisVar);
        if (audioRecorder != nullptr) {
            audioRecorder->recorderObj_ =  new OHOS::Media::Recorder();

            status = napi_wrap(env, thisVar, audioRecorder,
                AudioRecorderNapi::Destructor, nullptr, &(audioRecorder->wrapper_));
            if (status == napi_ok) {
                audioRecorder->callbackObj_ = (void *)(new RecorderCallback(env, audioRecorder));
                audioRecorder->recorderObj_->SetRecorderCallback(
                    reinterpret_cast<std::shared_ptr<RecorderCallback>&>(audioRecorder->callbackObj_));
                return thisVar;
            }
        }
    }

    HiLog::Error(LABEL, "Failed in AudioRecorderNapi::New()!");
    return nullptr;
}

napi_value AudioRecorderNapi::CreateAudioRecorderWrapper(napi_env env)
{
    napi_status status;
    napi_value result, constructor;
    const int argCount = 0;

    status = napi_get_reference_value(env, sConstructor_, &constructor);
    if (status == napi_ok) {
        status = napi_new_instance(env, constructor, argCount, nullptr, &result);
        if (status == napi_ok) {
            HiLog::Info(LABEL, "New instance creation for Recorder Napi successful");
            return result;
        } else {
            HiLog::Error(LABEL, "New instance creation for Recorder Napi failed");
        }
    }

    HiLog::Error(LABEL, "Failed in AudioRecorderNapi::CreateAudioRecorderWrapper!");
    return nullptr;
}

napi_value AudioRecorderNapi::CreateAudioRecorder(napi_env env, napi_callback_info info)
{
    // Needed to start the recorder module
    if (camKit_ == nullptr) {
        camKit_ = OHOS::Media::CameraKit::GetInstance();
    }

    return AudioRecorderNapi::CreateAudioRecorderWrapper(env);
}

void AudioRecorderNapi::On(std::string type, napi_value handler)
{
    uint32_t initialRefCount = 1;
    HiLog::Info(LABEL, "Inside AudioRecorderNapi::On!");
    if (first_ == nullptr) {
        HiLog::Error(LABEL, "No existing listeners present");
        first_ = last_ = new EventListener();
    } else {
        last_->next = new EventListener();
        last_->next->back = last_;
        last_ = last_->next;
    }
    if (strncpy_s(last_->type, TYPE_LENGTH - 1, type.c_str(), type.length()) == 0) {
        napi_create_reference(env_, handler, initialRefCount, &last_->handlerRef);
    } else {
        HiLog::Error(LABEL, "Inside AudioRecorderNapi::On(): Failed while performing string copy for type!");
    }
}

void AudioRecorderNapi::SendCallback(std::string type, napi_value extras)
{
    napi_handle_scope scope;
    napi_value handler = nullptr;
    napi_value thisVar = nullptr;
    napi_value result;
    napi_status status;

    HiLog::Info(LABEL, "Inside AudioRecorderNapi::SendCallback with event = %{public}s", type.c_str());
    status = napi_open_handle_scope(env_, &scope);
    if (status == napi_ok) {
        status = napi_get_reference_value(env_, thisVar_, &thisVar);
        if (status == napi_ok) {
            for (EventListener* i = first_; i != nullptr; i = i->next) {
                if (strcmp(i->type, type.c_str()) == 0) {
                    status = napi_get_reference_value(env_, i->handlerRef, &handler);
                    if (status == napi_ok) {
                        napi_call_function(env_, thisVar, handler,
                            extras ? 1 : 0, extras ? &extras : nullptr, &result);
                    }
                }
            }
        }
        napi_close_handle_scope(env_, scope);
    }
}

napi_value AudioRecorderNapi::SendErrorCallback(napi_env env, int32_t errCode,
    std::string event, std::string subEvent)
{
    napi_value code = nullptr;
    napi_value msg = nullptr;
    napi_value errCb = nullptr;
    napi_value result = nullptr;

    std::string str = "Failed in: " + event;
    if (subEvent.length() != 0) {
        str += " while performing " + subEvent;
    }
    napi_create_string_utf8(env, str.c_str(), NAPI_AUTO_LENGTH, &msg);
    napi_create_int32(env, errCode, &code);

    napi_create_error(env, code, msg, &errCb);
    this->SendCallback("error", errCb);

    napi_get_undefined(env, &result);
    return result;
}

void AudioRecorderNapi::GetAudioConfig(napi_env env, napi_value confObj, std::string type, int32_t* configItem)
{
    napi_value item;
    napi_status status;
    bool ifExist = false;

    status = napi_has_named_property(env, confObj, type.c_str(), &ifExist);
    if (status == napi_ok && ifExist) {
        if (napi_get_named_property(env, confObj, type.c_str(), &item) == napi_ok) {
            if (napi_get_value_int32(env, item, configItem) == napi_ok) {
                return;
            }
        }
    }

    HiLog::Error(LABEL, "GetAudioConfig: Could not get audio config");
}

napi_value AudioRecorderNapi::SetAudioProps(napi_env env, napi_value confObj, AudioRecorderConfig* config, int32_t id)
{
    int32_t retVal;
    int32_t encoderType = -1;

    this->GetAudioConfig(env, confObj, "sampleRate", &config->sampleRate);
    if (config->sampleRate != -1) {
        retVal = this->recorderObj_->SetAudioSampleRate(id, config->sampleRate);
        if (retVal != 0) {
            return this->SendErrorCallback(env, retVal, "Start", "SetAudioChannels");
        }
    }

    this->GetAudioConfig(env, confObj, "numberOfChannels", &config->numberOfChannels);
    if (config->numberOfChannels != -1) {
        retVal = this->recorderObj_->SetAudioChannels(id, config->numberOfChannels);
        if (retVal != 0) {
            return this->SendErrorCallback(env, retVal, "Start", "SetAudioChannels");
        }
    }

    this->GetAudioConfig(env, confObj, "encodeBitRate", &config->encodeBitRate);
    if (config->encodeBitRate != -1) {
        retVal = this->recorderObj_->SetAudioEncodingBitRate(id, config->encodeBitRate);
        if (retVal != 0) {
            return this->SendErrorCallback(env, retVal, "Start", "SetAudioEncodingBitRate");
        }
    }

    this->GetAudioConfig(env, confObj, "encoder", &encoderType);
    if (encoderType != -1) {
        config->encoder = static_cast<AudioCodecFormat>(encoderType);
        retVal = this->recorderObj_->SetAudioEncoder(id, config->encoder);
        if (retVal != 0) {
            return this->SendErrorCallback(env, retVal, "Start", "SetAudioEncoder");
        }
    }

    return nullptr;
}

napi_value AudioRecorderNapi::SetFileProps(napi_env env, napi_value confObj, AudioRecorderConfig* config)
{
    napi_value configItem;
    napi_status status;
    bool ifExist = false;
    int32_t retVal, fileFormatType;
    size_t bytesToCopy;
    char buffer[PATH_LENGTH];
    napi_value result = nullptr;

    status = napi_has_named_property(env, confObj, "fileFormat", &ifExist);
    if (status == napi_ok && ifExist) {
        status = napi_get_named_property(env, confObj, "fileFormat", &configItem);
        if (status == napi_ok) {
            status = napi_get_value_int32(env, configItem, &fileFormatType);
            if (status == napi_ok) {
                config->fileFormat = static_cast<OHOS::Media::OutputFormatType>(fileFormatType);
                retVal = this->recorderObj_->SetOutputFormat(config->fileFormat);
                if (retVal != 0) {
                    return this->SendErrorCallback(env, retVal, "Start", "SetFileFormat");
                }
            }
        }
    }

    status = napi_has_named_property(env, confObj, "filePath", &ifExist);
    if (status == napi_ok && ifExist) {
        status = napi_get_named_property(env, confObj, "filePath", &configItem);
        if (status == napi_ok) {
            status = napi_get_value_string_latin1(env, configItem, buffer, PATH_LENGTH - 1, &bytesToCopy);
            if (status == napi_ok) {
                config->filePath = buffer;
                retVal = this->recorderObj_->SetOutputPath(config->filePath);
                if (retVal != 0) {
                    return this->SendErrorCallback(env, retVal, "Start", "SetFilePath");
                }
            }
        }
    } else if (!ifExist) {
        HiLog::Error(LABEL, "Failed to get Output File Path for audio recorder");
        napi_get_undefined(env, &result);
        return result;
    }

    return nullptr;
}

napi_value AudioRecorderNapi::JS_Start(napi_env env, napi_callback_info cbInfo)
{
    size_t argc = ARGS_ONE;
    napi_value args[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    napi_value result = nullptr;
    napi_valuetype valueType;
    AudioRecorderNapi* audioRecorder = nullptr;
    int32_t sourceId, retVal;

    HiLog::Debug(LABEL, "Inside AudioRecorderNapi::JS_Start!");
    if (napi_get_cb_info(env, cbInfo, &argc, args, &thisVar, nullptr) != napi_ok || argc != ARGS_ONE) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return nullptr;
    }

    if (napi_unwrap(env, thisVar, (void**)&audioRecorder) == napi_ok) {
        if (napi_typeof(env, args[PARAM0], &valueType) != napi_ok || valueType != napi_object) {
            HiLog::Error(LABEL, "Invalid arguments!");
            return nullptr;
        }

        audioRecorder->audioConfig_ = new AudioRecorderConfig;
        if ((retVal = audioRecorder->recorderObj_->SetAudioSource(DEFAULT_AUDIO_SRC, sourceId)) != 0) {
            return audioRecorder->SendErrorCallback(env, retVal, "Start", "SetAudioSource");
        }

        if ((result = audioRecorder->SetAudioProps(env, args[PARAM0],
            audioRecorder->audioConfig_, sourceId)) != nullptr) {
            return result;
        }

        if ((result = audioRecorder->SetFileProps(env, args[PARAM0], audioRecorder->audioConfig_)) != nullptr) {
            return result;
        }

        /* Prepare the Audio recorder */
        if ((retVal = audioRecorder->recorderObj_->Prepare()) != 0) {
            return audioRecorder->SendErrorCallback(env, retVal, "Start", "Prepare");
        }

        if ((retVal = audioRecorder->recorderObj_->Start()) != 0) {
            return audioRecorder->SendErrorCallback(env, retVal, "Start", "");
        } else {
            audioRecorder->SendCallback("start", nullptr);
        }
    }

    napi_get_undefined(env, &result);
    return result;
}

napi_value AudioRecorderNapi::JS_Pause(napi_env env, napi_callback_info cbInfo)
{
    size_t argc = 0;
    napi_value thisVar = nullptr;
    int32_t retVal;
    napi_status status;
    napi_value result = nullptr;
    AudioRecorderNapi* audioRecorder = nullptr;

    HiLog::Debug(LABEL, "Inside AudioRecorderNapi::JS_Pause!");
    status = napi_get_cb_info(env, cbInfo, &argc, nullptr, &thisVar, nullptr);
    if (status != napi_ok || argc != 0) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return nullptr;
    }

    status = napi_unwrap(env, thisVar, (void**)&audioRecorder);
    if (status == napi_ok) {
        retVal = audioRecorder->recorderObj_->Pause();
        if (retVal != OHOS::Media::SUCCESS) {
            return audioRecorder->SendErrorCallback(env, retVal, "Pause", "");
        } else {
            audioRecorder->SendCallback("pause", nullptr);
        }
    }

    napi_get_undefined(env, &result);
    return result;
}

napi_value AudioRecorderNapi::JS_Resume(napi_env env, napi_callback_info cbInfo)
{
    size_t argc = 0;
    napi_value thisVar = nullptr;
    int32_t retVal;
    napi_status status;
    napi_value result = nullptr;
    AudioRecorderNapi* audioRecorder = nullptr;

    HiLog::Debug(LABEL, "Inside AudioRecorderNapi::JS_Resume!");
    status = napi_get_cb_info(env, cbInfo, &argc, nullptr, &thisVar, nullptr);
    if (status != napi_ok || argc != 0) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return nullptr;
    }

    status = napi_unwrap(env, thisVar, (void**)&audioRecorder);
    if (status == napi_ok) {
        retVal = audioRecorder->recorderObj_->Resume();
        if (retVal != OHOS::Media::SUCCESS) {
            return audioRecorder->SendErrorCallback(env, retVal, "Resume", "");
        } else {
            audioRecorder->SendCallback("resume", nullptr);
        }
    }

    napi_get_undefined(env, &result);
    return result;
}

napi_value AudioRecorderNapi::JS_Stop(napi_env env, napi_callback_info cbInfo)
{
    size_t argc = 0;
    napi_value thisVar = nullptr;
    int32_t retVal;
    napi_status status;
    napi_value result = nullptr;
    AudioRecorderNapi* audioRecorder = nullptr;

    HiLog::Debug(LABEL, "Inside AudioRecorderNapi::JS_Stop!");
    status = napi_get_cb_info(env, cbInfo, &argc, nullptr, &thisVar, nullptr);
    if (status != napi_ok || argc != 0) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return nullptr;
    }

    status = napi_unwrap(env, thisVar, (void**)&audioRecorder);
    if (status == napi_ok) {
        retVal = audioRecorder->recorderObj_->Stop(STOP_MODE);
        if (retVal != OHOS::Media::SUCCESS) {
            return audioRecorder->SendErrorCallback(env, retVal, "Stop", "");
        } else {
            audioRecorder->SendCallback("stop", nullptr);
        }
    }

    napi_get_undefined(env, &result);
    return result;
}

napi_value AudioRecorderNapi::JS_Release(napi_env env, napi_callback_info cbInfo)
{
    size_t argc = 0;
    napi_value thisVar = nullptr;
    int32_t retVal;
    napi_status status;
    napi_value result = nullptr;
    AudioRecorderNapi* audioRecorder = nullptr;

    HiLog::Debug(LABEL, "Inside AudioRecorderNapi::JS_Release!");
    status = napi_get_cb_info(env, cbInfo, &argc, nullptr, &thisVar, nullptr);
    if (status != napi_ok || argc != 0) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return nullptr;
    }

    status = napi_unwrap(env, thisVar, (void**)&audioRecorder);
    if (status == napi_ok) {
        retVal = audioRecorder->recorderObj_->Release();
        if (retVal != OHOS::Media::SUCCESS) {
            return audioRecorder->SendErrorCallback(env, retVal, "Release", "");
        } else {
            audioRecorder->SendCallback("release", nullptr);
        }
    }

    napi_get_undefined(env, &result);
    return result;
}

napi_value AudioRecorderNapi::JS_On(napi_env env, napi_callback_info cbInfo)
{
    size_t argc = ARGS_TWO;
    napi_value args[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;
    napi_status status;
    napi_valuetype eventValueType, eventHandleType;
    napi_value result = nullptr;
    AudioRecorderNapi* audioRecorder = nullptr;
    char type[TYPE_LENGTH] = {0};
    size_t typeLen = 0;
    std::string typeStr;

    HiLog::Debug(LABEL, "Inside AudioRecorderNapi::JS_On!");
    status = napi_get_cb_info(env, cbInfo, &argc, args, &thisVar, nullptr);
    if (status != napi_ok || argc != ARGS_TWO) {
        HiLog::Error(LABEL, "Invalid arguments!");
        return nullptr;
    }

    status = napi_unwrap(env, thisVar, (void**)&audioRecorder);
    if (status == napi_ok) {
        if (napi_typeof(env, args[PARAM0], &eventValueType) != napi_ok || eventValueType != napi_string
            || napi_typeof(env, args[PARAM1], &eventHandleType) != napi_ok || eventHandleType != napi_function) {
            HiLog::Error(LABEL, "Invalid arguments!");
            return nullptr;
        }

        status = napi_get_value_string_utf8(env, args[PARAM0], type, TYPE_LENGTH - 1, &typeLen);
        typeStr = type;
        if (status == napi_ok) {
            audioRecorder->On(typeStr, args[PARAM1]);
        }
    }

    napi_get_undefined(env, &result);
    return result;
}
