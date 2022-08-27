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

#include "audiodec_capi_mock.h"
#include "avformat_capi_mock.h"
#include "avmemory_capi_mock.h"
#include "native_avcodec_base.h"

namespace OHOS {
namespace Media {
std::mutex AudioDecCapiMock::mutex_;
std::map<OH_AVCodec *, std::shared_ptr<AVCodecCallbackMock>> AudioDecCapiMock::mockCbMap_;

void AudioDecCapiMock::OnError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb != nullptr) {
        mockCb->OnError(errorCode);
    }
}

void AudioDecCapiMock::OnStreamChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb != nullptr) {
        auto formatMock = std::make_shared<AVFormatCapiMock>(format);
        mockCb->OnStreamChanged(formatMock);
    }
}

void AudioDecCapiMock::OnNeedInputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb != nullptr) {
        std::shared_ptr<AVMemoryMock> memMock = std::make_shared<AVMemoryCapiMock>(data);
        mockCb->OnNeedInputData(index, memMock);
    }
}

void AudioDecCapiMock::OnNewOutputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data,
    OH_AVCodecBufferAttr *attr, void *userData)
{
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb != nullptr) {
        std::shared_ptr<AVMemoryMock> memMock = std::make_shared<AVMemoryCapiMock>(data);
        struct AVCodecBufferAttrMock bufferInfo;
        bufferInfo.pts = attr->pts;
        bufferInfo.size = attr->size;
        bufferInfo.offset = attr->offset;
        bufferInfo.flags = attr->flags;
        mockCb->OnNewOutputData(index, memMock, bufferInfo);
    }
}

std::shared_ptr<AVCodecCallbackMock> AudioDecCapiMock::GetCallback(OH_AVCodec *codec)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (mockCbMap_.find(codec) != mockCbMap_.end()) {
        return mockCbMap_.at(codec);
    }
    return nullptr;
}

void AudioDecCapiMock::SetCallback(OH_AVCodec *codec, std::shared_ptr<AVCodecCallbackMock> cb)
{
    std::lock_guard<std::mutex> lock(mutex_);
    mockCbMap_[codec] = cb;
}

void AudioDecCapiMock::DelCallback(OH_AVCodec *codec)
{
    auto it = mockCbMap_.find(codec);
    if (it != mockCbMap_.end()) {
        mockCbMap_.erase(it);
    }
}

int32_t AudioDecCapiMock::SetCallback(std::shared_ptr<AVCodecCallbackMock> cb)
{
    if (cb != nullptr && codec_ != nullptr) {
        SetCallback(codec_, cb);
        struct OH_AVCodecAsyncCallback callback;
        callback.onError = AudioDecCapiMock::OnError;
        callback.onStreamChanged = AudioDecCapiMock::OnStreamChanged;
        callback.onNeedInputData = AudioDecCapiMock::OnNeedInputData;
        callback.onNeedOutputData = AudioDecCapiMock::OnNewOutputData;
        return OH_AudioDecoder_SetCallback(codec_, callback, NULL);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t AudioDecCapiMock::Configure(std::shared_ptr<FormatMock> format)
{
    if (codec_ != nullptr && format != nullptr) {
        auto formatMock = std::static_pointer_cast<AVFormatCapiMock>(format);
        OH_AVFormat *avFormat = formatMock->GetFormat();
        if (avFormat != nullptr) {
            return OH_AudioDecoder_Configure(codec_, avFormat);
        }
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t AudioDecCapiMock::Prepare()
{
    if (codec_ != nullptr) {
        return OH_AudioDecoder_Prepare(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t AudioDecCapiMock::Start()
{
    if (codec_ != nullptr) {
        return OH_AudioDecoder_Start(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t AudioDecCapiMock::Stop()
{
    if (codec_ != nullptr) {
        return OH_AudioDecoder_Stop(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t AudioDecCapiMock::Flush()
{
    if (codec_ != nullptr) {
        return OH_AudioDecoder_Flush(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t AudioDecCapiMock::Reset()
{
    if (codec_ != nullptr) {
        return OH_AudioDecoder_Reset(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t AudioDecCapiMock::Release()
{
    if (codec_ != nullptr) {
        DelCallback(codec_);
        return OH_AudioDecoder_Destroy(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

std::shared_ptr<FormatMock> AudioDecCapiMock::GetOutputMediaDescription()
{
    if (codec_ != nullptr) {
        OH_AVFormat *format = OH_AudioDecoder_GetOutputDescription(codec_);
        return std::make_shared<AVFormatCapiMock>(format);
    }
    return nullptr;
}

int32_t AudioDecCapiMock::SetParameter(std::shared_ptr<FormatMock> format)
{
    if (codec_ != nullptr && format != nullptr) {
        auto formatMock = std::static_pointer_cast<AVFormatCapiMock>(format);
        return OH_AudioDecoder_SetParameter(codec_, formatMock->GetFormat());
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t AudioDecCapiMock::PushInputData(uint32_t index, AVCodecBufferAttrMock &attr)
{
    if (codec_ != nullptr) {
        OH_AVCodecBufferAttr info;
        info.pts = attr.pts;
        info.size = attr.size;
        info.offset = attr.offset;
        info.flags = attr.flags;
        return OH_AudioDecoder_PushInputData(codec_, index, info);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t AudioDecCapiMock::FreeOutputData(uint32_t index)
{
    if (codec_ != nullptr) {
        return OH_AudioDecoder_FreeOutputData(codec_, index);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}
}
}
