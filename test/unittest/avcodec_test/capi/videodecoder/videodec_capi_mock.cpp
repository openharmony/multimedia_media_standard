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

#include "videodec_capi_mock.h"
#include <iostream>
#include "avformat_capi_mock.h"
#include "avmemory_capi_mock.h"
#include "surface_capi_mock.h"
#include "native_avcodec_base.h"
#include "native_avmagic.h"
#include "window.h"
#include "avcodec_video_decoder.h"
using namespace std;

namespace OHOS {
namespace Media {
std::mutex VideoDecCapiMock::mutex_;
std::map<OH_AVCodec *, std::shared_ptr<AVCodecCallbackMock>> VideoDecCapiMock::mockCbMap_;
void VideoDecCapiMock::OnError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb != nullptr) {
        mockCb->OnError(errorCode);
    }
}

void VideoDecCapiMock::OnStreamChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb != nullptr) {
        auto formatMock = std::make_shared<AVFormatCapiMock>(format);
        mockCb->OnStreamChanged(formatMock);
    }
}

void VideoDecCapiMock::OnNeedInputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb != nullptr) {
        std::shared_ptr<AVMemoryMock> memMock = std::make_shared<AVMemoryCapiMock>(data);
        mockCb->OnNeedInputData(index, memMock);
    }
}

void VideoDecCapiMock::OnNewOutputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data,
    OH_AVCodecBufferAttr *attr, void *userData)
{
    (void)data;
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb != nullptr) {
        struct AVCodecBufferAttrMock bufferInfo;
        bufferInfo.pts = attr->pts;
        bufferInfo.size = attr->size;
        bufferInfo.offset = attr->offset;
        bufferInfo.flags = attr->flags;
        mockCb->OnNewOutputData(index, nullptr, bufferInfo);
    }
}

std::shared_ptr<AVCodecCallbackMock> VideoDecCapiMock::GetCallback(OH_AVCodec *codec)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (mockCbMap_.find(codec) != mockCbMap_.end()) {
        return mockCbMap_.at(codec);
    }
    return nullptr;
}

void VideoDecCapiMock::SetCallback(OH_AVCodec *codec, std::shared_ptr<AVCodecCallbackMock> cb)
{
    std::lock_guard<std::mutex> lock(mutex_);
    mockCbMap_[codec] = cb;
}

void VideoDecCapiMock::DelCallback(OH_AVCodec *codec)
{
    auto it = mockCbMap_.find(codec);
    if (it != mockCbMap_.end()) {
        mockCbMap_.erase(it);
    }
}

int32_t VideoDecCapiMock::SetCallback(std::shared_ptr<AVCodecCallbackMock> cb)
{
    if (cb != nullptr && codec_ != nullptr) {
        SetCallback(codec_, cb);
        struct OH_AVCodecAsyncCallback callback;
        callback.onError = VideoDecCapiMock::OnError;
        callback.onStreamChanged = VideoDecCapiMock::OnStreamChanged;
        callback.onNeedInputData = VideoDecCapiMock::OnNeedInputData;
        callback.onNeedOutputData = VideoDecCapiMock::OnNewOutputData;
        return OH_VideoDecoder_SetCallback(codec_, callback, NULL);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

struct VDecObject : public OH_AVCodec {
    explicit VDecObject(const std::shared_ptr<AVCodecVideoDecoder> &decoder)
        : OH_AVCodec(AVMagic::MEDIA_MAGIC_VIDEO_DECODER), videoDecoder_(decoder) {}
    ~VDecObject() = default;

    const std::shared_ptr<AVCodecVideoDecoder> videoDecoder_;
};

int32_t VideoDecCapiMock::SetOutputSurface(std::shared_ptr<SurfaceMock> surface)
{
    if (codec_ != nullptr && surface != nullptr) {
        auto surfaceMock = std::static_pointer_cast<SurfaceCapiMock>(surface);
        OHNativeWindow *nativeWindow  = surfaceMock->GetSurface();
        if (nativeWindow != nullptr) {
            return OH_VideoDecoder_SetSurface(codec_, nativeWindow);
        }
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoDecCapiMock::Configure(std::shared_ptr<FormatMock> format)
{
    if (codec_ != nullptr && format != nullptr) {
        auto formatMock = std::static_pointer_cast<AVFormatCapiMock>(format);
        OH_AVFormat *avFormat = formatMock->GetFormat();
        if (avFormat != nullptr) {
            return OH_VideoDecoder_Configure(codec_, avFormat);
        }
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoDecCapiMock::Prepare()
{
    if (codec_ != nullptr) {
        return OH_VideoDecoder_Prepare(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoDecCapiMock::Start()
{
    if (codec_ != nullptr) {
        return OH_VideoDecoder_Start(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoDecCapiMock::Stop()
{
    if (codec_ != nullptr) {
        return OH_VideoDecoder_Stop(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoDecCapiMock::Flush()
{
    if (codec_ != nullptr) {
        return OH_VideoDecoder_Flush(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoDecCapiMock::Reset()
{
    if (codec_ != nullptr) {
        return OH_VideoDecoder_Reset(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoDecCapiMock::Release()
{
    if (codec_ != nullptr) {
        DelCallback(codec_);
        return OH_VideoDecoder_Destroy(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

std::shared_ptr<FormatMock> VideoDecCapiMock::GetOutputMediaDescription()
{
    if (codec_ != nullptr) {
        OH_AVFormat *format = OH_VideoDecoder_GetOutputDescription(codec_);
        return std::make_shared<AVFormatCapiMock>(format);
    }
    return nullptr;
}

int32_t VideoDecCapiMock::SetParameter(std::shared_ptr<FormatMock> format)
{
    if (codec_ != nullptr && format != nullptr) {
        auto formatMock = std::static_pointer_cast<AVFormatCapiMock>(format);
        return OH_VideoDecoder_SetParameter(codec_, formatMock->GetFormat());
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoDecCapiMock::PushInputData(uint32_t index, AVCodecBufferAttrMock &attr)
{
    if (codec_ != nullptr) {
        OH_AVCodecBufferAttr info;
        info.pts = attr.pts;
        info.size = attr.size;
        info.offset = attr.offset;
        info.flags = attr.flags;
        return OH_VideoDecoder_PushInputData(codec_, index, info);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoDecCapiMock::RenderOutputData(uint32_t index)
{
    if (codec_ != nullptr) {
        return OH_VideoDecoder_RenderOutputData(codec_, index);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoDecCapiMock::FreeOutputData(uint32_t index)
{
    if (codec_ != nullptr) {
        return OH_VideoDecoder_FreeOutputData(codec_, index);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}
}
}