/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "videodec_native_mock.h"
#include "avcodec_info_native_mock.h"
#include "avformat_native_mock.h"
#include "avmemory_native_mock.h"
#include "surface_native_mock.h"
#include "media_errors.h"

namespace OHOS {
namespace Media {
VideoDecCallbackMock::VideoDecCallbackMock(std::shared_ptr<AVCodecCallbackMock> cb,
    std::weak_ptr<AVCodecVideoDecoder> vd)
    : mockCb_(cb), videoDec_(vd)
{
}

void VideoDecCallbackMock::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    (void)errorType;
    if (mockCb_ != nullptr) {
        mockCb_->OnError(errorCode);
    }
}

void VideoDecCallbackMock::OnOutputFormatChanged(const Format &format)
{
    if (mockCb_ != nullptr) {
        auto formatMock = std::make_shared<AVFormatNativeMock>(format);
        mockCb_->OnStreamChanged(formatMock);
    }
}

void VideoDecCallbackMock::OnInputBufferAvailable(uint32_t index)
{
    auto videoDec = videoDec_.lock();
    if (mockCb_ != nullptr && videoDec != nullptr) {
        std::shared_ptr<AVSharedMemory> mem = videoDec->GetInputBuffer(index);
        if (mem != nullptr) {
            std::shared_ptr<AVMemoryMock> memMock = std::make_shared<AVMemoryNativeMock>(mem);
            mockCb_->OnNeedInputData(index, memMock);
        }
    }
}

void VideoDecCallbackMock::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    if (mockCb_ != nullptr) {
        struct AVCodecBufferAttrMock bufferInfo;
        bufferInfo.pts = info.presentationTimeUs;
        bufferInfo.size = info.size;
        bufferInfo.offset = info.offset;
        bufferInfo.flags = flag;
        return mockCb_->OnNewOutputData(index, nullptr, bufferInfo);
    }
}

int32_t VideoDecNativeMock::SetCallback(std::shared_ptr<AVCodecCallbackMock> cb)
{
    if (cb != nullptr) {
        auto callback = std::make_shared<VideoDecCallbackMock>(cb, videoDec_);
        if (videoDec_ != nullptr && callback != nullptr) {
            return videoDec_->SetCallback(callback);
        }
    }
    return MSERR_INVALID_OPERATION;
}

int32_t VideoDecNativeMock::SetOutputSurface(std::shared_ptr<SurfaceMock> surface)
{
    if (surface != nullptr) {
        auto decSurface = std::static_pointer_cast<SurfaceNativeMock>(surface);
        sptr<Surface> nativeSurface = decSurface->GetSurface();
        if (videoDec_ != nullptr && nativeSurface != nullptr) {
            return videoDec_->SetOutputSurface(nativeSurface);
        }
    }
    return MSERR_INVALID_OPERATION;
}

int32_t VideoDecNativeMock::Configure(std::shared_ptr<FormatMock> format)
{
    if (videoDec_ != nullptr && format != nullptr) {
        auto fmt = std::static_pointer_cast<AVFormatNativeMock>(format);
        return videoDec_->Configure(fmt->GetFormat());
    }
    return MSERR_INVALID_OPERATION;
}

int32_t VideoDecNativeMock::Prepare()
{
    if (videoDec_ != nullptr) {
        return videoDec_->Prepare();
    }
    return MSERR_INVALID_OPERATION;
}

int32_t VideoDecNativeMock::Start()
{
    if (videoDec_ != nullptr) {
        return videoDec_->Start();
    }
    return MSERR_INVALID_OPERATION;
}

int32_t VideoDecNativeMock::Stop()
{
    if (videoDec_ != nullptr) {
        return videoDec_->Stop();
    }
    return MSERR_INVALID_OPERATION;
}

int32_t VideoDecNativeMock::Flush()
{
    if (videoDec_ != nullptr) {
        return videoDec_->Flush();
    }
    return MSERR_INVALID_OPERATION;
}

int32_t VideoDecNativeMock::Reset()
{
    if (videoDec_ != nullptr) {
        return videoDec_->Reset();
    }
    return MSERR_INVALID_OPERATION;
}

int32_t VideoDecNativeMock::Release()
{
    if (videoDec_ != nullptr) {
        return videoDec_->Release();
    }
    return MSERR_INVALID_OPERATION;
}

std::shared_ptr<FormatMock> VideoDecNativeMock::GetOutputMediaDescription()
{
    if (videoDec_ != nullptr) {
        Format format;
        (void)videoDec_->GetOutputFormat(format);
        return std::make_shared<AVFormatNativeMock>(format);
    }
    return nullptr;
}

int32_t VideoDecNativeMock::SetParameter(std::shared_ptr<FormatMock> format)
{
    if (videoDec_ != nullptr && format != nullptr) {
        auto fmt = std::static_pointer_cast<AVFormatNativeMock>(format);
        return videoDec_->SetParameter(fmt->GetFormat());
    }
    return MSERR_INVALID_OPERATION;
}

int32_t VideoDecNativeMock::PushInputData(uint32_t index, AVCodecBufferAttrMock &attr)
{
    if (videoDec_ != nullptr) {
        AVCodecBufferInfo info;
        info.presentationTimeUs = attr.pts;
        info.size = attr.size;
        info.offset = attr.offset;
        AVCodecBufferFlag flags = static_cast<AVCodecBufferFlag>(attr.flags);
        return videoDec_->QueueInputBuffer(index, info, flags);
    }
    return MSERR_INVALID_OPERATION;
}

int32_t VideoDecNativeMock::RenderOutputData(uint32_t index)
{
    if (videoDec_ != nullptr) {
        return videoDec_->ReleaseOutputBuffer(index, true);
    }
    return MSERR_INVALID_OPERATION;
}

int32_t VideoDecNativeMock::FreeOutputData(uint32_t index)
{
    if (videoDec_ != nullptr) {
        return videoDec_->ReleaseOutputBuffer(index, false);
    }
    return MSERR_INVALID_OPERATION;
}
}
}