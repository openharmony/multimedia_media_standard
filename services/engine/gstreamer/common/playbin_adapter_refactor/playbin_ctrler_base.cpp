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
#include "gst_utils.h"
#include "media_log.h"
#include "media_errors.h"
#include "playbin_async_action.h"
#include "scope_guard.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayBinCtrler"};
}

namespace OHOS {
namespace Media {
namespace PlayBin {
const std::string_view StringifyPlayBinState(PlayBinState state)
{
    static const std::unordered_map<PlayBinState, std::string_view> table = {
        { PLAYBIN_STATE_ERROR, "PLAYBIN_STATE_ERROR" },
        { PLAYBIN_STATE_IDLE, "PLAYBIN_STATE_IDLE" },
        { PLAYBIN_STATE_PREPARING, "PLAYBIN_STATE_PREPARING" },
        { PLAYBIN_STATE_PREPARED, "PLAYBIN_STATE_PREPARED" },
        { PLAYBIN_STATE_PLAYING, "PLAYBIN_STATE_PLAYING" },
        { PLAYBIN_STATE_PAUSED, "PLAYBIN_STATE_PAUSED" },
        { PLAYBIN_STATE_STOPPED, "PLAYBIN_STATE_STOPPED" },
    };

    static const std::string_view dummy = "PLAYBIN_STATE_UNKNOWN";

    auto iter = table.find(state);
    if (iter != table.end()) {
        return iter->second;
    }

    return dummy;
}

std::shared_ptr<IPlayBinCtrler> IPlayBinCtrler::Create(const CreateParam &createParam)
{
    auto inst = std::make_shared<PlayBinCtrlerBase>(createParam);
    int32_t ret = inst->Init();
    if (ret != MSERR_OK) {
        MEDIA_LOGE("Failed to create PlaybinCtrlerBase");
        return nullptr;
    }

    return inst;
}

class PlayBinCtrlerBase::PlayBinStateOperator : public StateOperator {
public:
    PlayBinStateOperator(PlayBinCtrlerBase &ctrler) : ctrler_(ctrler) {}
    ~PlayBinStateOperator() = default;

    PlayBinState GetPlayBinState() const override
    {
        return ctrler_.selfState_;
    }

    int32_t SetPlayBinState(PlayBinState targetState) override
    {
        ctrler_.UpdateStateAndReport(targetState);
    }

    GstState GetGstState() const override
    {
        GstState state = GST_STATE_VOID_PENDING;
        GstState pending = GST_STATE_VOID_PENDING;
        auto ret = gst_element_get_state(ctrler_.playbin_, &state, &pending, static_cast<GstClockTime>(0));
        if (ret != GST_STATE_CHANGE_SUCCESS) {
            MEDIA_LOGE("Failed to get playbin gst state");
            return GST_STATE_VOID_PENDING;
        }
        return state;
    }

    int32_t SetGstState(GstState targetState) override
    {
        MEDIA_LOGI("Begin to set playbin gst state to %{public}s", gst_element_state_get_name(targetState));

        auto ret = gst_element_set_state(ctrler_.playbin_, targetState);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            MEDIA_LOGE("Failed to set playbin gst state to %{public}s", gst_element_state_get_name(targetState));
            return MSERR_INVALID_OPERATION;
        }

        return MSERR_OK;
    }

private:
    PlayBinCtrlerBase &ctrler_;
};

class PlayBinCtrlerBase::PlayBinEventMsgSender : public EventMsgSender {
public:
    PlayBinEventMsgSender(PlayBinCtrlerBase &ctrler) : ctrler_(ctrler) {}
    ~PlayBinEventMsgSender() = default;

    int32_t SendSeekEvent(int64_t position, IPlayBinCtrler::SeekMode mode) override
    {
    }

    int32_t SendSpeedEvent(double rate) override
    {

    }

    void SendMessage(const InnerMessage &msg) override
    {

    }

private:
    PlayBinCtrlerBase &ctrler_;
};

PlayBinCtrlerBase::PlayBinCtrlerBase(const CreateParam &createParam)
    : renderMode_(createParam.notifier),
      notifier_(createParam.notifier),
      sinkProvider_(createParam.sinkProvider),
      msgConverter_(createParam.msgConverter),
      workLooper_(createParam.workLooper)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayBinCtrlerBase::~PlayBinCtrlerBase()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t PlayBinCtrlerBase::Init()
{
    if (workLooper_ == nullptr) {
        MEDIA_LOGE("Failed to init without work looper");
        return MSERR_INVALID_VAL;
    }

    playbin_ = reinterpret_cast<GstPipeline *>(gst_element_factory_make("playbin", "playbin"));
    CHECK_AND_RETURN_RET_LOG(playbin_ != nullptr, MSERR_INVALID_OPERATION, "Failed to create gstplaybin");

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::SetSource(const std::shared_ptr<SourceBase> &source)
{
    CHECK_AND_RETURN_RET_LOG(source != nullptr, MSERR_INVALID_VAL, "Invalid source");

    source_ = source;
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::Prepare()
{
    auto prepareAction = std::make_shared<TaskHandler>([this]() {
        ON_SCOPE_EXIT(0) { DoUnSetup(); };

        int32_t ret = DoSetup();
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to setup");

        ret = source_->Start();
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to start source");

        GstStateChangeReturn stateRet = gst_element_set_state(playbin_, GST_STATE_PAUSED);
        if (stateRet == GST_STATE_CHANGE_FAILURE) {
            MEDIA_LOGE("Failed to set state to %{public}s", gst_element_state_get_name(GST_STATE_PAUSED));
            DoUnSetup();
            return MSERR_INVALID_OPERATION;
        }

        CANCEL_SCOPE_EXIT_GUARD(0);
        UpdateStateAndReport(PLAYBIN_STATE_PREPARING);
    });

    int32_t ret = workLooper_.EnqueueTask(prepareAction);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to enqueue prepareAction");

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::Play()
{
    if (selfState_ != PLAYBIN_STATE_PREPARED && selfState_ != PLAYBIN_STATE_PAUSED) {
        return MSERR_INVALID_STATE;
    }

    auto playAction = [this]() {
        GstStateChangeReturn stateRet = gst_element_set_state(playbin_, GST_STATE_PLAYING);
        if (stateRet == GST_STATE_CHANGE_FAILURE) {
            MEDIA_LOGE("Failed to set state to %{public}s", gst_element_state_get_name(GST_STATE_PAUSED));
            return MSERR_INVALID_OPERATION;
        }
    };

    int32_t ret = workLooper_.EnqueueTask(playAction);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to enqueue playAction");

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::Pause()
{

}

int32_t PlayBinCtrlerBase::DoSetup()
{
    CHECK_AND_RETURN_RET_LOG(source_ != nullptr, MSERR_INVALID_OPERATION, "Invalid source");
    if (selfState_ != PLAYBIN_STATE_IDLE) {
        MEDIA_LOGE("Invalid state: %{public}s", StringifyPlayBinState(selfState_).data());
        return MSERR_INVALID_STATE;
    }

    int32_t ret = DoSetupSink();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to setup sink");

    ret = DoSetupRenderPipeline();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to setup render pipeline");

    ret = DoSetupSignal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to setup signal");

    ret = DoSetupMsgDispatcher();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to setup msg dispatcher");

    ret = DoSetupSource();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to setup source");

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::DoSetupSource()
{
    std::string urlDesc = source->GetGstUrlDesc();
    CHECK_AND_RETURN_RET_LOG(!urlDesc.empty(), MSERR_INVALID_VAL, "Invalid url");

    g_object_set(playbin_, "uri", urlDesc.c_str(), nullptr);

    source_->SetMsgNotifier([this](const InnerMessage &msg) {
        auto handler = std::bind(&PlayBinCtrlerBase::OnMessageReceived, this, std::placeholders::_1);
        (void)workLooper_->EnqueueTask(handler);
    });

    source_->OnPlayBinSetup(playbin_);
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::DoSetupSink()
{
    if (sinkProvider_ == nullptr) {
        MEDIA_LOGW("no sinkprovider, delay the sink selection until the playbin enters pause state.");
        return MSERR_OK;
    }

    auto audioSink = sinkProvider_->CreateAudioSink();
    if (audioSink != nullptr) {
        g_object_set(playbin_, "audio-sink", audioSink, nullptr);
    }

    auto videoSink = sinkProvider_->CreateVideoSink();
    if (videoSink != nullptr) {
        g_object_set(playbin_, "video-sink", videoSink, nullptr);
    } else if (audioSink != nullptr) {
        g_object_set(playbin_, "video-sink", audioSink, nullptr); // ???
    }

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::DoSetupRenderPipeline()
{
    uint32_t flags = 0;
    g_object_get(playbin_, "flags", &flags, nullptr);
    flags &= ~GST_PLAY_FLAG_VIS;

    if ((renderMode_ & RenderMode::NATIVE_VIDEO_STREAM) != 0) {
        flags |= GST_PLAY_FLAG_NATIVE_VIDEO;
        flags &= ~GST_PLAY_FLAG_SOFT_COLORBALANCE;
    }

    if ((renderMode_ & RenderMode::NATIVE_AUDIO_STREAM) != 0) {
        flags |= GST_PLAY_FLAG_NATIVE_AUDIO;
        flags &= ~GST_PLAY_FLAG_SOFT_VOLUME;
    }

    if ((renderMode_ & RenderMode::NATIVE_AUDIO_STREAM) == 0) {
        GstElement *audioFilter = gst_element_factory_make("scaletempo", "scaletempo");
        if (audioFilter != nullptr) {
            g_object_set(playbin_, "audio-filter", audioFilter, nullptr);
        } else {
            MEDIA_LOGD("can not create scaletempo, the audio playback speed can not be adjusted");
        }
    }

    if ((renderMode_ & RenderMode::DISABLE_TEXT) != 0) {
        flags &= ~GST_PLAY_FLAG_TEXT;
    }

    // for hardware native video mode, delay to set flags until the hardware video decoder setup
    g_object_set(playbin_, "flags", flags, nullptr);
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::DoSetupSignal()
{
    gulong signalId = g_signal_connect(playbin_, "source-setup", G_CALLBACK(&PlayBinCtrlerBase::OnSourceSetup), this);
    if (signalId == 0) {
        MEDIA_LOGE("Failed to connect source-setup signal");
        return MSERR_UNKNOWN;
    }
    (void)signalIds_.emplace(GST_ELEMENT_CAST(playbin_), signalId);

    signalId = g_signal_connect(playbin_, "element-setup", G_CALLBACK(&PlayBinCtrlerBase::OnElementSetup), this);
    if (signalId == 0) {
        MEDIA_LOGE("Failed to connect element-setup signal");
        return MSERR_UNKNOWN;
    }
    (void)signalIds_.emplace(GST_ELEMENT_CAST(playbin_), signalId);

    signalId = g_signal_connect_data(playbin_, "deep-element-removed",
        G_CALLBACK(&PlayBinCtrlerBase::OnElementUnSetup), this);
    if (signalId == 0) {
        MEDIA_LOGE("Failed to connect element-setup signal");
        return MSERR_UNKNOWN;
    }
    (void)signalIds_.emplace(GST_ELEMENT_CAST(playbin_), signalId);

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::DoSetupMsgDispatcher()
{
    GstBus *bus = gst_pipeline_get_bus(playbin_);
    CHECK_AND_RETURN_RET_LOG(bus != nullptr, MSERR_UNKNOWN, "can not get bus");

    auto msgNotifier = [this](const InnerMessage &msg) {
        auto handler = std::bind(&PlayBinCtrlerBase::OnMessageReceived, this, std::placeholders::_1);
        (void)workLooper_->EnqueueTask(handler);
    };
    msgDispatcher_ = std::make_unique<GstMsgDispatcher>(*bus, msgNotifier, msgConverter_);

    gst_object_unref(bus);
    bus = nullptr;

    int32_t ret = msgDispatcher_->Init();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    // only concern the msg from playbin
    msgDispatcher_->AddMsgFilter(ELEM_NAME(GST_ELEMENT_CAST(playbin_)));
    return MSERR_OK;
}

void PlayBinCtrlerBase::DoUnSetup()
{
    if (source_ != nullptr) {
        (void)source_->Stop();
        source_->SetMsgNotifier(nullptr);
    }

    if (msgDispatcher_ != nullptr) {
        msgDispatcher_->Reset();
        msgDispatcher_ = nullptr;
    }

    for (auto &[elem, signalId] : signalIds_) {
        g_signal_handler_disconnect(elem, signalId);
    }
    signalIds_.clear();
}

void PlayBinCtrlerBase::OnMessageReceived(const InnerMessage &msg)
{
    switch (msg.type) {
        case INNER_MSG_STATE_CHANGED: {
            if (selfState_ == PLAYBIN_STATE_PREPARING && msg.detail2 == GST_STATE_PAUSED) {
                UpdateStateAndReport(PLAYBIN_STATE_PREPARED);
            } else if (selfState_ == PLAYBIN_STATE_PREPARED) {
                if (msg.detail2 == GST_STATE_READY) {
                    UpdateStateAndReport(PLAYBIN_STATE_STOPPED);
                } else if (msg.detail2 == GST_STATE_PLAYING) {
                    UpdateStateAndReport(PLAYBIN_STATE_PLAYING);
                }
            }
            break;
        }
        default:
            break;
    }
}

void PlayBinCtrlerBase::UpdateStateAndReport(PlayBinState newState)
{
    MEDIA_LOGI("Success change playbin state from %{public}s to %{public}s",
        StringifyPlayBinState(selfState_).data(), StringifyPlayBinState(newState_).data());
    PlayBinState oldState = selfState_;
    selfState_ = newState;

    if (notifier_ != nullptr) {
        notifier_(InnerMessage { INNER_MSG_STATE_CHANGED, oldState, newState });
    }
}
}
}
}