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

#include "playbin_state.h"
#include <gst/gst.h>
#include <gst/playback/gstplay-enum.h>
#include "media_errors.h"
#include "media_log.h"
#include "dumper.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayBinCtrlerBase"};
}

namespace OHOS {
namespace Media {
int32_t PlayBinCtrlerBase::BaseState::SetUp()
{
    MEDIA_LOGE("invalid state");
    return MSERR_INVALID_STATE;
}

int32_t PlayBinCtrlerBase::BaseState::Prepare()
{
    MEDIA_LOGE("invalid state");
    return MSERR_INVALID_STATE;
}

int32_t PlayBinCtrlerBase::BaseState::Play()
{
    MEDIA_LOGE("invalid state");
    return MSERR_INVALID_STATE;
}

int32_t PlayBinCtrlerBase::BaseState::Pause()
{
    MEDIA_LOGE("invalid state");
    return MSERR_INVALID_STATE;
}

int32_t PlayBinCtrlerBase::BaseState::Seek(int64_t timeUs, int32_t option)
{
    (void)timeUs;
    (void)option;

    MEDIA_LOGE("invalid state");
    return MSERR_INVALID_STATE;
}

int32_t PlayBinCtrlerBase::BaseState::Stop()
{
    MEDIA_LOGE("invalid state");
    return MSERR_INVALID_STATE;
}

void PlayBinCtrlerBase::BaseState::OnMessageReceived(const InnerMessage &msg)
{
    ProcessMessage(msg);

    // process error, wanring, info msg , dump dot graph at here.
    if (msg.type == INNER_MSG_STATE_CHANGED) {
        Dumper::DumpDotGraph(*ctrler_.playbin_, msg.detail1, msg.detail2);
    }

    if (msg.type == INNER_MSG_ERROR) {
        if (ctrler_.GetCurrState() != ctrler_.idleState_) {
            auto stopTask = std::make_shared<TaskHandler<void>>([this]() {
                ctrler_.ChangeState(ctrler_.stoppedState_);
            });
            ctrler_.DeferTask(stopTask, 0);
        }

        PlayBinMessage playbinMsg { PLAYBIN_MSG_ERROR, 0, msg.detail1 };
        ctrler_.ReportMessage(playbinMsg);
    }
}

int32_t PlayBinCtrlerBase::IdleState::SetUp()
{
    MEDIA_LOGD("IdleState::SetUp enter");

    int32_t ret = ctrler_.OnInit();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);
    CHECK_AND_RETURN_RET(ctrler_.playbin_ != nullptr, static_cast<int32_t>(MSERR_UNKNOWN));

    ctrler_.playbin_ = GST_PIPELINE_CAST(gst_object_ref(ctrler_.playbin_));
    ctrler_.SetupCustomElement();
    ret = ctrler_.SetupSignalMessage();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    if (ctrler_.currScene_ == PlayBinScene::METADATA || ctrler_.currScene_ == PlayBinScene::THUBNAIL) {
        uint32_t flags;
        g_object_get(ctrler_.playbin_, "flags", &flags, nullptr);
        flags |= GST_PLAY_FLAG_NATIVE_VIDEO | GST_PLAY_FLAG_NATIVE_AUDIO;
        flags &= ~(GST_PLAY_FLAG_SOFT_COLORBALANCE | GST_PLAY_FLAG_SOFT_VOLUME);
        g_object_set(ctrler_.playbin_, "flags", flags, nullptr);
    }

    g_object_set(ctrler_.playbin_, "uri", ctrler_.uri_.c_str(), nullptr);
    ctrler_.ChangeState(ctrler_.initializedState_);

    MEDIA_LOGD("IdleState::SetUp exit");
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::InitializedState::Prepare()
{
    ctrler_.ChangeState(ctrler_.preparingState_);
    return MSERR_OK;
}

void PlayBinCtrlerBase::PreparingState::StateEnter()
{
    PlayBinMessage msg = { PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_BUFFERING_START, 0, {} };
    ctrler_.ReportMessage(msg);

    GstStateChangeReturn ret = gst_element_set_state(GST_ELEMENT_CAST(ctrler_.playbin_), GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        MEDIA_LOGE("Failed to change playbin's state to paused");
        return;
    }

    MEDIA_LOGD("PreparingState::StateEnter finished");
}

void PlayBinCtrlerBase::PreparingState::ProcessMessage(const InnerMessage &msg)
{
    if (msg.type == INNER_MSG_STATE_CHANGED) {
        if ((msg.detail1 == GST_STATE_READY) && (msg.detail2 == GST_STATE_PAUSED)) {
            MEDIA_LOGI("state changed from ready to paused");
            ctrler_.ChangeState(ctrler_.preparedState_);
            return;
        }
    }
}

void PlayBinCtrlerBase::PreparingState::StateExit()
{
    PlayBinMessage msg = { PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_BUFFERING_END, 0, {} };
    ctrler_.ReportMessage(msg);
}

void PlayBinCtrlerBase::PreparedState::StateEnter()
{
    PlayBinMessage msg = { PLAYBIN_MSG_STATE_CHANGE, 0, PLAYBIN_STATE_PREPARED, {} };
    ctrler_.ReportMessage(msg);

    // get duration at here.
}

int32_t PlayBinCtrlerBase::PreparedState::Prepare()
{
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PreparedState::Play()
{
    MEDIA_LOGD("PreparedState::Play begin");

    GstStateChangeReturn ret = gst_element_set_state(GST_ELEMENT_CAST(ctrler_.playbin_), GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        MEDIA_LOGE("Failed to change playbin's state to playing");
        return MSERR_INVALID_OPERATION;
    }

    MEDIA_LOGD("PreparedState::Play finished");
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PreparedState::Pause()
{
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PreparedState::Seek(int64_t timeUs, int32_t option)
{
    (void)timeUs;
    (void)option;
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PreparedState::Stop()
{
    // change to stop always success
    ctrler_.ChangeState(ctrler_.stoppedState_);
    return MSERR_OK;
}

void PlayBinCtrlerBase::PreparedState::ProcessMessage(const InnerMessage &msg)
{
    if (msg.type == INNER_MSG_STATE_CHANGED) {
        if ((msg.detail1 == GST_STATE_PAUSED) && (msg.detail2 == GST_STATE_PLAYING)) {
            MEDIA_LOGI("state changed from paused to playing");
            ctrler_.ChangeState(ctrler_.playingState_);
            return;
        }
    }
}

void PlayBinCtrlerBase::PlayingState::StateEnter()
{
    PlayBinMessage msg = { PLAYBIN_MSG_STATE_CHANGE, 0, PLAYBIN_STATE_PLAYING, {} };
    ctrler_.ReportMessage(msg);

    msg.type = PLAYBIN_MSG_SUBTYPE;
    msg.subType = PLAYBIN_SUB_MSG_VIDEO_RENDING_START;
    ctrler_.ReportMessage(msg);

    // add tick handler to periodically query the current location
    // if the state = eos, should seek to 0 position
}

int32_t PlayBinCtrlerBase::PlayingState::Play()
{
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PlayingState::Pause()
{
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PlayingState::Seek(int64_t timeUs, int32_t option)
{
    (void)timeUs;
    (void)option;
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PlayingState::Stop()
{
    return MSERR_OK;
}

void PlayBinCtrlerBase::PlayingState::ProcessMessage(const InnerMessage &msg)
{
    (void)msg;
}

void PlayBinCtrlerBase::PausedState::StateEnter()
{
    PlayBinMessage msg = { PLAYBIN_MSG_STATE_CHANGE, 0, PLAYBIN_STATE_PAUSED, {} };
    ctrler_.ReportMessage(msg);
}

int32_t PlayBinCtrlerBase::PausedState::Play()
{
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PausedState::Pause()
{
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PausedState::Seek(int64_t timeUs, int32_t option)
{
    (void)timeUs;
    (void)option;
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PausedState::Stop()
{
    return MSERR_OK;
}

void PlayBinCtrlerBase::PausedState::ProcessMessage(const InnerMessage &msg)
{
    (void)msg;
}

void PlayBinCtrlerBase::StoppedState::StateEnter()
{
    // maybe need the deferred task to change state from ready to null, refer to gstplayer.

    ctrler_.msgProcessor_->FlushBegin();
    GstStateChangeReturn ret = gst_element_set_state(GST_ELEMENT_CAST(ctrler_.playbin_), GST_STATE_READY);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        MEDIA_LOGW("Failed to change playbin's state to ready");
    }
    ctrler_.msgProcessor_->FlushEnd();

    MEDIA_LOGD("StoppedState::StateEnter finished");
}

int32_t PlayBinCtrlerBase::StoppedState::Prepare()
{
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::StoppedState::Play()
{
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::StoppedState::Pause()
{
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::StoppedState::Seek(int64_t timeUs, int32_t option)
{
    (void)timeUs;
    (void)option;
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::StoppedState::Stop()
{
    return MSERR_OK;
}

void PlayBinCtrlerBase::StoppedState::ProcessMessage(const InnerMessage &msg)
{
    if (msg.type == INNER_MSG_STATE_CHANGED) {
        if (msg.detail2 == GST_STATE_READY) {
            MEDIA_LOGI("state changed from %{public}s to %{public}s",
                       gst_element_state_get_name(static_cast<GstState>(msg.detail1)),
                       gst_element_state_get_name(static_cast<GstState>(msg.detail2)));

            PlayBinMessage playBinMsg = { PLAYBIN_MSG_STATE_CHANGE, 0, PLAYBIN_STATE_STOPPED, {} };
            ctrler_.ReportMessage(playBinMsg);
            return;
        }
    }
}
}
}