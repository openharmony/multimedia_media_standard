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

#include "volume_control.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include "audio_svc_manager.h"
#include "media_errors.h"
#include "media_log.h"

using namespace std;

namespace OHOS {
namespace Media {
const int32_t MAX_VOL_LEVEL = 15;
const int32_t MIN_VOL_LEVEL = 0;
const int32_t CUR_VOL_LEVEL = 10;
const float MAX_VOL_RANGE = 300.0f;
const int32_t HALF_FACTOR = 2;
const float EPSINON = 1e-6f;

VolumeControl::VolumeControl()
{
    mAudioRendererSink_ = nullptr;
    mAudioSvcMgr_ = nullptr;
    VolumeControl::VolumeInit();
}

void VolumeControl::SetRenderer(AudioRendererSink *renderer)
{
    mAudioRendererSink_ = renderer;
}

int32_t VolumeControl::VolumeInit()
{
    if (mAudioSvcMgr_ == nullptr) {
        mAudioSvcMgr_ = AudioSvcManager::GetInstance();
    }

    /* Get the volume from Audio Service */
    int ret = mAudioSvcMgr_->GetVolume(AudioSvcManager::AudioVolumeType::STREAM_MUSIC);
    if (ret >= 0) {
        MEDIA_ERR_LOG("Received Current volume from AudiSvcManager Volume=%d", ret);
        mCurVolLevel_ = ret;
    } else {
        MEDIA_ERR_LOG("Setting Default Current volume, as couldnot get from AudiSvcManager");
        mCurVolLevel_ = CUR_VOL_LEVEL;
    }

    mMaxVolLevel_ = mAudioSvcMgr_->GetMaxVolume(AudioSvcManager::AudioVolumeType::STREAM_MUSIC);
    if (mMaxVolLevel_ <= 0) {
        MEDIA_ERR_LOG("Getting Default MAX volume from AudioSvcMgr failed, so setting default");
        mMaxVolLevel_ = MAX_VOL_LEVEL;
    }

    mMinVolLevel_ = mAudioSvcMgr_->GetMinVolume(AudioSvcManager::AudioVolumeType::STREAM_MUSIC);
    if (mMinVolLevel_ < 0) {
        MEDIA_ERR_LOG("Getting Default MIN volume from AudioSvcMgr failed, so setting default");
        mMinVolLevel_ = MIN_VOL_LEVEL;
    }

    MEDIA_INFO_LOG("Max volume=%d, Min Volume=%d, Current Volume=%d", mMaxVolLevel_,
        mMinVolLevel_, (int32_t)mCurVolLevel_);
    return 0;
}

float VolumeControl::GetAvgVolume(float lVolume, float rVolume)
{
    float Volume;

    if ((lVolume > -EPSINON && lVolume < EPSINON) && !(rVolume > -EPSINON && rVolume < EPSINON)) {
        Volume = rVolume;
    } else if (!(lVolume > -EPSINON && lVolume < EPSINON) && (rVolume > -EPSINON && rVolume < EPSINON)) {
        Volume = lVolume;
    } else {
        Volume = (lVolume + rVolume) / HALF_FACTOR;
    }
    MEDIA_INFO_LOG("Avg volume value to set: (%f)", Volume);

    return Volume;
}

float VolumeControl::VolumeToLevel(float Volume)
{
    return (Volume * mMaxVolLevel_);
}

int32_t VolumeControl::SetVolume(float lVolume, float rVolume)
{
    float AvgVolume;

    /* calculate left and right average for 2 ch */
    AvgVolume = GetAvgVolume(lVolume, rVolume);

    /* Update current Volume Level */
    mCurVolLevel_ = VolumeToLevel(AvgVolume);

    /* Convert the volume to the volume range supported in hardware */
    float ActualVolume = AvgVolume * MAX_VOL_RANGE;

    int32_t ret = mAudioRendererSink_->SetVolume(ActualVolume);
    if (ret) {
        MEDIA_ERR_LOG("Error Setting volume to Renderer!");
        return ret;
    }

    MEDIA_INFO_LOG("Setting Volume level:%d in AudioService and Volume:%0.2f in audioRenderer",
        (int32_t)mCurVolLevel_, ActualVolume);

    return 0;
}

int32_t VolumeControl::SetCurrentVolume()
{
    float Volume;

    if (mMaxVolLevel_ == 0) {
        MEDIA_ERR_LOG("mMaxVolLevel_ is zero!");
        return ERROR;
    }
    /* Get the volume in float */
    Volume = (mCurVolLevel_ / (float)mMaxVolLevel_) * MAX_VOL_RANGE;

    int32_t ret = mAudioRendererSink_->SetVolume(Volume);
    if (ret) {
        MEDIA_ERR_LOG("Error Setting Current volume!");
        return ret;
    }
    MEDIA_INFO_LOG("Initial volume set to %0.2f and level=%d", Volume, (int32_t)mCurVolLevel_);

    return 0;
}

int32_t VolumeControl::SaveVolume(float lVolume, float rVolume)
{
    float AvgVolume;

    /* calculate left and right average for 2 ch */
    AvgVolume = GetAvgVolume(lVolume, rVolume);

    /* Update current Volume Level */
    mCurVolLevel_ = VolumeToLevel(AvgVolume);

    if (mAudioSvcMgr_) {
        mAudioSvcMgr_->SetVolume(AudioSvcManager::AudioVolumeType::STREAM_MUSIC, (int32_t)mCurVolLevel_);
        MEDIA_INFO_LOG("Set the Current Volume level in AudioService to %d", (int32_t)mCurVolLevel_);
    } else {
        MEDIA_ERR_LOG("%s: mAudioSvcMgr_ is NULL!", __FUNCTION__);
    }
    return 0;
}
} // namespace Media
} // namespace OHOS
