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

#include "recorder_data_source.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
constexpr int32_t KEY_IS_SYNC_FRAME = 1; // "is-sync-frame"
constexpr int32_t KEY_TIME_US = 2; // "timeUs"
constexpr int32_t SURFACE_QUEUE_SIZE = 3;
constexpr int32_t SURFACE_SIZE = 1024 * 1024; // 1M for surface
constexpr int32_t SURFACE_DEFAULT_HEIGHT = 1920; // 1M for surface
constexpr int32_t SURFACE_DEFAULT_WIDTH = 1080; // 1M for surface

RecorderDataSource::RecorderDataSource()
    : surface_(nullptr),
      frameAvailableCount_(0),
      acquireBuffer_(nullptr),
      fence_(-1),
      timestamp_(0),
      damage_ {0},
      started_(false)
{
}

RecorderDataSource::~RecorderDataSource() = default;

std::shared_ptr<OHOS::Surface> RecorderDataSource::GetSurface()
{
    if ((surface_ == nullptr) || (surface_.get() == nullptr)) {
        dataConSurface_ = Surface::CreateSurfaceAsConsumer();
        if (dataConSurface_ == nullptr) {
            return nullptr;
        }
        sptr<IBufferConsumerListener> listener(this);
        dataConSurface_->SetUserData("surface_width", std::to_string(SURFACE_DEFAULT_WIDTH));
        dataConSurface_->SetUserData("surface_height", std::to_string(SURFACE_DEFAULT_HEIGHT));
        dataConSurface_->SetQueueSize(SURFACE_QUEUE_SIZE);
        dataConSurface_->SetUserData("surface_size", std::to_string(SURFACE_SIZE));
        dataConSurface_->RegisterConsumerListener(listener);
        surface_.reset(dataConSurface_.GetRefPtr());
    }
    MEDIA_INFO_LOG("Get Recorder data Surface SUCCESS");
    auto refSurface = surface_;
    return refSurface;
}

void RecorderDataSource::OnBufferAvailable()
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

int32_t RecorderDataSource::Start()
{
    started_ = true;
    MEDIA_INFO_LOG("Start Recorder Video Source SUCCESS");
    return SUCCESS;
}

int32_t RecorderDataSource::AcquireBuffer(RecorderSourceBuffer &buffer, bool isBlocking)
{
    if (!started_) {
        return ERR_NOT_STARTED;
    }
    if (isBlocking) {
        std::unique_lock<std::mutex> lock(lock_);
        if (frameAvailableCount_ <= 0) {
            frameAvailableCondition_.wait(lock);
            if (!started_) {
                return SUCCESS;
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
    buffer.dataLen = acquireBuffer_->GetSize();
    int64_t valuets = 0;
    acquireBuffer_->GetInt64(KEY_TIME_US, valuets);
    buffer.timeStamp = valuets;

    int32_t value = 0;
    acquireBuffer_->GetInt32(KEY_IS_SYNC_FRAME, value);
    buffer.keyFrameFlag = (value == 1) ? true : false;
    return SUCCESS;
}

int32_t RecorderDataSource::ReleaseBuffer(RecorderSourceBuffer &buffer)
{
    surface_->ReleaseBuffer(acquireBuffer_, fence_);
    return SUCCESS;
}

int32_t RecorderDataSource::Stop()
{
    started_ = false;
    std::unique_lock<std::mutex> lock(lock_);
    frameAvailableCondition_.notify_all();

    if (dataConSurface_ != nullptr) {
        dataConSurface_->UnregisterConsumerListener();
        dataConSurface_ = nullptr;
        surface_ = nullptr;
    }
    return SUCCESS;
}

int32_t RecorderDataSource::Resume()
{
    return SUCCESS;
}

int32_t RecorderDataSource::Pause()
{
    return SUCCESS;
}

int32_t RecorderDataSource::Release()
{
    return SUCCESS;
}
}  // namespace Media
}  // namespace OHOS
