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

#include "avcodec_info_native_mock.h"
#include "avformat_native_mock.h"
#include "avmemory_native_mock.h"
#include "surface_native_mock.h"
#include "media_errors.h"
#include "videoenc_native_mock.h"

namespace OHOS {
namespace Media {
VideoEncCallbackMock::VideoEncCallbackMock(std::shared_ptr<AVCodecCallbackMock> cb,
    std::weak_ptr<AVCodecVideoEncoder> vd)
    : mockCb_(cb), videoEnc_(vd)
{
}

void VideoEncCallbackMock::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    (void)errorType;
    if (mockCb_ != nullptr) {
        mockCb_->OnError(errorCode);
    }
}

void VideoEncCallbackMock::OnOutputFormatChanged(const Format &format)
{
    if (mockCb_ != nullptr) {
        auto formatMock = std::make_shared<AVFormatNativeMock>(format);
        mockCb_->OnStreamChanged(formatMock);
    }
}

void VideoEncCallbackMock::OnInputBufferAvailable(uint32_t index)
{
    if (mockCb_ != nullptr) {
        mockCb_->OnNeedInputData(index, nullptr);
    }
}

void VideoEncCallbackMock::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    auto videoEnc = videoEnc_.lock();
    if (mockCb_ != nullptr && videoEnc != nullptr) {
        std::shared_ptr<AVSharedMemory> mem = videoEnc->GetOutputBuffer(index);
        if (mem != nullptr) {
            std::shared_ptr<AVMemoryMock> memMock = std::make_shared<AVMemoryNativeMock>(mem);
            struct AVCodecBufferAttrMock bufferInfo;
            bufferInfo.pts = info.presentationTimeUs;
            bufferInfo.size = info.size;
            bufferInfo.offset = info.offset;
            bufferInfo.flags = flag;
            return mockCb_->OnNewOutputData(index, memMock, bufferInfo);
        }
    }
}

int32_t VideoEncNativeMock::SetCallback(std::shared_ptr<AVCodecCallbackMock> cb)
{
    if (cb != nullptr) {
        auto callback = std::make_shared<VideoEncCallbackMock>(cb, videoEnc_);
        if (videoEnc_ != nullptr && callback != nullptr) {
            return videoEnc_->SetCallback(callback);
        }
    }
    return MSERR_INVALID_OPERATION;
}

std::shared_ptr<SurfaceMock> VideoEncNativeMock::GetInputSurface()
{
    if (videoEnc_ != nullptr) {
        sptr<Surface> surface = videoEnc_->CreateInputSurface();
        if (surface != nullptr) {
            return std::make_shared<SurfaceNativeMock>(surface);
        }
    }
    return nullptr;
}

int32_t VideoEncNativeMock::Configure(std::shared_ptr<FormatMock> format)
{
    if (videoEnc_ != nullptr) {
        auto fmt = std::static_pointer_cast<AVFormatNativeMock>(format);
        return videoEnc_->Configure(fmt->GetFormat());
    }
    return MSERR_INVALID_OPERATION;
}

int32_t VideoEncNativeMock::Prepare()
{
    if (videoEnc_ != nullptr) {
        return videoEnc_->Prepare();
    }
    return MSERR_INVALID_OPERATION;
}

int32_t VideoEncNativeMock::Start()
{
    if (videoEnc_ != nullptr) {
        return videoEnc_->Start();
    }
    return MSERR_INVALID_OPERATION;
}

int32_t VideoEncNativeMock::Stop()
{
    if (videoEnc_ != nullptr) {
        return videoEnc_->Stop();
    }
    return MSERR_INVALID_OPERATION;
}

int32_t VideoEncNativeMock::Flush()
{
    if (videoEnc_ != nullptr) {
        return videoEnc_->Flush();
    }
    return MSERR_INVALID_OPERATION;
}

int32_t VideoEncNativeMock::Reset()
{
    if (videoEnc_ != nullptr) {
        return videoEnc_->Reset();
    }
    return MSERR_INVALID_OPERATION;
}

int32_t VideoEncNativeMock::Release()
{
    if (videoEnc_ != nullptr) {
        return videoEnc_->Release();
    }
    return MSERR_INVALID_OPERATION;
}

int32_t VideoEncNativeMock::NotifyEos()
{
    if (videoEnc_ != nullptr) {
        return videoEnc_->NotifyEos();
    }
    return MSERR_INVALID_OPERATION;
}

std::shared_ptr<FormatMock> VideoEncNativeMock::GetOutputMediaDescription()
{
    if (videoEnc_ != nullptr) {
        Format format;
        (void)videoEnc_->GetOutputFormat(format);
        return std::make_shared<AVFormatNativeMock>(format);
    }
    return nullptr;
}

int32_t VideoEncNativeMock::SetParameter(std::shared_ptr<FormatMock> format)
{
    if (videoEnc_ != nullptr) {
        auto fmt = std::static_pointer_cast<AVFormatNativeMock>(format);
        return videoEnc_->SetParameter(fmt->GetFormat());
    }
    return MSERR_INVALID_OPERATION;
}

int32_t VideoEncNativeMock::FreeOutputData(uint32_t index)
{
    if (videoEnc_ != nullptr) {
        return videoEnc_->ReleaseOutputBuffer(index);
    }
    return MSERR_INVALID_OPERATION;
}
}
}