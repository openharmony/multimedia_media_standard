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

#include "video_capture_sf_impl.h"
#include <map>
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoCaptureSfmpl"};
    constexpr int32_t DEFAULT_SURFACE_QUEUE_SIZE = 6;
    constexpr int32_t DEFAULT_SURFACE_SIZE = 1024 * 1024;
    constexpr int32_t DEFAULT_SURFACE_WIDTH = 1920;
    constexpr int32_t DEFAULT_SURFACE_HEIGHT = 1080;
}

namespace OHOS {
namespace Media {
VideoCaptureSfImpl::VideoCaptureSfImpl()
    : surfaceWidth_(DEFAULT_SURFACE_WIDTH),
      surfaceHeight_(DEFAULT_SURFACE_HEIGHT),
      fence_(-1),
      bufferAvailableCount_(0),
      timestamp_(0),
      damage_ {0},
      surfaceBuffer_(nullptr),
      started_(false),
      paused_(false),
      streamType_(VIDEO_STREAM_TYPE_UNKNOWN),
      streamTypeUnknown_(true),
      dataConSurface_(nullptr),
      producerSurface_(nullptr)
{
}

VideoCaptureSfImpl::~VideoCaptureSfImpl()
{
    (void)Stop();
}

int32_t VideoCaptureSfImpl::Prepare()
{
    printf("surfaceWidth_: %d, surfaceHeight_: %d", surfaceWidth_, surfaceHeight_);

    sptr<Surface> consumerSurface = Surface::CreateSurfaceAsConsumer();
    CHECK_AND_RETURN_RET_LOG(consumerSurface != nullptr, MSERR_NO_MEMORY, "create surface fail");

    sptr<IBufferConsumerListener> listenerProxy = new (std::nothrow) ConsumerListenerProxy(*this);
    CHECK_AND_RETURN_RET_LOG(listenerProxy != nullptr, MSERR_NO_MEMORY, "create consumer listener fail");

    if (consumerSurface->RegisterConsumerListener(listenerProxy) != SURFACE_ERROR_OK) {
        MEDIA_LOGW("register consumer listener fail");
    }

    sptr<IBufferProducer> producer = consumerSurface->GetProducer();
    CHECK_AND_RETURN_RET_LOG(producer != nullptr, MSERR_NO_MEMORY, "get producer fail");
    sptr<Surface> producerSurface = Surface::CreateSurfaceAsProducer(producer);
    CHECK_AND_RETURN_RET_LOG(producerSurface != nullptr, MSERR_NO_MEMORY, "get producer fail");

    dataConSurface_ = consumerSurface;
    producerSurface_ = producerSurface;

    SetSurfaceUserData();
    return MSERR_OK;
}

int32_t VideoCaptureSfImpl::Start()
{
    std::unique_lock<std::mutex> lock(mutex_);
    started_.store(true);
    startedCondition_.notify_all();
    return MSERR_OK;
}

int32_t VideoCaptureSfImpl::Pause()
{
    return MSERR_OK;
}

int32_t VideoCaptureSfImpl::Resume()
{
    return MSERR_OK;
}

int32_t VideoCaptureSfImpl::Stop()
{
    std::unique_lock<std::mutex> lock(mutex_);
    started_.store(false);
    if (bufferAvailableCount_ == 0) {
        bufferAvailableCount_++;
    }
    bufferAvailableCondition_.notify_all();
    if (dataConSurface_ != nullptr) {
        if (dataConSurface_->UnregisterConsumerListener() != SURFACE_ERROR_OK) {
            MEDIA_LOGW("deregister consumer listener fail");
        }
        dataConSurface_ = nullptr;
        producerSurface_ = nullptr;
    }
    return MSERR_OK;
}

int32_t VideoCaptureSfImpl::SetSurfaceWidth(uint32_t width)
{
    surfaceWidth_ = width;
    return MSERR_OK;
}

int32_t VideoCaptureSfImpl::SetSurfaceHeight(uint32_t height)
{
    surfaceHeight_ = height;
    return MSERR_OK;
}

sptr<Surface> VideoCaptureSfImpl::GetSurface()
{
    CHECK_AND_RETURN_RET_LOG(producerSurface_ != nullptr, nullptr, "surface not created");
    return producerSurface_;
}

std::shared_ptr<EsAvcCodecBuffer> VideoCaptureSfImpl::GetCodecBuffer()
{
    if (AcquireSurfaceBuffer() == MSERR_OK) {
        if (streamTypeUnknown_) {
            ProbeStreamType();
        }
        return DoGetCodecBuffer();
    }

    return nullptr;
}

std::shared_ptr<VideoFrameBuffer> VideoCaptureSfImpl::GetFrameBufferInner()
{
    if (streamTypeUnknown_) {
        ProbeStreamType();
    }
    bufferNumber_++;
    return DoGetFrameBuffer();
}

std::shared_ptr<VideoFrameBuffer> VideoCaptureSfImpl::GetFrameBuffer()
{
    if (bufferNumber_  == 0) {
        return GetFrameBufferInner();
    } else {
        if (AcquireSurfaceBuffer() == MSERR_OK) {
            return GetFrameBufferInner();
        }
    }
    return nullptr;
}

void VideoCaptureSfImpl::SetSurfaceUserData()
{
    SurfaceError ret = dataConSurface_->SetUserData("surface_width", std::to_string(surfaceWidth_));
    if (ret != SURFACE_ERROR_OK) {
        MEDIA_LOGW("set surface width fail");
    }
    ret = dataConSurface_->SetUserData("surface_height", std::to_string(surfaceHeight_));
    if (ret != SURFACE_ERROR_OK) {
        MEDIA_LOGW("set surface height fail");
    }
    ret = dataConSurface_->SetQueueSize(DEFAULT_SURFACE_QUEUE_SIZE);
    if (ret != SURFACE_ERROR_OK) {
        MEDIA_LOGW("set queue size fail");
    }
    ret = dataConSurface_->SetUserData("surface_size", std::to_string(DEFAULT_SURFACE_SIZE));
    if (ret != SURFACE_ERROR_OK) {
        MEDIA_LOGW("set surface size fail");
    }
}

void VideoCaptureSfImpl::GetSufferExtraData()
{
    CHECK_AND_RETURN_LOG(surfaceBuffer_ != nullptr, "surfacebuffer is null");

    surfaceBuffer_->ExtraGet("dataSize", dataSize_);
    surfaceBuffer_->ExtraGet("timeStamp", pts_);
    surfaceBuffer_->ExtraGet("isKeyFrame", isCodecFrame_);

    MEDIA_LOGI("surfaceBuffer extraData dataSize_: %{public}d, pts: (%{public}" PRId64 ")", dataSize_, pts_);
    MEDIA_LOGI("is this surfaceBuffer keyFrame ? : %{public}d", isCodecFrame_);
}

int32_t VideoCaptureSfImpl::AcquireSurfaceBuffer()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (!started_ || (dataConSurface_ == nullptr)) {
        return MSERR_INVALID_OPERATION;
    }
    bufferAvailableCondition_.wait(lock, [this]() { return bufferAvailableCount_ > 0; });
    if (!started_ || (dataConSurface_ == nullptr)) {
        return MSERR_INVALID_OPERATION;
    }
    if (dataConSurface_->AcquireBuffer(surfaceBuffer_, fence_, timestamp_, damage_) != SURFACE_ERROR_OK) {
        return MSERR_UNKNOWN;
    }

    GetSufferExtraData();

    bufferAvailableCount_--;
    return MSERR_OK;
}

void VideoCaptureSfImpl::ConsumerListenerProxy::OnBufferAvailable()
{
    return owner_.OnBufferAvailable();
}

void VideoCaptureSfImpl::OnBufferAvailable()
{
    if (dataConSurface_ == nullptr) {
        return;
    }
    std::unique_lock<std::mutex> lock(mutex_);
    if (bufferAvailableCount_ == 0) {
        bufferAvailableCondition_.notify_all();
    }
    bufferAvailableCount_++;
}

void VideoCaptureSfImpl::ProbeStreamType()
{
    streamTypeUnknown_ = false;
    streamType_ = VIDEO_STREAM_TYPE_ES_AVC;
}
}  // namespace Media
}  // namespace OHOS
