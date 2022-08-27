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

#include "audioenc_capi_mock.h"
#include "avformat_capi_mock.h"
#include "avmemory_capi_mock.h"
#include "native_avcodec_base.h"

namespace OHOS {
namespace Media {
std::mutex AudioEncCapiMock::mutex_;
std::map<OH_AVCodec *, std::shared_ptr<AVCodecCallbackMock>> AudioEncCapiMock::mockCbMap_;

void AudioEncCapiMock::OnError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb != nullptr) {
        mockCb->OnError(errorCode);
    }
}

void AudioEncCapiMock::OnStreamChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb != nullptr) {
        auto formatMock = std::make_shared<AVFormatCapiMock>(format);
        mockCb->OnStreamChanged(formatMock);
    }
}

void AudioEncCapiMock::OnNeedInputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb != nullptr) {
        std::shared_ptr<AVMemoryMock> memMock = std::make_shared<AVMemoryCapiMock>(data);
        mockCb->OnNeedInputData(index, memMock);
    }
}

void AudioEncCapiMock::OnNewOutputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data,
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

std::shared_ptr<AVCodecCallbackMock> AudioEncCapiMock::GetCallback(OH_AVCodec *codec)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (mockCbMap_.find(codec) != mockCbMap_.end()) {
        return mockCbMap_.at(codec);
    }
    return nullptr;
}

void AudioEncCapiMock::SetCallback(OH_AVCodec *codec, std::shared_ptr<AVCodecCallbackMock> cb)
{
    std::lock_guard<std::mutex> lock(mutex_);
    mockCbMap_[codec] = cb;
}

void AudioEncCapiMock::DelCallback(OH_AVCodec *codec)
{
    auto it = mockCbMap_.find(codec);
    if (it != mockCbMap_.end()) {
        mockCbMap_.erase(it);
    }
}

int32_t AudioEncCapiMock::SetCallback(std::shared_ptr<AVCodecCallbackMock> cb)
{
    if (cb != nullptr && codec_ != nullptr) {
        SetCallback(codec_, cb);
        struct OH_AVCodecAsyncCallback callback;
        callback.onError = AudioEncCapiMock::OnError;
        callback.onStreamChanged = AudioEncCapiMock::OnStreamChanged;
        callback.onNeedInputData = AudioEncCapiMock::OnNeedInputData;
        callback.onNeedOutputData = AudioEncCapiMock::OnNewOutputData;
        return OH_AudioEncoder_SetCallback(codec_, callback, NULL);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t AudioEncCapiMock::Configure(std::shared_ptr<FormatMock> format)
{
    if (codec_ != nullptr && format != nullptr) {
        auto formatMock = std::static_pointer_cast<AVFormatCapiMock>(format);
        OH_AVFormat *avFormat = formatMock->GetFormat();
        if (avFormat != nullptr) {
            return OH_AudioEncoder_Configure(codec_, avFormat);
        }
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t AudioEncCapiMock::Prepare()
{
    if (codec_ != nullptr) {
        return OH_AudioEncoder_Prepare(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t AudioEncCapiMock::Start()
{
    if (codec_ != nullptr) {
        return OH_AudioEncoder_Start(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t AudioEncCapiMock::Stop()
{
    if (codec_ != nullptr) {
        return OH_AudioEncoder_Stop(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t AudioEncCapiMock::Flush()
{
    if (codec_ != nullptr) {
        return OH_AudioEncoder_Flush(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t AudioEncCapiMock::Reset()
{
    if (codec_ != nullptr) {
        return OH_AudioEncoder_Reset(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t AudioEncCapiMock::Release()
{
    if (codec_ != nullptr) {
        DelCallback(codec_);
        return OH_AudioEncoder_Destroy(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

std::shared_ptr<FormatMock> AudioEncCapiMock::GetOutputMediaDescription()
{
    if (codec_ != nullptr) {
        OH_AVFormat *format = OH_AudioEncoder_GetOutputDescription(codec_);
        return std::make_shared<AVFormatCapiMock>(format);
    }
    return nullptr;
}

int32_t AudioEncCapiMock::SetParameter(std::shared_ptr<FormatMock> format)
{
    if (codec_ != nullptr && format != nullptr) {
        auto formatMock = std::static_pointer_cast<AVFormatCapiMock>(format);
        return OH_AudioEncoder_SetParameter(codec_, formatMock->GetFormat());
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t AudioEncCapiMock::PushInputData(uint32_t index, AVCodecBufferAttrMock &attr)
{
    if (codec_ != nullptr) {
        OH_AVCodecBufferAttr info;
        info.pts = attr.pts;
        info.size = attr.size;
        info.offset = attr.offset;
        info.flags = attr.flags;
        return OH_AudioEncoder_PushInputData(codec_, index, info);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t AudioEncCapiMock::FreeOutputData(uint32_t index)
{
    if (codec_ != nullptr) {
        return OH_AudioEncoder_FreeOutputData(codec_, index);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}
}
}
