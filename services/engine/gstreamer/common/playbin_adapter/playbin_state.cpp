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
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayBinState"};
    constexpr int32_t USEC_PER_MSEC = 1000;
    constexpr uint32_t DEFAULT_POSITION_UPDATE_INTERVAL_MS = 1000; // 1000 ms
}

namespace OHOS {
namespace Media {
void PlayBinCtrlerBase::BaseState::ReportInvalidOperation()
{
    MEDIA_LOGE("invalid operation for %{public}s", GetStateName().c_str());

    PlayBinMessage msg { PlayBinMsgType::PLAYBIN_MSG_ERROR, 0, MSERR_INVALID_STATE, {} };
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

int32_t PlayBinCtrlerBase::BaseState::SetRate(double rate)
{
    (void)rate;

    ReportInvalidOperation();
    return MSERR_INVALID_STATE;
}

int32_t PlayBinCtrlerBase::BaseState::ChangePlayBinState(GstState targetState)
{
    if (targetState < GST_STATE_PLAYING) {
        int64_t position = ctrler_.QueryPositionInternal(false) / USEC_PER_MSEC;
        int32_t tickType = INNER_MSG_POSITION_UPDATE;
        ctrler_.msgProcessor_->RemoveTickSource(tickType);
        PlayBinMessage posUpdateMsg { PLAYBIN_MSG_POSITION_UPDATE, 0, static_cast<int32_t>(position), {} };
        ctrler_.ReportMessage(posUpdateMsg);
    }

    GstStateChangeReturn ret = gst_element_set_state(GST_ELEMENT_CAST(ctrler_.playbin_), targetState);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        MEDIA_LOGE("Failed to change playbin's state to %{public}s", gst_element_state_get_name(targetState));
        return MSERR_INVALID_OPERATION;
    }

    return MSERR_OK;
}

void PlayBinCtrlerBase::BaseState::HandleStateChange(const InnerMessage &msg)
{
    GstState targetState = static_cast<GstState>(msg.detail2);
    MEDIA_LOGI("state changed from %{public}s to %{public}s",
        gst_element_state_get_name(static_cast<GstState>(msg.detail1)),
        gst_element_state_get_name(targetState));

    if (targetState == GST_STATE_PLAYING) {
        int32_t tickType = INNER_MSG_POSITION_UPDATE;
        uint32_t interval = DEFAULT_POSITION_UPDATE_INTERVAL_MS;
        ctrler_.msgProcessor_->AddTickSource(tickType, interval);
    } else if (targetState == GST_STATE_PAUSED) {
        int32_t tickType = INNER_MSG_POSITION_UPDATE;
        ctrler_.msgProcessor_->RemoveTickSource(tickType);
        if (!ctrler_.isSeeking_ && !ctrler_.isRating_) {
            int64_t position = ctrler_.QueryPositionInternal(false) / USEC_PER_MSEC;
            PlayBinMessage posUpdateMsg { PLAYBIN_MSG_POSITION_UPDATE, 0, static_cast<int32_t>(position), {} };
            ctrler_.ReportMessage(posUpdateMsg);
        }
    }

    Dumper::DumpDotGraph(*ctrler_.playbin_, msg.detail1, msg.detail2);
    if (msg.extend.has_value() && std::any_cast<GstPipeline *>(msg.extend) == ctrler_.playbin_) {
        ProcessStateChange(msg);
        if ((msg.detail1 == GST_STATE_PAUSED && msg.detail2 == GST_STATE_PLAYING) && ctrler_.isNetWorkPlay_) {
            ctrler_.HandleCacheCtrl(ctrler_.cachePercent_);
        }
    }
}

void PlayBinCtrlerBase::BaseState::HandleDurationChange()
{
    MEDIA_LOGI("received duration change msg, update duration");
    ctrler_.QueryDuration();
}

void PlayBinCtrlerBase::BaseState::HandleResolutionChange(const InnerMessage &msg)
{
    std::pair<int32_t, int32_t> resolution;
    resolution.first = msg.detail1;
    resolution.second = msg.detail2;
    PlayBinMessage playBinMsg { PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_VIDEO_SIZE_CHANGED, 0, resolution };
    ctrler_.ReportMessage(playBinMsg);
}

void PlayBinCtrlerBase::BaseState::HandleAsyncDone(const InnerMessage &msg)
{
    if (std::any_cast<GstPipeline *>(msg.extend) == ctrler_.playbin_) {
        GstState state = GST_STATE_NULL;
        GstStateChangeReturn stateRet = gst_element_get_state(GST_ELEMENT_CAST(ctrler_.playbin_), &state,
            nullptr, static_cast<GstClockTime>(0));
        if ((stateRet == GST_STATE_CHANGE_SUCCESS) && (state >= GST_STATE_PAUSED)) {
            if (ctrler_.isSeeking_) {
                int64_t position = ctrler_.seekPos_ / USEC_PER_MSEC;
                ctrler_.isSeeking_ = false;
                ctrler_.isDuration_ = (position == ctrler_.duration_ / USEC_PER_MSEC) ? true : false;
                MEDIA_LOGI("asyncdone after seek done, pos = %{public}" PRIi64 "ms", position);
                PlayBinMessage playBinMsg { PLAYBIN_MSG_SEEKDONE, 0, static_cast<int32_t>(position), {} };
                ctrler_.ReportMessage(playBinMsg);

                PlayBinMessage posUpdateMsg { PLAYBIN_MSG_POSITION_UPDATE, 0, static_cast<int32_t>(position), {} };
                ctrler_.ReportMessage(posUpdateMsg);
            } else if (ctrler_.isRating_) {
                ctrler_.isRating_ = false;
                MEDIA_LOGI("asyncdone after setRate done, rate = %{public}lf", ctrler_.rate_);
                PlayBinMessage playBinMsg { PLAYBIN_MSG_SPEEDDONE, 0, ctrler_.rate_, {} };
                ctrler_.ReportMessage(playBinMsg);

                int64_t position = ctrler_.QueryPositionInternal(false) / USEC_PER_MSEC;
                PlayBinMessage posUpdateMsg { PLAYBIN_MSG_POSITION_UPDATE, 0, static_cast<int32_t>(position), {} };
                ctrler_.ReportMessage(posUpdateMsg);
            } else {
                MEDIA_LOGD("Async done, not seeking or rating!");
            }
        }
    }
}

void PlayBinCtrlerBase::BaseState::HandleError(const InnerMessage &msg)
{
    PlayBinMessage playbinMsg { PLAYBIN_MSG_ERROR, 0, msg.detail1, {} };
    ctrler_.ReportMessage(playbinMsg);
}

void PlayBinCtrlerBase::BaseState::HandleEos()
{
    int64_t position = ctrler_.QueryPositionInternal(false) / USEC_PER_MSEC;
    int32_t tickType = INNER_MSG_POSITION_UPDATE;
    ctrler_.msgProcessor_->RemoveTickSource(tickType);
    PlayBinMessage posUpdateMsg { PLAYBIN_MSG_POSITION_UPDATE, 0, static_cast<int32_t>(position), {} };
    ctrler_.ReportMessage(posUpdateMsg);

    PlayBinMessage playBinMsg = { PLAYBIN_MSG_EOS, 0, static_cast<int32_t>(ctrler_.enableLooping_.load()), {} };
    ctrler_.ReportMessage(playBinMsg);
}

void PlayBinCtrlerBase::BaseState::HandleBuffering(const InnerMessage &msg)
{
    ctrler_.HandleCacheCtrlCb(msg);
}

void PlayBinCtrlerBase::BaseState::HandleBufferingTime(const InnerMessage &msg)
{
    std::pair<uint32_t, int64_t> bufferingTimePair;
    bufferingTimePair.first = static_cast<uint32_t>(msg.detail1);
    bufferingTimePair.second = std::any_cast<int64_t>(msg.extend);
    PlayBinMessage playBinMsg = { PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_BUFFERING_TIME, 0, bufferingTimePair };
    ctrler_.ReportMessage(playBinMsg);
}

void PlayBinCtrlerBase::BaseState::HandleUsedMqNum(const InnerMessage &msg)
{
    uint32_t usedMqNum = static_cast<uint32_t>(msg.detail1);
    PlayBinMessage playBinMsg = { PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_BUFFERING_USED_MQ_NUM, 0, usedMqNum };
    ctrler_.ReportMessage(playBinMsg);
}

void PlayBinCtrlerBase::BaseState::OnMessageReceived(const InnerMessage &msg)
{
    switch (msg.type) {
        case INNER_MSG_STATE_CHANGED:
            HandleStateChange(msg);
            break;
        case INNER_MSG_DURATION_CHANGED:
            HandleDurationChange();
            break;
        case INNER_MSG_RESOLUTION_CHANGED:
            HandleResolutionChange(msg);
            break;
        case INNER_MSG_ASYNC_DONE:
            HandleAsyncDone(msg);
            break;
        case INNER_MSG_ERROR:
            HandleError(msg);
            break;
        case INNER_MSG_EOS:
            HandleEos();
            break;
        case INNER_MSG_BUFFERING:
            HandleBuffering(msg);
            break;
        case INNER_MSG_BUFFERING_TIME:
            HandleBufferingTime(msg);
            break;
        case INNER_MSG_BUFFERING_USED_MQ_NUM:
            HandleUsedMqNum(msg);
            break;
        case INNER_MSG_POSITION_UPDATE:
            HandlePositionUpdate();
            break;
        default:
            break;
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

void PlayBinCtrlerBase::PreparingState::ProcessStateChange(const InnerMessage &msg)
{
    if ((msg.detail1 == GST_STATE_READY) && (msg.detail2 == GST_STATE_PAUSED)) {
        ctrler_.ChangeState(ctrler_.preparedState_);
        ctrler_.stateCond_.notify_one(); // awake the stateCond_'s waiter in Prepare()
    }
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
    return ctrler_.SeekInternal(timeUs, option);
}

int32_t PlayBinCtrlerBase::PreparedState::Stop()
{
    // change to stop always success
    ctrler_.ChangeState(ctrler_.stoppedState_);
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PreparedState::SetRate(double rate)
{
    return ctrler_.SetRateInternal(rate);
}

void PlayBinCtrlerBase::PreparedState::ProcessStateChange(const InnerMessage &msg)
{
    if ((msg.detail1 == GST_STATE_PAUSED) && (msg.detail2 == GST_STATE_PLAYING)) {
        ctrler_.ChangeState(ctrler_.playingState_);
        return;
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
    ctrler_.isUserSetPause_ = true;
    return ChangePlayBinState(GST_STATE_PAUSED);
}

int32_t PlayBinCtrlerBase::PlayingState::Seek(int64_t timeUs, int32_t option)
{
    return ctrler_.SeekInternal(timeUs, option);
}

int32_t PlayBinCtrlerBase::PlayingState::Stop()
{
    // change to stop always success
    ctrler_.ChangeState(ctrler_.stoppedState_);
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PlayingState::SetRate(double rate)
{
    return ctrler_.SetRateInternal(rate);
}

void PlayBinCtrlerBase::PlayingState::ProcessStateChange(const InnerMessage &msg)
{
    if ((msg.detail1 == GST_STATE_PLAYING) && (msg.detail2 == GST_STATE_PAUSED) &&
        !ctrler_.isBuffering_ && ctrler_.isUserSetPause_) {
        ctrler_.ChangeState(ctrler_.pausedState_);
        ctrler_.isUserSetPause_ = false;
        return;
    }

    if (msg.detail2 == GST_STATE_PLAYING) {
        GstState state = GST_STATE_NULL;
        GstStateChangeReturn stateRet = gst_element_get_state(GST_ELEMENT_CAST(ctrler_.playbin_), &state,
            nullptr, static_cast<GstClockTime>(0));
        if ((stateRet == GST_STATE_CHANGE_SUCCESS) && (state == GST_STATE_PLAYING)) {
            if (ctrler_.isSeeking_) {
                int64_t position = ctrler_.seekPos_ / USEC_PER_MSEC;
                ctrler_.isSeeking_ = false;
                ctrler_.isDuration_ = (position == ctrler_.duration_ / USEC_PER_MSEC) ? true : false;
                MEDIA_LOGI("playing after seek done, pos = %{public}" PRIi64 "ms", position);
                PlayBinMessage playBinMsg { PLAYBIN_MSG_SEEKDONE, 0, static_cast<int32_t>(position), {} };
                ctrler_.ReportMessage(playBinMsg);

                PlayBinMessage posUpdateMsg { PLAYBIN_MSG_POSITION_UPDATE, 0, static_cast<int32_t>(position), {} };
                ctrler_.ReportMessage(posUpdateMsg);
            } else if (ctrler_.isRating_) {
                ctrler_.isRating_ = false;
                MEDIA_LOGI("playing after setRate done, rate = %{public}lf", ctrler_.rate_);
                PlayBinMessage playBinMsg { PLAYBIN_MSG_SPEEDDONE, 0, ctrler_.rate_, {} };
                ctrler_.ReportMessage(playBinMsg);

                int64_t position = ctrler_.QueryPositionInternal(false) / USEC_PER_MSEC;
                PlayBinMessage posUpdateMsg { PLAYBIN_MSG_POSITION_UPDATE, 0, static_cast<int32_t>(position), {} };
                ctrler_.ReportMessage(posUpdateMsg);
            } else {
                MEDIA_LOGD("playing, not seeking or rating!");
            }
        }
    }
}

void PlayBinCtrlerBase::PlayingState::HandlePositionUpdate()
{
    int64_t position = ctrler_.QueryPositionInternal(false) / USEC_PER_MSEC;
    PlayBinMessage posUpdateMsg { PLAYBIN_MSG_POSITION_UPDATE, 0, static_cast<int32_t>(position), {} };
    ctrler_.ReportMessage(posUpdateMsg);
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
    return ctrler_.SeekInternal(timeUs, option);
}

int32_t PlayBinCtrlerBase::PausedState::Stop()
{
    // change to stop always success
    ctrler_.ChangeState(ctrler_.stoppedState_);
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PausedState::SetRate(double rate)
{
    return ctrler_.SetRateInternal(rate);
}

void PlayBinCtrlerBase::PausedState::ProcessStateChange(const InnerMessage &msg)
{
    if ((msg.detail1 == GST_STATE_PAUSED) && (msg.detail2 == GST_STATE_PLAYING)) {
        ctrler_.ChangeState(ctrler_.playingState_);
    }
}

void PlayBinCtrlerBase::StoppedState::StateEnter()
{
    // maybe need the deferred task to change state from ready to null, refer to gstplayer.

    (void)ChangePlayBinState(GST_STATE_READY);
    ctrler_.isDuration_ = false;

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

void PlayBinCtrlerBase::StoppedState::ProcessStateChange(const InnerMessage &msg)
{
    if (msg.detail2 == GST_STATE_READY) {
        PlayBinMessage playBinMsg = { PLAYBIN_MSG_STATE_CHANGE, 0, PLAYBIN_STATE_STOPPED, {} };
        ctrler_.ReportMessage(playBinMsg);
    }
}

void PlayBinCtrlerBase::PlaybackCompletedState::StateEnter()
{
    PlayBinMessage msg = { PLAYBIN_MSG_STATE_CHANGE, 0, PLAYBIN_STATE_PLAYBACK_COMPLETE, {} };
    ctrler_.ReportMessage(msg);
}

int32_t PlayBinCtrlerBase::PlaybackCompletedState::Play()
{
    ctrler_.isDuration_ = false;
    return ctrler_.SeekInternal(0, IPlayBinCtrler::PlayBinSeekMode::PREV_SYNC);
}

int32_t PlayBinCtrlerBase::PlaybackCompletedState::Stop()
{
    ctrler_.ChangeState(ctrler_.stoppedState_);
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PlaybackCompletedState::Seek(int64_t timeUs, int32_t option)
{
    (void)option;
    int64_t position = timeUs / USEC_PER_MSEC;
    PlayBinMessage msg = { PLAYBIN_MSG_SEEKDONE, 0, static_cast<int32_t>(position), {} };
    ctrler_.ReportMessage(msg);
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PlaybackCompletedState::SetRate(double rate)
{
    ctrler_.rate_ = rate;
    PlayBinMessage msg = { PLAYBIN_MSG_SPEEDDONE, 0, rate, {} };
    ctrler_.ReportMessage(msg);
    return MSERR_OK;
}

void PlayBinCtrlerBase::PlaybackCompletedState::ProcessStateChange(const InnerMessage &msg)
{
    (void)msg;
    if (msg.detail2 == GST_STATE_PLAYING && ctrler_.isSeeking_) {
        ctrler_.ChangeState(ctrler_.playingState_);
        ctrler_.isSeeking_ = false;
    }
}

void PlayBinCtrlerBase::PlaybackCompletedState::HandleAsyncDone(const InnerMessage &msg)
{
    (void)msg;
}
} // namespace Media
} // namespace OHOS