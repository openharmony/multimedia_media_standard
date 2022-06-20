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
    constexpr uint64_t RING_BUFFER_MAX_SIZE = 5242880; // 5 * 1024 * 1024
    constexpr int32_t PLAYBIN_QUEUE_MAX_SIZE = 100 * 1024 * 1024; // 100 * 1024 * 1024 Bytes
    constexpr uint64_t BUFFER_DURATION = 15000000000; // 15s
    constexpr int32_t BUFFER_LOW_PERCENT_DEFAULT = 1;
    constexpr int32_t BUFFER_HIGH_PERCENT_DEFAULT = 4;
    constexpr uint32_t HTTP_TIME_OUT_DEFAULT = 15; // 15s
    constexpr int32_t NANO_SEC_PER_USEC = 1000;
    constexpr double DEFAULT_RATE = 1.0;
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

void PlayBinCtrlerBase::SourceSetup(const GstElement *playbin, GstElement *elem, gpointer userdata)
{
    if (elem == nullptr || userdata == nullptr) {
        return;
    }

    auto thizStrong = PlayBinCtrlerWrapper::TakeStrongThiz(userdata);
    if (thizStrong != nullptr) {
        return thizStrong->OnSourceSetup(playbin, elem, thizStrong);
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
    sinkProvider_ = nullptr;
    notifier_ = nullptr;
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
    playbackCompletedState_ = std::make_shared<PlaybackCompletedState>(*this);

    rate_ = DEFAULT_RATE;

    ChangeState(idleState_);

    msgQueue_ = std::make_unique<TaskQueue>("playbin-ctrl-msg");
    int32_t ret = msgQueue_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "msgqueue start failed");

    return ret;
}

bool PlayBinCtrlerBase::IsLiveSource()
{
    if (appsrcWrap_ != nullptr && appsrcWrap_->IsLiveMode()) {
        return true;
    }
    return false;
}

int32_t PlayBinCtrlerBase::SetSource(const std::string &url)
{
    std::unique_lock<std::mutex> lock(mutex_);
    uri_ = url;

    MEDIA_LOGI("Set source: %{public}s", url.c_str());
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::SetSource(const std::shared_ptr<GstAppsrcWrap> &appsrcWrap)
{
    std::unique_lock<std::mutex> lock(mutex_);
    appsrcWrap_ = appsrcWrap;
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::Prepare()
{
    MEDIA_LOGD("enter");

    std::unique_lock<std::mutex> lock(mutex_);

    int32_t ret = PrepareAsyncInternal();
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
    return PrepareAsyncInternal();
}

int32_t PlayBinCtrlerBase::Play()
{
    MEDIA_LOGD("enter");

    std::unique_lock<std::mutex> lock(mutex_);

    if (GetCurrState() == playingState_) {
        MEDIA_LOGI("already at playing state, skip");
        return MSERR_OK;
    }

    auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
    int32_t ret = currState->Play();
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

    auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
    int32_t ret = currState->Pause();
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

    std::unique_lock<std::mutex> lock(mutex_);

    auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
    int32_t ret = currState->Seek(timeUs, seekOption);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Seek failed");

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::StopInternal()
{
    if (appsrcWrap_ != nullptr) {
        appsrcWrap_->Stop();
    }

    auto state = GetCurrState();
    if (state == idleState_ || state == stoppedState_ || state == initializedState_ || state == preparingState_) {
        MEDIA_LOGI("curr state is %{public}s, skip", state->GetStateName().c_str());
        return MSERR_OK;
    }

    g_object_set(playbin_, "exit-block", 1, nullptr);
    auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
    int32_t ret = currState->Stop();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Stop failed");

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::Stop()
{
    MEDIA_LOGD("enter");

    std::unique_lock<std::mutex> lock(mutex_);
    return StopInternal();
}

int64_t PlayBinCtrlerBase::GetDuration()
{
    std::unique_lock<std::mutex> lock(mutex_);
    QueryDuration();
    return duration_;
}

int64_t PlayBinCtrlerBase::GetPosition()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return QueryPosition();
}

GstSeekFlags PlayBinCtrlerBase::ChooseSetRateFlags(double rate)
{
    GstSeekFlags seekFlags;

    if (rate > 0.0) {
        seekFlags = static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH |
            GST_SEEK_FLAG_TRICKMODE | GST_SEEK_FLAG_SNAP_AFTER);
    } else {
        seekFlags = static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH |
            GST_SEEK_FLAG_TRICKMODE | GST_SEEK_FLAG_SNAP_BEFORE);
    }

    return seekFlags;
}

int32_t PlayBinCtrlerBase::SetRateInternal(double rate)
{
    MEDIA_LOGD("execute set rate, rate: %{public}lf", rate);

    gint64 position = 0;
    gboolean ret = gst_element_query_position(GST_ELEMENT_CAST(playbin_), GST_FORMAT_TIME, &position);
    CHECK_AND_RETURN_RET_LOG(ret, MSERR_NO_MEMORY, "query position failed");

    GstSeekFlags flags = ChooseSetRateFlags(rate);
    int64_t start = rate > 0 ? position : 0;
    int64_t stop = rate > 0 ? static_cast<int64_t>(GST_CLOCK_TIME_NONE) : position;

    isRating_ = true;
    GstEvent *event = gst_event_new_seek(rate, GST_FORMAT_TIME, flags,
        GST_SEEK_TYPE_SET, start, GST_SEEK_TYPE_SET, stop);
    CHECK_AND_RETURN_RET_LOG(event != nullptr, MSERR_NO_MEMORY, "set rate failed");

    ret = gst_element_send_event(GST_ELEMENT_CAST(playbin_), event);
    CHECK_AND_RETURN_RET_LOG(ret, MSERR_SEEK_FAILED, "set rate failed");

    rate_ = rate;

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::SetRate(double rate)
{
    MEDIA_LOGD("enter");
    std::unique_lock<std::mutex> lock(mutex_);

    if (IsLiveSource()) {
        return MSERR_INVALID_OPERATION;
    }

    auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
    int32_t ret = currState->SetRate(rate);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "SetRate failed");

    return MSERR_OK;
}

double PlayBinCtrlerBase::GetRate()
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGI("get rate=%{public}lf", rate_);
    return rate_;
}

int32_t PlayBinCtrlerBase::SetLoop(bool loop)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (IsLiveSource()) {
        return MSERR_INVALID_OPERATION;
    }
    enableLooping_ = loop;
    return MSERR_OK;
}

void PlayBinCtrlerBase::SetVolume(const float &leftVolume, const float &rightVolume)
{
    (void)rightVolume;
    std::unique_lock<std::mutex> lock(mutex_);
    float volume = leftVolume;
    if (audioSink_ != nullptr) {
        MEDIA_LOGI("SetVolume(%{public}f) to audio sink", volume);
        g_object_set(audioSink_, "volume", volume, nullptr);
    }
}

void PlayBinCtrlerBase::SetAudioRendererInfo(int32_t rendererInfo)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (audioSink_ != nullptr) {
        MEDIA_LOGI("SetAudioRendererInfo(%{public}d) to audio sink", rendererInfo);
        g_object_set(audioSink_, "audio-renderer-desc", rendererInfo, nullptr);
    }
}

int32_t PlayBinCtrlerBase::SelectBitRate(uint32_t bitRate)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (bitRateVec_.empty()) {
        MEDIA_LOGE("BitRate is empty");
        return MSERR_INVALID_OPERATION;
    }

    g_object_set(playbin_, "bitrate-parse-complete", static_cast<uint64_t>(bitRate), nullptr);

    PlayBinMessage msg = { PLAYBIN_MSG_BITRATEDONE, 0, static_cast<int32_t>(bitRate) };
    ReportMessage(msg);

    return MSERR_OK;
}

void PlayBinCtrlerBase::Reset() noexcept
{
    MEDIA_LOGD("enter");

    std::unique_lock<std::mutex> lock(mutex_);
    {
        std::unique_lock<std::mutex> lk(listenerMutex_);
        elemSetupListener_ = nullptr;
    }
    (void)StopInternal();

    ChangeState(idleState_);

    if (msgQueue_ != nullptr) {
        (void)msgQueue_->Stop();
    }

    uri_.clear();
    isErrorHappened_ = false;
    enableLooping_ = false;
    {
        std::unique_lock<std::mutex> appsrcLock(appsrcMutex_);
        appsrcWrap_ = nullptr;
    }

    rate_ = DEFAULT_RATE;
    seekPos_ = 0;
    lastTime_ = 0;
    isSeeking_ = false;
    isRating_ = false;
    isBuffering_ = false;

    MEDIA_LOGD("exit");
}

void PlayBinCtrlerBase::SetElemSetupListener(ElemSetupListener listener)
{
    std::unique_lock<std::mutex> lock(mutex_);
    std::unique_lock<std::mutex> lk(listenerMutex_);
    elemSetupListener_ = listener;
}

void PlayBinCtrlerBase::DoInitializeForHttp()
{
    g_object_set(playbin_, "ring-buffer-max-size", RING_BUFFER_MAX_SIZE, nullptr);
    g_object_set(playbin_, "buffering-flags", true, "buffer-size", PLAYBIN_QUEUE_MAX_SIZE,
        "buffer-duration", BUFFER_DURATION, "low-percent", BUFFER_LOW_PERCENT_DEFAULT,
        "high-percent", BUFFER_HIGH_PERCENT_DEFAULT, nullptr);
    g_object_set(playbin_, "timeout", HTTP_TIME_OUT_DEFAULT, nullptr);

    PlayBinCtrlerWrapper *wrapper = new(std::nothrow) PlayBinCtrlerWrapper(shared_from_this());
    CHECK_AND_RETURN_LOG(wrapper != nullptr, "can not create this wrapper");

    (void)signalIds_.emplace(GST_ELEMENT_CAST(playbin_), g_signal_connect_data(playbin_, "bitrate-parse-complete",
        G_CALLBACK(&PlayBinCtrlerBase::OnBitRateParseCompleteCb), wrapper,
        (GClosureNotify)&PlayBinCtrlerWrapper::OnDestory, static_cast<GConnectFlags>(0)));
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
        MEDIA_LOGE("enter initialized state failed");
    };

    int32_t ret = OnInit();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);
    CHECK_AND_RETURN_RET(playbin_ != nullptr, static_cast<int32_t>(MSERR_UNKNOWN));

    ret = DoInitializeForDataSource();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "DoInitializeForDataSource failed!");

    SetupCustomElement();
    ret = SetupSignalMessage();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    DoInitializeForHttp();

    uint32_t flags = 0;
    g_object_get(playbin_, "flags", &flags, nullptr);
    if (renderMode_ & PlayBinRenderMode::NATIVE_STREAM) {
        flags |= GST_PLAY_FLAG_NATIVE_VIDEO | GST_PLAY_FLAG_NATIVE_AUDIO;
        flags &= ~(GST_PLAY_FLAG_SOFT_COLORBALANCE | GST_PLAY_FLAG_SOFT_VOLUME);
    }
    if (renderMode_ & PlayBinRenderMode::DISABLE_TEXT) {
        flags &= ~GST_PLAY_FLAG_TEXT;
    }
    if (renderMode_ & PlayBinRenderMode::DISABLE_VIS) {
        flags &= ~GST_PLAY_FLAG_VIS;
    }
    g_object_set(playbin_, "flags", flags, nullptr);

    // There may be a risk of data competition, but the uri is unlikely to be reconfigured.
    if (!uri_.empty()) {
        g_object_set(playbin_, "uri", uri_.c_str(), nullptr);
    }

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

    for (auto &[elem, signalId] : signalIds_) {
        g_signal_handler_disconnect(elem, signalId);
    }
    signalIds_.clear();

    if (audioSink_ != nullptr) {
        gst_object_unref(audioSink_);
        audioSink_ = nullptr;
    }
    if (videoSink_ != nullptr) {
        gst_object_unref(videoSink_);
        videoSink_ = nullptr;
    }

    MEDIA_LOGD("unref playbin start");
    if (playbin_ != nullptr) {
        (void)gst_element_set_state(GST_ELEMENT_CAST(playbin_), GST_STATE_NULL);
        gst_object_unref(playbin_);
        playbin_ = nullptr;
    }
    MEDIA_LOGD("unref playbin stop");

    MEDIA_LOGD("ExitInitializedState exit");
}

int32_t PlayBinCtrlerBase::PrepareAsyncInternal()
{
    if ((GetCurrState() == preparingState_) || (GetCurrState() == preparedState_)) {
        MEDIA_LOGI("already at preparing state, skip");
        return MSERR_OK;
    }

    CHECK_AND_RETURN_RET_LOG((!uri_.empty() || appsrcWrap_), MSERR_INVALID_OPERATION, "Set uri firsty!");

    int32_t ret = EnterInitializedState();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
    ret = currState->Prepare();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "PrepareAsyncInternal failed");

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::SeekInternal(int64_t timeUs, int32_t seekOption)
{
    MEDIA_LOGD("execute seek, time: %{public}" PRIi64 ", option: %{public}d", timeUs, seekOption);

    int32_t seekFlags = SEEK_OPTION_TO_GST_SEEK_FLAGS.at(seekOption);
    timeUs = timeUs > duration_ ? duration_ : timeUs;
    timeUs = timeUs < 0 ? 0 : timeUs;

    constexpr int32_t usecToNanoSec = 1000;
    int64_t timeNs = timeUs * usecToNanoSec;
    seekPos_ = timeUs;

    isSeeking_ = true;
    GstEvent *event = gst_event_new_seek(1.0, GST_FORMAT_TIME, static_cast<GstSeekFlags>(seekFlags),
        GST_SEEK_TYPE_SET, timeNs, GST_SEEK_TYPE_SET, GST_CLOCK_TIME_NONE);
    CHECK_AND_RETURN_RET_LOG(event != nullptr, MSERR_NO_MEMORY, "seek failed");

    gboolean ret = gst_element_send_event(GST_ELEMENT_CAST(playbin_), event);
    CHECK_AND_RETURN_RET_LOG(ret, MSERR_SEEK_FAILED, "seek failed");

    return MSERR_OK;
}

void PlayBinCtrlerBase::SetupVolumeChangedCb()
{
    PlayBinCtrlerWrapper *wrapper = new(std::nothrow) PlayBinCtrlerWrapper(shared_from_this());
    CHECK_AND_RETURN_LOG(wrapper != nullptr, "can not create this wrapper");
    (void)signalIds_.emplace(audioSink_, g_signal_connect_data(audioSink_, "notify::volume",
        G_CALLBACK(&PlayBinCtrlerBase::OnVolumeChangedCb), wrapper,
        (GClosureNotify)&PlayBinCtrlerWrapper::OnDestory, static_cast<GConnectFlags>(0)));
}

void PlayBinCtrlerBase::SetupCustomElement()
{
    // There may be a risk of data competition, but the sinkProvider is unlikely to be reconfigured.
    if (sinkProvider_ != nullptr) {
        audioSink_ = sinkProvider_->CreateAudioSink();
        if (audioSink_ != nullptr) {
            g_object_set(playbin_, "audio-sink", audioSink_, nullptr);
        }
        videoSink_ = sinkProvider_->CreateVideoSink();
        if (videoSink_ != nullptr) {
            g_object_set(playbin_, "video-sink", videoSink_, nullptr);
        } else if (audioSink_ != nullptr) {
            g_object_set(playbin_, "video-sink", audioSink_, nullptr);
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

    (void)signalIds_.emplace(GST_ELEMENT_CAST(playbin_), g_signal_connect_data(playbin_, "element-setup",
        G_CALLBACK(&PlayBinCtrlerBase::ElementSetup), wrapper, (GClosureNotify)&PlayBinCtrlerWrapper::OnDestory,
        static_cast<GConnectFlags>(0)));

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
    if (state != preparedState_ && state != playingState_ && state != pausedState_ &&
        state != playbackCompletedState_) {
        MEDIA_LOGD("reuse the last query result: %{public}" PRIi64 " microsecond", duration_);
        return;
    }

    gint64 duration = 0;
    gboolean ret = gst_element_query_duration(GST_ELEMENT_CAST(playbin_), GST_FORMAT_TIME, &duration);
    CHECK_AND_RETURN_LOG(ret, "query duration failed");

    duration_ = duration / NANO_SEC_PER_USEC;
    MEDIA_LOGI("update the duration: %{public}" PRIi64 " microsecond", duration_);
}

int64_t PlayBinCtrlerBase::QueryPosition()
{
    auto state = GetCurrState();
    if (state == playbackCompletedState_) {
        if (IsLiveSource()) {
            return lastTime_;
        }
        return duration_;
    }

    if (state != preparedState_ && state != playingState_ && state != pausedState_) {
        MEDIA_LOGD("get position at state: %{public}s, return 0", state->GetStateName().c_str());
        return 0;
    }

    if (isSeeking_ || isRating_) {
        MEDIA_LOGI("in seeking or speeding, reuse the last postion: %{public}" PRIi64 "", lastTime_);
        return lastTime_;
    }

    return QueryPositionInternal(false);
}

int64_t PlayBinCtrlerBase::QueryPositionInternal(bool isSeekDone)
{
    gint64 position = 0;
    gboolean ret = gst_element_query_position(GST_ELEMENT_CAST(playbin_), GST_FORMAT_TIME, &position);
    if (!ret) {
        if (isSeekDone) {
            position = seekPos_ * NANO_SEC_PER_USEC;
        } else {
            MEDIA_LOGE("query position failed");
            return lastTime_;
        }
    }

    int64_t curTime = position / NANO_SEC_PER_USEC;
    curTime = std::min(curTime, duration_);
    lastTime_ = curTime;
    MEDIA_LOGI("update the position: %{public}" PRIi64 " microsecond", curTime);
    return curTime;
}

void PlayBinCtrlerBase::ProcessEndOfStream()
{
    MEDIA_LOGD("End of stream");
    std::unique_lock<std::mutex> lock(mutex_);
    if (IsLiveSource()) {
        MEDIA_LOGD("appsrc livemode, can not loop");
        return;
    }

    if (enableLooping_) {
        (void)SeekInternal(0, IPlayBinCtrler::PlayBinSeekMode::PREV_SYNC);
    } else {
        ChangeState(playbackCompletedState_);
    }
}

int32_t PlayBinCtrlerBase::DoInitializeForDataSource()
{
    if (appsrcWrap_ != nullptr) {
        (void)appsrcWrap_->Prepare();
        auto msgNotifier = std::bind(&PlayBinCtrlerBase::OnAppsrcErrorMessageReceived, this, std::placeholders::_1);
        CHECK_AND_RETURN_RET_LOG(appsrcWrap_->SetErrorCallback(msgNotifier) == MSERR_OK,
            MSERR_INVALID_OPERATION, "set appsrc error callback failed");

        PlayBinCtrlerWrapper *wrapper = new(std::nothrow) PlayBinCtrlerWrapper(shared_from_this());
        CHECK_AND_RETURN_RET_LOG(wrapper != nullptr, MSERR_NO_MEMORY, "can not create this wrapper");

        (void)signalIds_.emplace(GST_ELEMENT_CAST(playbin_), g_signal_connect_data(playbin_, "source-setup",
            G_CALLBACK(&PlayBinCtrlerBase::SourceSetup), wrapper, (GClosureNotify)&PlayBinCtrlerWrapper::OnDestory,
            static_cast<GConnectFlags>(0)));

        g_object_set(playbin_, "uri", "appsrc://", nullptr);
    }
    return MSERR_OK;
}

void PlayBinCtrlerBase::HandleCacheCtrl(const InnerMessage &msg)
{
    PlayBinMessage playBinMsg = { PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_BUFFERING_PERCENT, msg.detail1, {} };
    ReportMessage(playBinMsg);

    int32_t percent = msg.detail1;
    MEDIA_LOGI("HandleCacheCtrl percent is %{public}d", percent);
    if (!isBuffering_) {
        HandleCacheCtrlWhenNoBuffering(percent);
    } else if (isBuffering_) {
        HandleCacheCtrlWhenBuffering(percent);
    }
}

void PlayBinCtrlerBase::HandleCacheCtrlWhenNoBuffering(int32_t percent)
{
    if (percent < BUFFER_LOW_PERCENT_DEFAULT) {
        isBuffering_ = true;
        g_object_set(playbin_, "state-change", GST_PLAYER_STATUS_BUFFERING, nullptr);
        if (GetCurrState() == playingState_) {
            GstStateChangeReturn ret = gst_element_set_state(GST_ELEMENT_CAST(playbin_), GST_STATE_PAUSED);
            if (ret == GST_STATE_CHANGE_FAILURE) {
                MEDIA_LOGE("Failed to change playbin's state to GST_STATE_PAUSED");
                return;
            }
        }

        PlayBinMessage msg = { PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_BUFFERING_START, 0, {} };
        ReportMessage(msg);
    }
}

void PlayBinCtrlerBase::HandleCacheCtrlWhenBuffering(int32_t percent)
{
    if (percent > BUFFER_HIGH_PERCENT_DEFAULT) {
        isBuffering_ = false;
        if (GetCurrState() == playingState_) {
            g_object_set(playbin_, "state-change", GST_PLAYER_STATUS_PLAYING, nullptr);
            GstStateChangeReturn ret = gst_element_set_state(GST_ELEMENT_CAST(playbin_), GST_STATE_PLAYING);
            if (ret == GST_STATE_CHANGE_FAILURE) {
                MEDIA_LOGE("Failed to change playbin's state to GST_STATE_PLAYING");
                return;
            }
        } else {
            g_object_set(playbin_, "state-change", GST_PLAYER_STATUS_PAUSED, nullptr);
        }

        PlayBinMessage msg = { PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_BUFFERING_END, 0, {} };
        ReportMessage(msg);
    }
}

void PlayBinCtrlerBase::OnSourceSetup(const GstElement *playbin, GstElement *src,
    const std::shared_ptr<PlayBinCtrlerBase> &playbinCtrl)
{
    (void)playbin;
    CHECK_AND_RETURN_LOG(playbinCtrl != nullptr, "playbinCtrl is null");
    CHECK_AND_RETURN_LOG(src != nullptr, "src is null");

    GstElementFactory *elementFac = gst_element_get_factory(src);
    const gchar *eleTypeName = g_type_name(gst_element_factory_get_element_type(elementFac));

    std::unique_lock<std::mutex> appsrcLock(appsrcMutex_);
    if ((eleTypeName != nullptr) && (strstr(eleTypeName, "GstAppSrc") != nullptr) &&
        (playbinCtrl->appsrcWrap_ != nullptr)) {
        (void)playbinCtrl->appsrcWrap_->SetAppsrc(src);
    }
}

bool PlayBinCtrlerBase::OnVideoDecoderSetup(GstElement &elem)
{
    const gchar *metadata = gst_element_get_metadata(&elem, GST_ELEMENT_METADATA_KLASS);
    if (metadata == nullptr) {
        MEDIA_LOGE("gst_element_get_metadata return nullptr");
        return false;
    }

    std::string metaStr(metadata);
    if (metaStr.find("Decoder/Video") != std::string::npos) {
        return true;
    }

    return false;
}

void PlayBinCtrlerBase::OnElementSetup(GstElement &elem)
{
    MEDIA_LOGD("element setup: %{public}s", ELEM_NAME(&elem));

    // limit to the g-signal, send this notification at this thread, do not change the work thread.
    // otherwise ,the avmetaengine will work improperly.

    if (OnVideoDecoderSetup(elem) || strncmp(ELEM_NAME(&elem), "multiqueue", strlen("multiqueue")) == 0) {
        MEDIA_LOGD("add msgfilter element: %{public}s", ELEM_NAME(&elem));
        msgProcessor_->AddMsgFilter(ELEM_NAME(&elem));
    }

    decltype(elemSetupListener_) listener = nullptr;
    {
        std::unique_lock<std::mutex> lock(listenerMutex_);
        listener = elemSetupListener_;
    }

    if (listener != nullptr) {
        listener(elem);
    }
}

void PlayBinCtrlerBase::OnVolumeChangedCb(const GstElement *playbin, GstElement *elem, gpointer userdata)
{
    (void)playbin;
    if (elem == nullptr || userdata == nullptr) {
        return;
    }

    auto thizStrong = PlayBinCtrlerWrapper::TakeStrongThiz(userdata);
    if (thizStrong != nullptr) {
        PlayBinMessage msg { PlayBinMsgType::PLAYBIN_MSG_VOLUME_CHANGE, 0, 0 };
        thizStrong->ReportMessage(msg);
    }
}

void PlayBinCtrlerBase::OnBitRateParseCompleteCb(const GstElement *playbin, uint32_t *bitrateInfo,
    uint32_t bitrateNum, gpointer userdata)
{
    (void)playbin;
    auto thizStrong = PlayBinCtrlerWrapper::TakeStrongThiz(userdata);
    if (thizStrong != nullptr) {
        MEDIA_LOGD("bitrateNum = %{public}u", bitrateNum);
        for (uint32_t i = 0; i < bitrateNum; i++) {
            MEDIA_LOGD("bitrate = %{public}u", bitrateInfo[i]);
            thizStrong->bitRateVec_.push_back(bitrateInfo[i]);
        }
        std::pair<uint32_t *, uint32_t> bitRatePair;
        bitRatePair.first = bitrateInfo;
        bitRatePair.second = bitrateNum;
        PlayBinMessage msg = { PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_BITRATE_COLLECT, 0, bitRatePair };
        thizStrong->ReportMessage(msg);
    }
}

void PlayBinCtrlerBase::OnAppsrcErrorMessageReceived(int32_t errorCode)
{
    PlayBinMessage msg { PlayBinMsgType::PLAYBIN_MSG_ERROR, 0, errorCode };
    ReportMessage(msg);
}

void PlayBinCtrlerBase::OnMessageReceived(const InnerMessage &msg)
{
    HandleMessage(msg);
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

    if (msg.type == PlayBinMsgType::PLAYBIN_MSG_EOS) {
        ProcessEndOfStream();
    }
}
} // namespace Media
} // namespace OHOS