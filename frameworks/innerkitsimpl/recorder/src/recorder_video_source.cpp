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

#include "recorder_video_source.h"

#include "media_errors.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
constexpr int32_t KEY_IS_SYNC_FRAME = 1; // "is-sync-frame"
constexpr int32_t KEY_TIME_US = 2; // "timeUs"
constexpr int32_t SURFACE_QUEUE_SIZE = 3;
constexpr int32_t SURFACE_SIZE = 1024 * 1024; // 1M for surface

RecorderVideoSource::RecorderVideoSource()
    : surface_(nullptr),
      frameAvailableCount_(0),
      acquireBuffer_(nullptr),
      fence_(-1),
      timestamp_(0),
      damage_ {0},
      started_(false)
{
}

RecorderVideoSource::~RecorderVideoSource() = default;

std::shared_ptr<OHOS::Surface> RecorderVideoSource::GetSurface()
{
    if ((surface_ == nullptr) || (surface_.get() == nullptr)) {
        videoConSurface_ = Surface::CreateSurfaceAsConsumer();
        if (videoConSurface_ == nullptr) {
            return nullptr;
        }

        sptr<IBufferConsumerListener> listener(this);
        videoConSurface_->RegisterConsumerListener(listener);
        surface_.reset(videoConSurface_.GetRefPtr());
    }
    MEDIA_INFO_LOG("Get Recorder Surface SUCCESS");
    auto refSurface = surface_;
    return refSurface;
}

int32_t RecorderVideoSource::SetSurfaceSize(int32_t width, int32_t height)
{
    Surface *surface = GetSurface().get();
    if (surface == nullptr) {
        MEDIA_ERR_LOG("surface is NULL");
        return ERROR;
    }
    MEDIA_INFO_LOG("Get Recorder Surface SUCCESS");
    surface->SetUserData("surface_width", std::to_string(width));
    surface->SetUserData("surface_height", std::to_string(height));
    surface->SetQueueSize(SURFACE_QUEUE_SIZE);
    surface->SetUserData("surface_size", std::to_string(SURFACE_SIZE));
    return SUCCESS;
}

void RecorderVideoSource::OnBufferAvailable()
{
    if (surface_ == nullptr) {
        MEDIA_ERR_LOG("surface is NULL");
        return;
    }

    if (!started_) {
        MEDIA_ERR_LOG("Recorder source is not started");
        SurfaceError ret = surface_->AcquireBuffer(acquireBuffer_, fence_, timestamp_, damage_);
        if (ret != SURFACE_ERROR_OK) {
            MEDIA_ERR_LOG("Acquire buffer failed.");
            return;
        }
        surface_->ReleaseBuffer(acquireBuffer_, fence_);
        return;
    }
    std::unique_lock<std::mutex> lock(lock_);
    if (frameAvailableCount_ == 0) {
        frameAvailableCondition_.notify_one();
    }
    frameAvailableCount_++;
}

int32_t RecorderVideoSource::Start()
{
    started_ = true;
    MEDIA_INFO_LOG("Start Recorder Video Source SUCCESS");
    return SUCCESS;
}

int32_t RecorderVideoSource::AcquireBuffer(RecorderSourceBuffer &buffer, bool isBlocking)
{
    if (!started_) {
        return ERR_NOT_STARTED;
    }
    if (isBlocking) {
        std::unique_lock<std::mutex> lock(lock_);
        if (frameAvailableCount_ <= 0) {
            frameAvailableCondition_.wait(lock);
            if (!started_) {
                return ERR_READ_BUFFER;
            }
        }
        frameAvailableCount_--;
    }
    if (surface_ == nullptr) {
        MEDIA_ERR_LOG("surface is NULL");
        return ERR_READ_BUFFER;
    }
    SurfaceError ret = surface_->AcquireBuffer(acquireBuffer_, fence_, timestamp_, damage_);
    if (ret != SURFACE_ERROR_OK) {
        MEDIA_ERR_LOG("Acquire buffer failed.");
        return ERR_READ_BUFFER;
    }
    void *pBase = acquireBuffer_->GetVirAddr();
    if (pBase == nullptr) {
        MEDIA_ERR_LOG("GetVirAddr pBase is nullptr");
        return ERR_READ_BUFFER;
    }
    buffer.dataAddr = (uint8_t *)pBase;
    buffer.dataLen = stoi(surface_->GetUserData("surface_buffer_size"));
    int64_t valuets = 0;
    acquireBuffer_->GetInt64(KEY_TIME_US, valuets);
    buffer.timeStamp = valuets;

    int32_t value = 0;
    acquireBuffer_->GetInt32(KEY_IS_SYNC_FRAME, value);
    buffer.keyFrameFlag = (value == 1) ? true : false;
    return SUCCESS;
}

int32_t RecorderVideoSource::ReleaseBuffer(RecorderSourceBuffer &buffer)
{
    surface_->ReleaseBuffer(acquireBuffer_, fence_);
    return SUCCESS;
}

int32_t RecorderVideoSource::Stop()
{
    started_ = false;
    std::unique_lock<std::mutex> lock(lock_);
    frameAvailableCondition_.notify_all();

    if (videoConSurface_ != nullptr) {
        videoConSurface_->UnregisterConsumerListener();
        videoConSurface_ = nullptr;
        surface_ = nullptr;
    }
    return SUCCESS;
}

int32_t RecorderVideoSource::Resume()
{
    return SUCCESS;
}

int32_t RecorderVideoSource::Pause()
{
    return SUCCESS;
}

int32_t RecorderVideoSource::Release()
{
    return SUCCESS;
}
}  // namespace Media
}  // namespace OHOS
