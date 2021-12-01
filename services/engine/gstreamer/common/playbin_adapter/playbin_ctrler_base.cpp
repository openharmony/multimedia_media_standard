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
#include <gst/playback/gstplay-enum.h>
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
static const std::unordered_map<int32_t, int32_t> SEEK_OPTION_TO_GST_SEEK_FLAGS = {
    {
        IPlayBinCtrler::PlayBinSeekMode::PREV_SYNC,
        GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT | GST_SEEK_FLAG_SNAP_BEFORE,
    },
    {
        IPlayBinCtrler::PlayBinSeekMode::NEXT_SYNC,
        GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT | GST_SEEK_FLAG_SNAP_AFTER,
    },
    {
        IPlayBinCtrler::PlayBinSeekMode::CLOSET_SYNC,
        GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT | GST_SEEK_FLAG_SNAP_NEAREST,
    },
    {
        IPlayBinCtrler::PlayBinSeekMode::CLOSET,
        GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
    }
};

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

PlayBinCtrlerBase::PlayBinCtrlerBase(const PlayBinCreateParam &createParam)
    : renderMode_(createParam.renderMode),
    notifier_(createParam.notifier),
    sinkProvider_(createParam.sinkProvider)
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
    CHECK_AND_RETURN_RET_LOG(sinkProvider_ != nullptr, MSERR_INVALID_VAL, "sinkprovider is nullptr");

    idleState_ = std::make_shared<IdleState>(*this);
    initializedState_ = std::make_shared<InitializedState>(*this);
    preparingState_ = std::make_shared<PreparingState>(*this);
    preparedState_ = std::make_shared<PreparedState>(*this);
    playingState_ = std::make_shared<PlayingState>(*this);
    pausedState_ = std::make_shared<PausedState>(*this);
    stoppedState_ = std::make_shared<StoppedState>(*this);

    ChangeState(idleState_);

    int32_t ret = taskMgr_.Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "task mgr init failed");

    msgQueue_ = std::make_unique<TaskQueue>("playbin-ctrl-msg");
    ret = msgQueue_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "msgqueue start failed");

    return ret;
}

int32_t PlayBinCtrlerBase::SetSource(const std::string &url)
{
    UriHelper uriHeper = UriHelper(url).FormatMe();
    if ((uriHeper.UriType() != UriHelper::URI_TYPE_FILE) || !uriHeper.AccessCheck(UriHelper::URI_READ)) {
        MEDIA_LOGE("Invalid url : %{public}s", url.c_str());
        return MSERR_UNSUPPORT;
    }

    std::unique_lock<std::mutex> lock(mutex_);
    uri_ = uriHeper.FormattedUri();

    MEDIA_LOGI("Set source: %{public}s", uriHeper.FormattedUri().c_str());
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::Prepare()
{
    MEDIA_LOGD("enter");

    std::unique_lock<std::mutex> lock(mutex_);

    int32_t ret = PrepareAsyncInternel();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    {
        std::unique_lock<std::mutex> condLock(condMutex_);
        stateCond_.wait(condLock, [this]() {
            return GetCurrState() == preparedState_ || isErrorHappened_;
        });
    }

    if (GetCurrState() != preparedState_) {
        MEDIA_LOGE("Prepare failed");
        return MSERR_UNKNOWN;
    }

    MEDIA_LOGD("exit");
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PrepareAsync()
{
    MEDIA_LOGD("enter");

    std::unique_lock<std::mutex> lock(mutex_);
    return PrepareAsyncInternel();
}

int32_t PlayBinCtrlerBase::Play()
{
    MEDIA_LOGD("enter");

    std::unique_lock<std::mutex> lock(mutex_);

    if (GetCurrState() == playingState_) {
        MEDIA_LOGI("already at playing state, skip");
        return MSERR_OK;
    }

    auto playingTask = std::make_shared<TaskHandler<void>>([this]() {
        auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
        (void)currState->Play();
    });

    int ret = taskMgr_.LaunchTask(playingTask, PlayBinTaskType::STATE_CHANGE);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Play failed");

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::Pause()
{
    MEDIA_LOGD("enter");

    std::unique_lock<std::mutex> lock(mutex_);

    if (GetCurrState() == pausedState_ || GetCurrState() == preparedState_) {
        MEDIA_LOGI("already at paused state, skip");
        return MSERR_OK;
    }

    auto pauseTask = std::make_shared<TaskHandler<void>>([this]() {
        auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
        (void)currState->Pause();
    });

    int ret = taskMgr_.LaunchTask(pauseTask, PlayBinTaskType::STATE_CHANGE);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Pause failed");

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::Seek(int64_t timeUs, int32_t seekOption)
{
    MEDIA_LOGD("enter");

    if (SEEK_OPTION_TO_GST_SEEK_FLAGS.find(seekOption) == SEEK_OPTION_TO_GST_SEEK_FLAGS.end()) {
        MEDIA_LOGE("unsupported seek option: %{public}d", seekOption);
        return MSERR_INVALID_VAL;
    }

    if (timeUs < 0) {
        MEDIA_LOGE("negative seek position is invalid");
        return MSERR_INVALID_VAL;
    }

    std::unique_lock<std::mutex> lock(mutex_);

    auto seekTask = std::make_shared<TaskHandler<void>>([this, timeUs, seekOption]() {
        auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
        (void)currState->Seek(timeUs, seekOption);
    });

    int ret = taskMgr_.LaunchTask(seekTask, PlayBinTaskType::SEEKING);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Seek failed");

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::StopInternel()
{
    auto state = GetCurrState();
    if (state == idleState_ || state == stoppedState_ || state == initializedState_) {
        MEDIA_LOGI("curr state is %{public}s, skip", state->GetStateName().c_str());
        return MSERR_OK;
    }

    taskMgr_.ClearAllTask();

    auto stopTask = std::make_shared<TaskHandler<void>>([this]() {
        auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
        (void)currState->Stop();
    });

    int ret = taskMgr_.LaunchTask(stopTask, PlayBinTaskType::STATE_CHANGE);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Stop failed");

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::Stop()
{
    MEDIA_LOGD("enter");

    std::unique_lock<std::mutex> lock(mutex_);
    return StopInternel();
}

int64_t PlayBinCtrlerBase::GetDuration()
{
    std::unique_lock<std::mutex> lock(mutex_);
    QueryDuration();
    return duration_;
}

void PlayBinCtrlerBase::Reset() noexcept
{
    MEDIA_LOGD("enter");

    std::unique_lock<std::mutex> lock(mutex_);
    (void)StopInternel();

    auto idleTask = std::make_shared<TaskHandler<void>>([this]() { ChangeState(idleState_); });
    // fake state_change type, just wait the previous stop task's second phase finish.
    (void)taskMgr_.LaunchTask(idleTask, PlayBinTaskType::STATE_CHANGE);
    (void)idleTask->GetResult();

    (void)taskMgr_.Reset();

    if (msgQueue_ != nullptr) {
        (void)msgQueue_->Stop();
    }

    elemSetupListener_ = nullptr;
    uri_.clear();
    isErrorHappened_ = false;

    MEDIA_LOGD("exit");
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

    ON_SCOPE_EXIT(0) {
        ExitInitializedState();
        PlayBinMessage msg { PlayBinMsgType::PLAYBIN_MSG_ERROR, 0, MSERR_UNKNOWN };
        ReportMessage(msg);
        MEDIA_LOGE("enter intialized state failed");
    };

    int32_t ret = OnInit();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);
    CHECK_AND_RETURN_RET(playbin_ != nullptr, static_cast<int32_t>(MSERR_UNKNOWN));

    SetupCustomElement();
    ret = SetupSignalMessage();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    uint32_t flags = 0;
    g_object_get(playbin_, "flags", &flags, nullptr);
    if (renderMode_ & PlayBinRenderMode::NATIVE_STREAM) {
        flags |= GST_PLAY_FLAG_NATIVE_VIDEO | GST_PLAY_FLAG_NATIVE_AUDIO;
        flags &= ~(GST_PLAY_FLAG_SOFT_COLORBALANCE | GST_PLAY_FLAG_SOFT_VOLUME);
    }
    if (renderMode_ & PlayBinCtrlerBase::DISABLE_TEXT) {
        flags &= ~GST_PLAY_FLAG_TEXT;
    }
    g_object_set(playbin_, "flags", flags, nullptr);

    // There may be a risk of data competition, but the uri is unlikely to be reconfigured.
    g_object_set(playbin_, "uri", uri_.c_str(), nullptr);

    isInitialized = true;
    ChangeState(initializedState_);

    CANCEL_SCOPE_EXIT_GUARD(0);
    MEDIA_LOGD("EnterInitializedState exit");
    return MSERR_OK;
}

void PlayBinCtrlerBase::ExitInitializedState()
{
    MEDIA_LOGD("ExitInitializedState enter");

    isInitialized = false;

    if (msgProcessor_ != nullptr) {
        msgProcessor_->Reset();
        msgProcessor_ = nullptr;
    }

    if (signalId_ != 0) {
        g_signal_handler_disconnect(playbin_, signalId_);
        signalId_ = 0;
    }

    MEDIA_LOGD("unref playbin start");
    if (playbin_ != nullptr) {
        gst_object_unref(playbin_);
        playbin_ = nullptr;
    }
    MEDIA_LOGD("unref playbin stop");

    MEDIA_LOGD("ExitInitializedState exit");
}

int32_t PlayBinCtrlerBase::PrepareAsyncInternel()
{
    if ((GetCurrState() == preparingState_) || (GetCurrState() == preparedState_)) {
        MEDIA_LOGI("already at preparing state, skip");
        return MSERR_OK;
    }

    CHECK_AND_RETURN_RET_LOG(!uri_.empty(), MSERR_INVALID_OPERATION, "Set uri firsty!");

    auto preparedTask = std::make_shared<TaskHandler<int32_t>>([this]() {
        int32_t ret = EnterInitializedState();
        CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

        auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
        return currState->Prepare();
    });

    int32_t ret = taskMgr_.LaunchTask(preparedTask, PlayBinTaskType::STATE_CHANGE);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "PrepareAsync failed");

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::SeekInternel(int64_t timeUs, int32_t seekOption)
{
    MEDIA_LOGD("execute seek, time: %{public}" PRIi64 ", option: %{public}d", timeUs, seekOption);

    int32_t seekFlags = SEEK_OPTION_TO_GST_SEEK_FLAGS.at(seekOption);
    timeUs = timeUs > duration_ ? duration_ : timeUs;

    constexpr int32_t usecToNanoSec = 1000;
    int64_t timeNs = timeUs * usecToNanoSec;

    GstEvent *event = gst_event_new_seek(1.0, GST_FORMAT_TIME, static_cast<GstSeekFlags>(seekFlags),
        GST_SEEK_TYPE_SET, timeNs, GST_SEEK_TYPE_SET, GST_CLOCK_TIME_NONE);
    CHECK_AND_RETURN_RET_LOG(event != nullptr, MSERR_NO_MEMORY, "seek failed");

    gboolean ret = gst_element_send_event(GST_ELEMENT_CAST(playbin_), event);
    CHECK_AND_RETURN_RET_LOG(ret, MSERR_SEEK_FAILED, "seek failed");

    return MSERR_OK;
}

void PlayBinCtrlerBase::SetupCustomElement()
{
    // There may be a risk of data competition, but the sinkProvider is unlikely to be reconfigured.
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

    if ((renderMode_ & PlayBinRenderMode::NATIVE_STREAM) == 0) {
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

    signalId_ = g_signal_connect_data(playbin_, "element-setup", G_CALLBACK(&PlayBinCtrlerBase::ElementSetup),
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

void PlayBinCtrlerBase::QueryDuration()
{
    auto state = GetCurrState();
    if (state != preparedState_ && state != playingState_ && state != pausedState_) {
        MEDIA_LOGD("reuse the last query result: %{public}" PRIi64 " microsecond", duration_);
        return;
    }

    gint64 duration = 0;
    gboolean ret = gst_element_query_duration(GST_ELEMENT_CAST(playbin_), GST_FORMAT_TIME, &duration);
    CHECK_AND_RETURN_LOG(ret, "query duration failed");

    static const int32_t nanoSecPerUSec = 1000;
    duration_ = duration / nanoSecPerUSec;
    MEDIA_LOGI("update the duration: %{public}" PRIi64 " microsecond", duration_);
}

void PlayBinCtrlerBase::OnElementSetup(GstElement &elem)
{
    MEDIA_LOGD("element setup: %{public}s", ELEM_NAME(&elem));

    // limit to the g-signal, send this notification at this thread, do not change the work thread.
    // otherwise ,the avmetaengine will work inproperly.

    decltype(elemSetupListener_) listener = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        listener = elemSetupListener_;
    }

    if (listener != nullptr) {
        listener(elem);
    }
}

void PlayBinCtrlerBase::OnMessageReceived(const InnerMessage &msg)
{
    auto msgHandler = std::make_shared<TaskHandler<void>>([this, msg]() { HandleMessage(msg); });
    int32_t ret = taskMgr_.LaunchTask(msgHandler, PlayBinTaskType::PREEMPT);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("sync process msg failed, type: %{public}d, detail1: %{public}d, detail2: %{public}d",
                   msg.type, msg.detail1, msg.detail2);
    };
}

void PlayBinCtrlerBase::ReportMessage(const PlayBinMessage &msg)
{
    if (msg.type == PlayBinMsgType::PLAYBIN_MSG_ERROR) {
        MEDIA_LOGE("error happend, error code: %{public}d", msg.code);

        std::unique_lock<std::mutex> condLock(condMutex_);
        isErrorHappened_ = true;
        stateCond_.notify_all();
    }

    MEDIA_LOGD("report msg, type: %{public}d", msg.type);

    auto msgReportHandler = std::make_shared<TaskHandler<void>>([this, msg]() { notifier_(msg); });
    int32_t ret = msgQueue_->EnqueueTask(msgReportHandler);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("async report msg failed, type: %{public}d, subType: %{public}d, code: %{public}d",
                   msg.type, msg.subType, msg.code);
    };
}
}
}