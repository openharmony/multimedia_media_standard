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

#include "video_capture_sf_yuv_impl.h"
#include "media_log.h"
#include "media_errors.h"
#include "scope_guard.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoCaptureSfYuvmpl"};
    constexpr uint32_t MAX_SURFACE_BUFFER_SIZE = 10 * 1024 * 1024;
}

namespace OHOS {
namespace Media {
VideoCaptureSfYuvImpl::VideoCaptureSfYuvImpl()
{
}

VideoCaptureSfYuvImpl::~VideoCaptureSfYuvImpl()
{
}

std::shared_ptr<EsAvcCodecBuffer> VideoCaptureSfYuvImpl::DoGetCodecBuffer()
{
    return nullptr;
}

std::shared_ptr<VideoFrameBuffer> VideoCaptureSfYuvImpl::DoGetFrameBuffer()
{
    MEDIA_LOGD("enter yuv DoGetFrameBuffer");

    uint32_t bufferSize = static_cast<uint32_t>(dataSize_); // yuv size after encode
    CHECK_AND_RETURN_RET_LOG(bufferSize < MAX_SURFACE_BUFFER_SIZE, nullptr, "buffer size too long");

    ON_SCOPE_EXIT(0) { (void)dataConSurface_->ReleaseBuffer(surfaceBuffer_, fence_); };

    gpointer buffer = surfaceBuffer_->GetVirAddr();
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, nullptr, "surface buffer address is invalid");

    GstBuffer *gstBuffer = gst_buffer_new_allocate(nullptr, bufferSize, nullptr);
    CHECK_AND_RETURN_RET_LOG(gstBuffer != nullptr, nullptr, "no memory");

    ON_SCOPE_EXIT(1) { gst_buffer_unref(gstBuffer); };

    gsize size = gst_buffer_fill(gstBuffer, 0, (char *)buffer, bufferSize);
    CHECK_AND_RETURN_RET_LOG(size == static_cast<gsize>(bufferSize), nullptr, "unkonwn error during gst_buffer_fill");

    std::shared_ptr<VideoFrameBuffer> frameBuffer = std::make_shared<VideoFrameBuffer>();
    frameBuffer->keyFrameFlag = 0;
    frameBuffer->timeStamp = static_cast<uint64_t>(pts_); // yuv timestamp from camera
    frameBuffer->gstBuffer = gstBuffer;
    frameBuffer->size = static_cast<uint64_t>(bufferSize);

    CANCEL_SCOPE_EXIT_GUARD(1);
    return frameBuffer;
}
} // namespace Media
} // namespace OHOS

