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

#include "audiodec_native_mock.h"
#include "avformat_native_mock.h"
#include "avmemory_native_mock.h"
#include "media_errors.h"

namespace OHOS {
namespace Media {
AudioDecCallbackMock::AudioDecCallbackMock(std::shared_ptr<AVCodecCallbackMock> cb,
    std::weak_ptr<AVCodecAudioDecoder> ad)
    : mockCb_(cb), audioDec_(ad)
{
}

void AudioDecCallbackMock::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    (void)errorType;
    if (mockCb_ != nullptr) {
        mockCb_->OnError(errorCode);
    }
}

void AudioDecCallbackMock::OnOutputFormatChanged(const Format &format)
{
    if (mockCb_ != nullptr) {
        auto formatMock = std::make_shared<AVFormatNativeMock>(format);
        mockCb_->OnStreamChanged(formatMock);
    }
}

void AudioDecCallbackMock::OnInputBufferAvailable(uint32_t index)
{
    auto audioDec = audioDec_.lock();
    if (mockCb_ != nullptr && audioDec != nullptr) {
        std::shared_ptr<AVSharedMemory> mem = audioDec->GetInputBuffer(index);
        if (mem != nullptr) {
            std::shared_ptr<AVMemoryMock> memMock = std::make_shared<AVMemoryNativeMock>(mem);
            mockCb_->OnNeedInputData(index, memMock);
        }
    }
}

void AudioDecCallbackMock::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    auto audioDec = audioDec_.lock();
    if (mockCb_ != nullptr && audioDec != nullptr) {
        std::shared_ptr<AVSharedMemory> mem = audioDec->GetOutputBuffer(index);
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

int32_t AudioDecNativeMock::SetCallback(std::shared_ptr<AVCodecCallbackMock> cb)
{
    if (cb != nullptr) {
        auto callback = std::make_shared<AudioDecCallbackMock>(cb, audioDec_);
        if (audioDec_ != nullptr && callback != nullptr) {
            return audioDec_->SetCallback(callback);
        }
    }
    return MSERR_INVALID_OPERATION;
}

int32_t AudioDecNativeMock::Configure(std::shared_ptr<FormatMock> format)
{
    if (audioDec_ != nullptr && format != nullptr) {
        auto fmt = std::static_pointer_cast<AVFormatNativeMock>(format);
        return audioDec_->Configure(fmt->GetFormat());
    }
    return MSERR_INVALID_OPERATION;
}

int32_t AudioDecNativeMock::Prepare()
{
    if (audioDec_ != nullptr) {
        return audioDec_->Prepare();
    }
    return MSERR_INVALID_OPERATION;
}

int32_t AudioDecNativeMock::Start()
{
    if (audioDec_ != nullptr) {
        return audioDec_->Start();
    }
    return MSERR_INVALID_OPERATION;
}

int32_t AudioDecNativeMock::Stop()
{
    if (audioDec_ != nullptr) {
        return audioDec_->Stop();
    }
    return MSERR_INVALID_OPERATION;
}

int32_t AudioDecNativeMock::Flush()
{
    if (audioDec_ != nullptr) {
        return audioDec_->Flush();
    }
    return MSERR_INVALID_OPERATION;
}

int32_t AudioDecNativeMock::Reset()
{
    if (audioDec_ != nullptr) {
        return audioDec_->Reset();
    }
    return MSERR_INVALID_OPERATION;
}

int32_t AudioDecNativeMock::Release()
{
    if (audioDec_ != nullptr) {
        return audioDec_->Release();
    }
    return MSERR_INVALID_OPERATION;
}

std::shared_ptr<FormatMock> AudioDecNativeMock::GetOutputMediaDescription()
{
    if (audioDec_ != nullptr) {
        Format format;
        (void)audioDec_->GetOutputFormat(format);
        return std::make_shared<AVFormatNativeMock>(format);
    }
    return nullptr;
}

int32_t AudioDecNativeMock::SetParameter(std::shared_ptr<FormatMock> format)
{
    if (audioDec_ != nullptr && format != nullptr) {
        auto fmt = std::static_pointer_cast<AVFormatNativeMock>(format);
        return audioDec_->SetParameter(fmt->GetFormat());
    }
    return MSERR_INVALID_OPERATION;
}

int32_t AudioDecNativeMock::PushInputData(uint32_t index, AVCodecBufferAttrMock &attr)
{
    if (audioDec_ != nullptr) {
        AVCodecBufferInfo info;
        info.presentationTimeUs = attr.pts;
        info.size = attr.size;
        info.offset = attr.offset;
        AVCodecBufferFlag flags = static_cast<AVCodecBufferFlag>(attr.flags);
        return audioDec_->QueueInputBuffer(index, info, flags);
    }
    return MSERR_INVALID_OPERATION;
}

int32_t AudioDecNativeMock::FreeOutputData(uint32_t index)
{
    if (audioDec_ != nullptr) {
        return audioDec_->ReleaseOutputBuffer(index);
    }
    return MSERR_INVALID_OPERATION;
}
}
}