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

#include "playbin_async_action.h"
#include <chrono>
#include "media_errors.h"
#include "media_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayBinAsynAction"};
}

namespace OHOS {
namespace Media {
namespace PlayBin {
struct StateTransitionEntry {
    PlayBinState startState;
    GstState startGstState;
    PlayBinState targetState;
    GstState targetGstState;
};

#define STATE_TRANS_ENTRY(s, sg, t, tg) { PLAYBIN_STATE_##s, GST_STATE_##sg, PLAYBIN_STATE_##t, GST_STATE_##tg }

static const std::vector<StateTransitionEntry> STATE_TRANSTION_TABLE = {
    STATE_TRANS_ENTRY(IDLE, NULL, PREPARED, PAUSED),
    STATE_TRANS_ENTRY(PREPARED, PAUSED, PLAYING, PLAYING),
    STATE_TRANS_ENTRY(PREPARED, PAUSED, STOPPED, READY),
    STATE_TRANS_ENTRY(PLAYING, PLAYING, PAUSED, PAUSED),
    STATE_TRANS_ENTRY(PLAYING, PLAYING, STOPPED, READY),
    STATE_TRANS_ENTRY(PAUSED, PAUSED, PLAYING, PLAYING),
    STATE_TRANS_ENTRY(PAUSED, PAUSED, STOPPED, READY),
    STATE_TRANS_ENTRY(STOPPED, READY, PREPARED, PAUSED),
    STATE_TRANS_ENTRY(STOPPED, READY, IDLE, NULL),
};

int32_t ChangeStateAction::Execute()
{
    startState_ = stateOperator_.GetPlayBinState();
    GstState startGstState_ = stateOperator_.GetGstState();
    MEDIA_LOGI("Begin to change state from %{public}s to %{public}s, curr gst state: %{public}s",
        StringifyPlayBinState(startState_).data(), StringifyPlayBinState(targetState_).data(),
        gst_element_state_get_name(startGstState_));

    if (startGstState_ == GST_STATE_VOID_PENDING) {
        MEDIA_LOGE("Invalid gst state");
        return MSERR_INVALID_STATE;
    }

    for (auto &transEntry : STATE_TRANSTION_TABLE) {
        if (startState_ != transEntry.startState || targetState_ != transEntry.targetState ||
            startGstState_ != transEntry.startGstState) {
            continue;
        }

        int32_t ret = stateOperator_.SetGstState(transEntry.targetGstState);
        CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);
        targetGstState_ = transEntry.targetGstState;

        if (targetState_ == PLAYBIN_STATE_PREPARED) {
            ret = stateOperator_.SetPlayBinState(PLAYBIN_STATE_PREPARING);
            CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to set playbin state to PREPARING");
        }

        return MSERR_OK;
    }

    MEDIA_LOGE("Failed to change state from %{public}s to %{public}s, curr gst state: %{public}s",
        StringifyPlayBinState(startState_).data(), StringifyPlayBinState(targetState_).data(),
        gst_element_state_get_name(startGstState_));
    return MSERR_INVALID_STATE;
}

void ChangeStateAction::HandleMessage(const InnerMessage &inMsg)
{
    if (inMsg.type != INNER_MSG_STATE_CHANGED) {
        return;
    }

    MEDIA_LOGI("Got gst state change message, from %{public}s to %{public}s",
        gst_element_state_get_name(inMsg.detail1), gst_element_state_get_name(inMsg.detail2));

    if (inMsg.detail2 == targetGstState_) {
        MEDIA_LOGI("Success change state from %{public}s to %{public}s",
            StringifyPlayBinState(startState_).data(), StringifyPlayBinState(targetState_).data());
        int32_t ret = stateOperator_.SetPlayBinState(targetState_);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to set playbin state");

        observer_.OnActionDone(*this);
        return;
    }
}

int32_t SeekAction::Execute()
{
    PlayBinState currState = stateOperator_.GetPlayBinState();
    if (currState != PLAYBIN_STATE_PREPARED &&
        currState != PLAYBIN_STATE_PLAYING &&
        currState != PLAYBIN_STATE_PAUSED) {
        MEDIA_LOGE("Invalid state %{public}s to seek", StringifyPlayBinState(currState).data());
        return MSERR_INVALID_STATE;
    }

    MEDIA_LOGI("Begin seek to position %{public}" PRIi64 "us, mode: %{public}hhu", position_, mode_);
    return sender_.SendSeekEvent(position_, mode_);
}

void SeekAction::HandleMessage(const InnerMessage &inMsg)
{
    if (inMsg.type != INNER_MSG_ASYNC_DONE && inMsg.type != INNER_MSG_STATE_CHANGED) {
        return;
    }

    if (inMsg.type == INNER_MSG_ASYNC_DONE) {
        GstState currGstState = stateOperator_.GetGstState();
        if (currGstState < GST_STATE_PAUSED) {
            return;
        }

        MEDIA_LOGI("Success to seek to position %{public}" PRIi64 "", position_);
        observer_.OnActionDone(*this);
        sender_.SendMessage(InnerMessage { PLAYBIN_MSG_SEEK_DONE, position_ });
        return;
    }

    if (inMsg.type == INNER_MSG_STATE_CHANGED) {
        if (inMsg.detail1 != GST_STATE_PAUSED || inMsg.detail2 != GST_STATE_PLAYING) {
            return;
        }

        PlayBinState currState = stateOperator_.GetPlayBinState();
        if (currState != PLAYBIN_STATE_PLAYING) {
            MEDIA_LOGW("Not in PLAYBIN_STATE_PLAYING, but got gst state change from PAUSED to PLAYING");
            return;
        }

        MEDIA_LOGI("Success to seek to position %{public}" PRIi64 "", position_);
        observer_.OnActionDone(*this);
        sender_.SendMessage(InnerMessage { PLAYBIN_MSG_SEEK_DONE, position_ });

        return;
    }
}

int32_t SetSpeedAction::Execute()
{
    PlayBinState currState = stateOperator_.GetPlayBinState();
    if (currState != PLAYBIN_STATE_PREPARED &&
        currState != PLAYBIN_STATE_PLAYING &&
        currState != PLAYBIN_STATE_PAUSED) {
        MEDIA_LOGE("Invalid state %{public}s to set speed", StringifyPlayBinState(currState).data());
        return MSERR_INVALID_STATE;
    }

    MEDIA_LOGI("Begin set speed to %{public}lf", rate_);
    return sender_.SendSpeedEvent(rate_);
}

void SetSpeedAction::HandleMessage(const InnerMessage &inMsg)
{
    if (inMsg.type != INNER_MSG_ASYNC_DONE && inMsg.type != INNER_MSG_STATE_CHANGED) {
        return;
    }

    if (inMsg.type == INNER_MSG_ASYNC_DONE) {
        GstState currGstState = stateOperator_.GetGstState();
        if (currGstState < GST_STATE_PAUSED) {
            return;
        }

        MEDIA_LOGI("Success to set speed to %{public}lf", rate_);
        observer_.OnActionDone(*this);
        sender_.SendMessage(InnerMessage { PLAYBIN_MSG_SPEED_DONE, rate_ });
        return;
    }

    if (inMsg.type == INNER_MSG_STATE_CHANGED) {
        if (inMsg.detail1 != GST_STATE_PAUSED || inMsg.detail2 != GST_STATE_PLAYING) {
            return;
        }

        PlayBinState currState = stateOperator_.GetPlayBinState();
        if (currState != PLAYBIN_STATE_PLAYING) {
            MEDIA_LOGW("Not in PLAYBIN_STATE_PLAYING, but got gst state change from PAUSED to PLAYING");
            return;
        }

        MEDIA_LOGI("Success to set speed to %{public}lf", rate_);
        observer_.OnActionDone(*this);
        sender_.SendMessage(InnerMessage { PLAYBIN_MSG_SPEED_DONE, rate_ });

        return;
    }
}
}
}
}