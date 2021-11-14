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
#include <cmath>
#include "media_log.h"
#include "media_errors.h"
#include "graphic_common.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoCaptureSfmpl"};
    constexpr int32_t DEFAULT_SURFACE_QUEUE_SIZE = 6;
    constexpr int32_t DEFAULT_SURFACE_SIZE = 1024 * 1024;
    constexpr int32_t DEFAULT_VIDEO_WIDTH = 1920;
    constexpr int32_t DEFAULT_VIDEO_HEIGHT = 1080;
}

namespace OHOS {
namespace Media {
VideoCaptureSfImpl::VideoCaptureSfImpl()
    : videoWidth_(DEFAULT_VIDEO_WIDTH),
      videoHeight_(DEFAULT_VIDEO_HEIGHT),
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
    MEDIA_LOGI("videoWidth %{public}d, videoHeight %{public}d", videoWidth_, videoHeight_);
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
    return MSERR_OK;
}

int32_t VideoCaptureSfImpl::Pause()
{
    pauseTime_ = pts_;
    pauseCount_++;
    return MSERR_OK;
}

int32_t VideoCaptureSfImpl::Resume()
{
    resumeTime_ = pts_;
    if (resumeTime_ < pauseTime_) {
        MEDIA_LOGW("get wrong timestamp from HDI!");
    }

    persistTime_ = std::fabs(resumeTime_ - pauseTime_);

    totalPauseTime_ += persistTime_;

    MEDIA_LOGI("video capture has %{public}d times paused, persistTime: %{public}" PRIu64 ",totalPauseTime: %{public}"
    PRIu64 "", pauseCount_, persistTime_, totalPauseTime_);
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
    pauseTime_ = 0;
    resumeTime_ = 0;
    persistTime_ = 0;
    totalPauseTime_ = 0;
    pauseCount_ = 0;
    return MSERR_OK;
}

void VideoCaptureSfImpl::UnLock(bool start)
{
    std::unique_lock<std::mutex> lock(mutex_);
    resourceLock_ = start;
    bufferAvailableCondition_.notify_all();
    MEDIA_LOGI("Unlock any pending access to the resource: %{public}d", resourceLock_);
}

int32_t VideoCaptureSfImpl::SetVideoWidth(uint32_t width)
{
    videoWidth_ = width;
    return MSERR_OK;
}

int32_t VideoCaptureSfImpl::SetVideoHeight(uint32_t height)
{
    videoHeight_ = height;
    return MSERR_OK;
}

int32_t VideoCaptureSfImpl::SetStreamType(VideoStreamType streamType)
{
    streamTypeUnknown_ = false;
    streamType_ = streamType;
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
    if (bufferNumber_  == 0 && streamType_ == VIDEO_STREAM_TYPE_ES_AVC) {
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
    SurfaceError ret = dataConSurface_->SetUserData("video_width", std::to_string(videoWidth_));
    if (ret != SURFACE_ERROR_OK) {
        MEDIA_LOGW("set video width fail");
    }
    ret = dataConSurface_->SetUserData("video_height", std::to_string(videoHeight_));
    if (ret != SURFACE_ERROR_OK) {
        MEDIA_LOGW("set video height fail");
    }
    ret = dataConSurface_->SetQueueSize(DEFAULT_SURFACE_QUEUE_SIZE);
    if (ret != SURFACE_ERROR_OK) {
        MEDIA_LOGW("set queue size fail");
    }
    ret = dataConSurface_->SetUserData("surface_size", std::to_string(DEFAULT_SURFACE_SIZE));
    if (ret != SURFACE_ERROR_OK) {
        MEDIA_LOGW("set surface size fail");
    }
    ret = dataConSurface_->SetDefaultWidthAndHeight(videoWidth_, videoHeight_);
    if (ret != SURFACE_ERROR_OK) {
        MEDIA_LOGW("set surface width and height fail");
    }
}

int32_t VideoCaptureSfImpl::GetSufferExtraData()
{
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer_ != nullptr, MSERR_INVALID_OPERATION, "surfacebuffer is null");

    SurfaceError surfaceRet;

    surfaceRet = surfaceBuffer_->ExtraGet("dataSize", dataSize_);
    CHECK_AND_RETURN_RET_LOG(surfaceRet == SURFACE_ERROR_OK, MSERR_INVALID_OPERATION, "get dataSize fail");
    CHECK_AND_RETURN_RET_LOG(dataSize_ > 0, MSERR_INVALID_OPERATION, "illegal dataSize");
    surfaceRet = surfaceBuffer_->ExtraGet("timeStamp", pts_);
    CHECK_AND_RETURN_RET_LOG(surfaceRet == SURFACE_ERROR_OK, MSERR_INVALID_OPERATION, "get timeStamp fail");
    surfaceRet = surfaceBuffer_->ExtraGet("isKeyFrame", isCodecFrame_);
    CHECK_AND_RETURN_RET_LOG(surfaceRet == SURFACE_ERROR_OK, MSERR_INVALID_OPERATION, "get isKeyFrame fail");

    MEDIA_LOGI("surfaceBuffer extraData dataSize_: %{public}d, pts: (%{public}" PRId64 ")", dataSize_, pts_);
    MEDIA_LOGI("is this surfaceBuffer keyFrame ? : %{public}d", isCodecFrame_);

    if (pts_ < previousTimestamp_) {
        MEDIA_LOGW("get error timestamp from surfaceBuffer");
    }
    previousTimestamp_ = pts_;

    return MSERR_OK;
}

int32_t VideoCaptureSfImpl::AcquireSurfaceBuffer()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (!started_ || (dataConSurface_ == nullptr)) {
        return MSERR_INVALID_OPERATION;
    }

    bufferAvailableCondition_.wait(lock, [this]() { return bufferAvailableCount_ > 0 || resourceLock_; });
    if (resourceLock_) {
        MEDIA_LOGI("flush start / eos, skip acquire buffer");
        return MSERR_NO_MEMORY;
    }

    if (!started_ || (dataConSurface_ == nullptr)) {
        return MSERR_INVALID_OPERATION;
    }
    if (dataConSurface_->AcquireBuffer(surfaceBuffer_, fence_, timestamp_, damage_) != SURFACE_ERROR_OK) {
        return MSERR_UNKNOWN;
    }

    int32_t ret = GetSufferExtraData();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "get ExtraData fail");

    pts_ = pts_ - totalPauseTime_;
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
    // 从码流中或者从buffer中识别到底是es流还是yuv流
    // 但如果已经走到这里了，其实识别正确与否没办法了，只能报错然后重新启动
    // 可能后续会在次修改，识别nv12 nv21等一些同一类别下的流格式。
}
}  // namespace Media
}  // namespace OHOS
