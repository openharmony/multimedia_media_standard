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

#ifndef AUDIO_RECORDER_NAPI_H_
#define AUDIO_RECORDER_NAPI_H_

#include "napi/native_api.h"
#include "securec.h"
#include "recorder.h"
#include "camera_kit.h"

/* Default Audio Source for Recorder */
static const AudioSourceType DEFAULT_AUDIO_SRC = AUDIO_MIC;

/* Default stop mode for AudioRecorder Stop function */
static const int32_t STOP_MODE = false;

/* Length for event type name */
static const int TYPE_LENGTH = 64;
/* Length for output file path for recorder */
static const int PATH_LENGTH = 128;

/* EventListener for callback methods */
typedef struct _EventListener {
    char type[TYPE_LENGTH];
    napi_ref handlerRef;
    struct _EventListener* back;
    struct _EventListener* next;
} EventListener;

/* Config for storing the Audio Recorder properties */
typedef struct AudioRecorderConfig_ {
    int32_t sampleRate = -1;
    int32_t numberOfChannels = -1;
    int32_t encodeBitRate = -1;
    AudioCodecFormat encoder;
    OHOS::Media::OutputFormatType fileFormat;
    std::string filePath;
} AudioRecorderConfig;

/* Wrapper for Audio Recorder */
class AudioRecorderNapi {
public:
    /* Method for exporting audio recorder properties */
    static napi_value Init(napi_env env, napi_value exports);

    /* Release resources after recording is complete */
    static void Destructor(napi_env env, void* nativeObject, void* finalize_hint);

    /* Send the error callback to the js */
    napi_value SendErrorCallback(napi_env env, int32_t errCode, std::string event, std::string subEvent);

private:
    explicit AudioRecorderNapi(napi_env env, napi_value thisVar);
    virtual ~AudioRecorderNapi();

    /* Create an instance for CreateAudioWrapper */
    static napi_value CreateAudioRecorderWrapper(napi_env env);

    /* Create Audio Recorder and returning the object to JS */
    static napi_value CreateAudioRecorder(napi_env env, napi_callback_info info);

    /* NAPI methods and properties */
    static napi_value JS_Constructor(napi_env env, napi_callback_info cbInfo);
    static napi_value JS_Start(napi_env env, napi_callback_info cbInfo);
    static napi_value JS_Pause(napi_env env, napi_callback_info cbInfo);
    static napi_value JS_Resume(napi_env env, napi_callback_info cbInfo);
    static napi_value JS_Stop(napi_env env, napi_callback_info cbInfo);
    static napi_value JS_Release(napi_env env, napi_callback_info cbInfo);
    static napi_value JS_On(napi_env env, napi_callback_info cbInfo);

    /* Method to register the callbacks */
    virtual void On(std::string type, napi_value handler);

    /* Send the callback information to the js */
    void SendCallback(std::string type, napi_value extras);

    /* Set Audio Recorder Config properties and output file properties */
    napi_value SetFileProps(napi_env env, napi_value confObj, AudioRecorderConfig* config);
    napi_value SetAudioProps(napi_env env, napi_value confObj, AudioRecorderConfig* config, int32_t id);
    void GetAudioConfig(napi_env env, napi_value confObj, std::string type, int32_t* configItem);

    napi_env env_;
    napi_ref wrapper_;
    napi_ref thisVar_;
    EventListener* first_;
    EventListener* last_;
    AudioRecorderConfig* audioConfig_;
    static OHOS::Media::CameraKit *camKit_;
    static napi_ref sConstructor_;
    void *callbackObj_;
    /* Reference to the native Audio recorder instance */
    OHOS::Media::Recorder *recorderObj_;
};

class RecorderCallback : public OHOS::Media::RecorderCallback {
public:
    RecorderCallback(napi_env env = nullptr, AudioRecorderNapi *recorderWrapper = nullptr);
    virtual ~RecorderCallback() {}

    virtual void OnError(int32_t errorType, int32_t errCode);
    void OnInfo(int32_t type, int32_t extra) {}

private:
    napi_env env_;
    AudioRecorderNapi *recorderWrapper_;
};

#endif /* AUDIO_RECORDER_NAPI_H_ */
