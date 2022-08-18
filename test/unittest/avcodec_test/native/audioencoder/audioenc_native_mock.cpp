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

#include "audioenc_native_mock.h"
#include "avformat_native_mock.h"
#include "avmemory_native_mock.h"
#include "media_errors.h"

namespace OHOS {
namespace Media {
AudioEncCallbackMock::AudioEncCallbackMock(std::shared_ptr<AVCodecCallbackMock> cb,
    std::weak_ptr<AVCodecAudioEncoder> ad)
    : mockCb_(cb), audioEnc_(ad)
{
}

void AudioEncCallbackMock::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    (void)errorType;
    if (mockCb_ != nullptr) {
        mockCb_->OnError(errorCode);
    }
}

void AudioEncCallbackMock::OnOutputFormatChanged(const Format &format)
{
    if (mockCb_ != nullptr) {
        auto formatMock = std::make_shared<AVFormatNativeMock>(format);
        mockCb_->OnStreamChanged(formatMock);
    }
}

void AudioEncCallbackMock::OnInputBufferAvailable(uint32_t index)
{
    auto audioEnc = audioEnc_.lock();
    if (mockCb_ != nullptr && audioEnc != nullptr) {
        std::shared_ptr<AVSharedMemory> mem = audioEnc->GetInputBuffer(index);
        if (mem != nullptr) {
            std::shared_ptr<AVMemoryMock> memMock = std::make_shared<AVMemoryNativeMock>(mem);
            mockCb_->OnNeedInputData(index, memMock);
        }
    }
}

void AudioEncCallbackMock::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    auto audioEnc = audioEnc_.lock();
    if (mockCb_ != nullptr && audioEnc != nullptr) {
        std::shared_ptr<AVSharedMemory> mem = audioEnc->GetOutputBuffer(index);
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

int32_t AudioEncNativeMock::SetCallback(std::shared_ptr<AVCodecCallbackMock> cb)
{
    if (cb != nullptr) {
        auto callback = std::make_shared<AudioEncCallbackMock>(cb, audioEnc_);
        if (audioEnc_ != nullptr && callback != nullptr) {
            return audioEnc_->SetCallback(callback);
        }
    }
    return MSERR_INVALID_OPERATION;
}

int32_t AudioEncNativeMock::Configure(std::shared_ptr<FormatMock> format)
{
    if (audioEnc_ != nullptr && format != nullptr) {
        auto fmt = std::static_pointer_cast<AVFormatNativeMock>(format);
        return audioEnc_->Configure(fmt->GetFormat());
    }
    return MSERR_INVALID_OPERATION;
}

int32_t AudioEncNativeMock::Prepare()
{
    if (audioEnc_ != nullptr) {
        return audioEnc_->Prepare();
    }
    return MSERR_INVALID_OPERATION;
}

int32_t AudioEncNativeMock::Start()
{
    if (audioEnc_ != nullptr) {
        return audioEnc_->Start();
    }
    return MSERR_INVALID_OPERATION;
}

int32_t AudioEncNativeMock::Stop()
{
    if (audioEnc_ != nullptr) {
        return audioEnc_->Stop();
    }
    return MSERR_INVALID_OPERATION;
}

int32_t AudioEncNativeMock::Flush()
{
    if (audioEnc_ != nullptr) {
        return audioEnc_->Flush();
    }
    return MSERR_INVALID_OPERATION;
}

int32_t AudioEncNativeMock::Reset()
{
    if (audioEnc_ != nullptr) {
        return audioEnc_->Reset();
    }
    return MSERR_INVALID_OPERATION;
}

int32_t AudioEncNativeMock::Release()
{
    if (audioEnc_ != nullptr) {
        return audioEnc_->Release();
    }
    return MSERR_INVALID_OPERATION;
}

std::shared_ptr<FormatMock> AudioEncNativeMock::GetOutputMediaDescription()
{
    if (audioEnc_ != nullptr) {
        Format format;
        (void)audioEnc_->GetOutputFormat(format);
        return std::make_shared<AVFormatNativeMock>(format);
    }
    return nullptr;
}

int32_t AudioEncNativeMock::SetParameter(std::shared_ptr<FormatMock> format)
{
    if (audioEnc_ != nullptr && format != nullptr) {
        auto fmt = std::static_pointer_cast<AVFormatNativeMock>(format);
        return audioEnc_->SetParameter(fmt->GetFormat());
    }
    return MSERR_INVALID_OPERATION;
}

int32_t AudioEncNativeMock::PushInputData(uint32_t index, AVCodecBufferAttrMock &attr)
{
    if (audioEnc_ != nullptr) {
        AVCodecBufferInfo info;
        info.presentationTimeUs = attr.pts;
        info.size = attr.size;
        info.offset = attr.offset;
        AVCodecBufferFlag flags = static_cast<AVCodecBufferFlag>(attr.flags);
        return audioEnc_->QueueInputBuffer(index, info, flags);
    }
    return MSERR_INVALID_OPERATION;
}

int32_t AudioEncNativeMock::FreeOutputData(uint32_t index)
{
    if (audioEnc_ != nullptr) {
        return audioEnc_->ReleaseOutputBuffer(index);
    }
    return MSERR_INVALID_OPERATION;
}
}
}