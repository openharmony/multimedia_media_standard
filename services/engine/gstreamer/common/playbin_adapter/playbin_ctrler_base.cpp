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

#include "playbin_ctrler_base.h"
#include "nocopyable.h"
#include "string_ex.h"
#include "media_errors.h"
#include "media_log.h"
#include "uri_helper.h"
#include "scope_guard.h"
#include "playbin_state.h"
#include "gst_utils.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayBinCtrlerBase"};
}

namespace OHOS {
namespace Media {
using PlayBinCtrlerWrapper = ThizWrapper<PlayBinCtrlerBase>;

void PlayBinCtrlerBase::ElementSetup(const GstElement *playbin, GstElement *elem, gpointer userdata)
{
    (void)playbin;
    if (elem == nullptr || userdata == nullptr) {
        return;
    }

    auto thizStrong = PlayBinCtrlerWrapper::TakeStrongThiz(userdata);
    if (thizStrong != nullptr) {
        return thizStrong->OnElementSetup(*elem);
    }
}

PlayBinCtrlerBase::PlayBinCtrlerBase(const PlayBinMsgNotifier &notifier) : notifier_(notifier)
{
    MEDIA_LOGD("enter ctor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
}

PlayBinCtrlerBase::~PlayBinCtrlerBase()
{
    MEDIA_LOGD("enter dtor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
    Reset();
}

int32_t PlayBinCtrlerBase::Init()
{
    idleState_ = std::make_shared<IdleState>(*this);
    initializedState_ = std::make_shared<InitializedState>(*this);
    preparingState_ = std::make_shared<PreparingState>(*this);
    preparedState_ = std::make_shared<PreparedState>(*this);
    playingState_ = std::make_shared<PlayingState>(*this);
    pausedState_ = std::make_shared<PausedState>(*this);
    stoppedState_ = std::make_shared<StoppedState>(*this);

    ChangeState(idleState_);

    taskQueue_ = std::make_unique<TaskQueue>("playbin-ctrl-job");
    int32_t ret = taskQueue_->Start();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    msgQueue_ = std::make_unique<TaskQueue>("playbin-ctrl-msg");
    ret = msgQueue_->Start();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    return ret;
}

int32_t PlayBinCtrlerBase::SetSinkProvider(std::shared_ptr<PlayBinSinkProvider> sinkProvider)
{
    if (sinkProvider == nullptr) {
        return MSERR_INVALID_VAL;
    }

    std::unique_lock<std::mutex> lock(mutex_);
    if (sinkProvider_ != nullptr) {
        MEDIA_LOGE("sink provider is set already");
        return MSERR_INVALID_OPERATION;
    }

    MEDIA_LOGD("set sinkprovider ok");
    sinkProvider_ = sinkProvider;
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::SetScene(PlayBinScene scene)
{
    if (scene >= PlayBinScene::UNKNOWN) {
        MEDIA_LOGE("invalid scene: %{public}hhu", scene);
        return MSERR_INVALID_VAL;
    }

    std::unique_lock<std::mutex> lock(mutex_);
    if (currScene_ == scene) {
        MEDIA_LOGI("set playbin scene %{public}hhu success", scene);
        return MSERR_OK;
    }

    MEDIA_LOGI("set playbin scene %{public}hhu success", scene);
    currScene_ = scene;
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::SetSource(const std::string &uri)
{
    UriHelper uriHeper = UriHelper(uri).FormatMe();
    if ((uriHeper.UriType() != UriHelper::URI_TYPE_FILE) || !uriHeper.AccessCheck(UriHelper::URI_READ)) {
        MEDIA_LOGE("Invalid uri : %{public}s", uri.c_str());
        return MSERR_UNSUPPORT;
    }

    std::unique_lock<std::mutex> lock(mutex_);
    uri_ = uriHeper.FormattedUri();

    MEDIA_LOGI("Set source: %{public}s", uriHeper.FormattedUri().c_str());
    return MSERR_OK;
}

std::string PlayBinCtrlerBase::GetSource()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return uri_;
}

int32_t PlayBinCtrlerBase::Prepare()
{
    MEDIA_LOGD("enter");

    int32_t ret = PrepareAsync();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);
    CHECK_AND_RETURN_RET(preparedTask_ != nullptr, MSERR_UNKNOWN);

    std::unique_lock<std::mutex> lock(mutex_);
    auto result = preparedTask_->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_UNKNOWN, "Prepare failed");
    CHECK_AND_RETURN_RET_LOG(result.Value() == MSERR_OK, result.Value(), "Prepare failed");
    preparedTask_ = nullptr;

    MEDIA_LOGD("exit");
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PrepareAsync()
{
    MEDIA_LOGD("enter");

    std::unique_lock<std::mutex> lock(mutex_);
    if (preparedTask_ != nullptr) {
        return MSERR_OK;
    }

    int32_t ret = EnterInitializedState();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    preparedTask_ = std::make_shared<TaskHandler<int32_t>>([this]() {
        auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
        return currState->Prepare();
    });

    ret = taskQueue_->EnqueueTask(preparedTask_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "PrepareAsync failed");

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::Play()
{
    MEDIA_LOGD("enter");

    std::unique_lock<std::mutex> lock(mutex_);
    auto playingTask = std::make_shared<TaskHandler<void>>([this]() {
        auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
        (void)currState->Play();
    });

    int ret = taskQueue_->EnqueueTask(playingTask);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Play failed");

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::Pause()
{
    MEDIA_LOGD("enter");

    std::unique_lock<std::mutex> lock(mutex_);
    auto pauseTask = std::make_shared<TaskHandler<void>>([this]() {
        auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
        (void)currState->Pause();
    });

    int ret = taskQueue_->EnqueueTask(pauseTask);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Pause failed");

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::Seek(int64_t timeUs, int32_t seekOption)
{
    MEDIA_LOGD("enter");

    std::unique_lock<std::mutex> lock(mutex_);
    auto seekTask = std::make_shared<TaskHandler<void>>([this, timeUs, seekOption]() {
        auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
        (void)currState->Seek(timeUs, seekOption);
    });

    int ret = taskQueue_->EnqueueTask(seekTask);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Seek failed");

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::Stop()
{
    MEDIA_LOGD("enter");

    std::unique_lock<std::mutex> lock(mutex_);
    if (!isInitialized) {
        return MSERR_OK;
    }

    auto stopTask = std::make_shared<TaskHandler<void>>([this]() {
        auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
        (void)currState->Stop();
    });

    int ret = taskQueue_->EnqueueTask(stopTask);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Stop failed");

    return MSERR_OK;
}

void PlayBinCtrlerBase::Reset() noexcept
{
    MEDIA_LOGD("enter");

    std::unique_lock<std::mutex> lock(mutex_);

    isInitialized = false;
    currScene_ = PlayBinScene::UNKNOWN;

    auto idleTask = std::make_shared<TaskHandler<void>>([this]() { ChangeState(idleState_); });
    (void)taskQueue_->EnqueueTask(idleTask, true);

    if (taskQueue_ != nullptr) {
        (void)taskQueue_->Stop();
    }

    if (msgQueue_ != nullptr) {
        (void)msgQueue_->Stop();
    }

    if (msgProcessor_ != nullptr) {
        msgProcessor_->Reset();
        msgProcessor_ = nullptr;
    }

    sinkProvider_ = nullptr;
    elemSetupListener_ = nullptr;

    MEDIA_LOGD("unref playbin start");
    if (playbin_ != nullptr) {
        gst_object_unref(playbin_);
        playbin_ = nullptr;
    }
    MEDIA_LOGD("unref playbin stop");
}

void PlayBinCtrlerBase::SetElemSetupListener(ElemSetupListener listener)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (listener == nullptr) {
        MEDIA_LOGE("nullptr listener");
        return;
    }
    elemSetupListener_ = listener;
}

int32_t PlayBinCtrlerBase::EnterInitializedState()
{
    if (isInitialized) {
        return MSERR_OK;
    }

    MEDIA_LOGD("EnterInitializedState enter");

    if (currScene_ == PlayBinScene::UNKNOWN) {
        MEDIA_LOGE("Set scene firstly!");
        return MSERR_INVALID_OPERATION;
    }

    CHECK_AND_RETURN_RET_LOG(!uri_.empty(), MSERR_INVALID_OPERATION, "Set uri firsty!");

    auto initHandler = std::make_shared<TaskHandler<int32_t>>([this]() {
        auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
        return currState->SetUp();
    });

    int32_t ret = taskQueue_->EnqueueTask(initHandler);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "playbin init failed");

    auto result = initHandler->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_UNKNOWN, "playbin init failed");
    CHECK_AND_RETURN_RET_LOG(result.Value() == MSERR_OK, result.Value(), "playbin init failed");

    isInitialized = true;
    MEDIA_LOGD("EnterInitializedState exit");
    return MSERR_OK;
}

void PlayBinCtrlerBase::SetupCustomElement()
{
    if (sinkProvider_ != nullptr) {
        PlayBinSinkProvider::SinkPtr audSink = sinkProvider_->CreateAudioSink();
        if (audSink != nullptr) {
            g_object_set(playbin_, "audio-sink", audSink, nullptr);
        }
        PlayBinSinkProvider::SinkPtr vidSink = sinkProvider_->CreateVideoSink();
        if (vidSink != nullptr) {
            g_object_set(playbin_, "video-sink", vidSink, nullptr);
        }
    } else {
        MEDIA_LOGD("no sinkprovider, delay the sink selection until the playbin enters pause state.");
    }

    if (currScene_ == PlayBinScene::PLAYBACK) {
        GstElement *audioFilter = gst_element_factory_make("scaletempo", "scaletempo");
        if (audioFilter != nullptr) {
            g_object_set(playbin_, "audio-filter", audioFilter, nullptr);
        } else {
            MEDIA_LOGD("can not create scaletempo, the audio playback speed can not be adjusted");
        }
    }
}

int32_t PlayBinCtrlerBase::SetupSignalMessage()
{
    MEDIA_LOGD("SetupSignalMessage enter");

    PlayBinCtrlerWrapper *wrapper = new(std::nothrow) PlayBinCtrlerWrapper(shared_from_this());
    CHECK_AND_RETURN_RET_LOG(wrapper != nullptr, MSERR_NO_MEMORY, "can not create this wrapper");

    (void)g_signal_connect_data(playbin_, "element-setup", G_CALLBACK(&PlayBinCtrlerBase::ElementSetup),
        wrapper, (GClosureNotify)&PlayBinCtrlerWrapper::OnDestory, static_cast<GConnectFlags>(0));

    GstBus *bus = gst_pipeline_get_bus(playbin_);
    CHECK_AND_RETURN_RET_LOG(bus != nullptr, MSERR_UNKNOWN, "can not get bus");

    auto msgNotifier = std::bind(&PlayBinCtrlerBase::OnMessageReceived, this, std::placeholders::_1);
    msgProcessor_ = std::make_unique<GstMsgProcessor>(*bus, msgNotifier);

    gst_object_unref(bus);
    bus = nullptr;

    int32_t ret = msgProcessor_->Init();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    // only concern the msg from playbin
    msgProcessor_->AddMsgFilter(ELEM_NAME(GST_ELEMENT_CAST(playbin_)));

    MEDIA_LOGD("SetupSignalMessage exit");
    return MSERR_OK;
}

void PlayBinCtrlerBase::OnElementSetup(GstElement &elem)
{
    MEDIA_LOGD("element setup: %{public}s", ELEM_NAME(&elem));

    std::unique_lock<std::mutex> lock(mutex_);
    if (elemSetupListener_ != nullptr) {
        elemSetupListener_(elem);
    }
}

void PlayBinCtrlerBase::DeferTask(const std::shared_ptr<TaskHandler<void>> &task, int64_t delayNs)
{
    (void)delayNs; // taskqueue need support for delay

    if (task != nullptr) {
        int32_t ret = taskQueue_->EnqueueTask(task);
        if (ret != MSERR_OK) {
            MEDIA_LOGE("defer task failed");

            PlayBinMessage msg = {PLAYBIN_MSG_ERROR, 0, MSERR_UNKNOWN, {}};
            ReportMessage(msg);
        }
    }
}

void PlayBinCtrlerBase::OnMessageReceived(const InnerMessage &msg)
{
    auto msgHandler = std::make_shared<TaskHandler<void>>([this, msg]() { HandleMessage(msg); });
    int32_t ret = taskQueue_->EnqueueTask(msgHandler);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("sync process msg failed, type: %{public}d, detail1: %{public}d, detail2: %{public}d",
                   msg.type, msg.detail1, msg.detail2);
    };
}

void PlayBinCtrlerBase::ReportMessage(const PlayBinMessage &msg)
{
    auto msgReportHandler = std::make_shared<TaskHandler<void>>([this, msg]() { notifier_(msg); });
    int32_t ret = msgQueue_->EnqueueTask(msgReportHandler);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("async report msg failed, type: %{public}d, subType: %{public}d, code: %{public}d",
                   msg.type, msg.subType, msg.code);
    };
}
}
}