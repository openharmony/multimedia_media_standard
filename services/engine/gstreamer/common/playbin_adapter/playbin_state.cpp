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
void PlayBinCtrlerBase::BaseState::ReportInvalidOperation()
{
    MEDIA_LOGE("invalid operation for %{public}s", GetStateName().c_str());

    PlayBinMessage msg { PlayBinMsgType::PLAYBIN_MSG_ERROR, 0, MSERR_INVALID_STATE };
    ctrler_.ReportMessage(msg);
}

int32_t PlayBinCtrlerBase::BaseState::Prepare()
{
    ReportInvalidOperation();
    return MSERR_INVALID_STATE;
}

int32_t PlayBinCtrlerBase::BaseState::Play()
{
    ReportInvalidOperation();
    return MSERR_INVALID_STATE;
}

int32_t PlayBinCtrlerBase::BaseState::Pause()
{
    ReportInvalidOperation();
    return MSERR_INVALID_STATE;
}

int32_t PlayBinCtrlerBase::BaseState::Seek(int64_t timeUs, int32_t option)
{
    (void)timeUs;
    (void)option;

    ReportInvalidOperation();
    return MSERR_INVALID_STATE;
}

int32_t PlayBinCtrlerBase::BaseState::Stop()
{
    ReportInvalidOperation();
    return MSERR_INVALID_STATE;
}

int32_t PlayBinCtrlerBase::BaseState::ChangePlayBinState(GstState targetState)
{
    GstStateChangeReturn ret = gst_element_set_state(GST_ELEMENT_CAST(ctrler_.playbin_), targetState);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        MEDIA_LOGE("Failed to change playbin's state to %{public}s", gst_element_state_get_name(targetState));
        return MSERR_INVALID_OPERATION;
    }

    return MSERR_OK;
}

void PlayBinCtrlerBase::BaseState::OnMessageReceived(const InnerMessage &msg)
{
    ProcessMessage(msg);

    if (msg.type == INNER_MSG_STATE_CHANGED) {
        MEDIA_LOGI("state changed from %{public}s to %{public}s",
                   gst_element_state_get_name(static_cast<GstState>(msg.detail1)),
                   gst_element_state_get_name(static_cast<GstState>(msg.detail2)));

        // every time the state of playbin changed, we try to awake the stateCond_'s waiters.
        ctrler_.stateCond_.notify_one();

        Dumper::DumpDotGraph(*ctrler_.playbin_, msg.detail1, msg.detail2);
    }

    if (msg.type == INNER_MSG_DURATION_CHANGED) {
        if (this == ctrler_.preparingState_.get()) {
            return;
        }
        MEDIA_LOGI("received duration change msg, update duration");
        ctrler_.QueryDuration();
        return;
    }

    if (msg.type == INNER_MSG_ERROR) {
        PlayBinMessage playbinMsg { PLAYBIN_MSG_ERROR, 0, msg.detail1 };
        ctrler_.ReportMessage(playbinMsg);
    }
}

void PlayBinCtrlerBase::IdleState::StateEnter()
{
    ctrler_.ExitInitializedState();
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

    (void)ChangePlayBinState(GST_STATE_PAUSED);

    MEDIA_LOGD("PreparingState::StateEnter finished");
}

int32_t PlayBinCtrlerBase::PreparingState::Stop()
{
    // change to stop always success
    ctrler_.ChangeState(ctrler_.stoppedState_);
    return MSERR_OK;
}

void PlayBinCtrlerBase::PreparingState::ProcessMessage(const InnerMessage &msg)
{
    if (msg.type == INNER_MSG_STATE_CHANGED) {
        if ((msg.detail1 == GST_STATE_READY) && (msg.detail2 == GST_STATE_PAUSED)) {
            stateChanged_ = true;
            return;
        }
    }

    if (msg.type == INNER_MSG_ASYNC_DONE) {
        if (stateChanged_) {
            ctrler_.ChangeState(ctrler_.preparedState_);
            stateChanged_ = false;
            return;
        }
    }
}

void PlayBinCtrlerBase::PreparingState::StateExit()
{
    (void)ctrler_.taskMgr_.MarkSecondPhase();
}

void PlayBinCtrlerBase::PreparedState::StateEnter()
{
    PlayBinMessage msg = { PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_BUFFERING_END, 0, {} };
    ctrler_.ReportMessage(msg);

    msg = { PLAYBIN_MSG_STATE_CHANGE, 0, PLAYBIN_STATE_PREPARED, {} };
    ctrler_.ReportMessage(msg);

    ctrler_.QueryDuration();
}

int32_t PlayBinCtrlerBase::PreparedState::Prepare()
{
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PreparedState::Play()
{
    return ChangePlayBinState(GST_STATE_PLAYING);
}

int32_t PlayBinCtrlerBase::PreparedState::Seek(int64_t timeUs, int32_t option)
{
    return ctrler_.SeekInternel(timeUs, option);
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
            ctrler_.ChangeState(ctrler_.playingState_);
            (void)ctrler_.taskMgr_.MarkSecondPhase();
            return;
        }
    }

    if (msg.type == INNER_MSG_ASYNC_DONE) {
        if (ctrler_.taskMgr_.GetCurrTaskType() == PlayBinTaskType::SEEKING) {
            MEDIA_LOGI("seek done");
            PlayBinMessage playBinMsg { PLAYBIN_MSG_SEEKDONE, 0, 0 };
            ctrler_.ReportMessage(playBinMsg);
            (void)ctrler_.taskMgr_.MarkSecondPhase();
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
    return ChangePlayBinState(GST_STATE_PAUSED);
}

int32_t PlayBinCtrlerBase::PlayingState::Seek(int64_t timeUs, int32_t option)
{
    return ctrler_.SeekInternel(timeUs, option);
}

int32_t PlayBinCtrlerBase::PlayingState::Stop()
{
    // change to stop always success
    ctrler_.ChangeState(ctrler_.stoppedState_);
    return MSERR_OK;
}

void PlayBinCtrlerBase::PlayingState::ProcessMessage(const InnerMessage &msg)
{
    if (msg.type == INNER_MSG_STATE_CHANGED) {
        if ((msg.detail1 == GST_STATE_PLAYING) && (msg.detail2 == GST_STATE_PAUSED)) {
            ctrler_.ChangeState(ctrler_.pausedState_);
            (void)ctrler_.taskMgr_.MarkSecondPhase();
            return;
        }
        if ((msg.detail1 == GST_STATE_PAUSED) && (msg.detail2 == GST_STATE_PLAYING)) {
            MEDIA_LOGI("seek done");
            PlayBinMessage playBinMsg { PLAYBIN_MSG_SEEKDONE, 0, 0 };
            ctrler_.ReportMessage(playBinMsg);
            (void)ctrler_.taskMgr_.MarkSecondPhase();
            return;
        }
    }
}

void PlayBinCtrlerBase::PausedState::StateEnter()
{
    PlayBinMessage msg = { PLAYBIN_MSG_STATE_CHANGE, 0, PLAYBIN_STATE_PAUSED, {} };
    ctrler_.ReportMessage(msg);
}

int32_t PlayBinCtrlerBase::PausedState::Play()
{
    return ChangePlayBinState(GST_STATE_PLAYING);
}

int32_t PlayBinCtrlerBase::PausedState::Pause()
{
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PausedState::Seek(int64_t timeUs, int32_t option)
{
    return ctrler_.SeekInternel(timeUs, option);
}

int32_t PlayBinCtrlerBase::PausedState::Stop()
{
    // change to stop always success
    ctrler_.ChangeState(ctrler_.stoppedState_);
    return MSERR_OK;
}

void PlayBinCtrlerBase::PausedState::ProcessMessage(const InnerMessage &msg)
{
    if (msg.type == INNER_MSG_STATE_CHANGED) {
        if ((msg.detail1 == GST_STATE_PAUSED) && (msg.detail2 == GST_STATE_PLAYING)) {
            ctrler_.ChangeState(ctrler_.playingState_);
            (void)ctrler_.taskMgr_.MarkSecondPhase();
            return;
        }
    }
}

void PlayBinCtrlerBase::StoppedState::StateEnter()
{
    // maybe need the deferred task to change state from ready to null, refer to gstplayer.

    (void)ChangePlayBinState(GST_STATE_READY);

    MEDIA_LOGD("StoppedState::StateEnter finished");
}

int32_t PlayBinCtrlerBase::StoppedState::Prepare()
{
    ctrler_.ChangeState(ctrler_.preparingState_);
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
            PlayBinMessage playBinMsg = { PLAYBIN_MSG_STATE_CHANGE, 0, PLAYBIN_STATE_STOPPED, {} };
            ctrler_.ReportMessage(playBinMsg);
            (void)ctrler_.taskMgr_.MarkSecondPhase();
            return;
        }
    }
}
}
}