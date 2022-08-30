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

#include "videoenc_capi_mock.h"
#include <iostream>
#include "avformat_capi_mock.h"
#include "avmemory_capi_mock.h"
#include "surface_capi_mock.h"
#include "media_errors.h"
#include "native_avmagic.h"
#include "native_avcodec_base.h"
#include "window.h"
#include "avcodec_video_encoder.h"


using namespace std;

namespace OHOS {
namespace Media {
std::mutex VideoEncCapiMock::mutex_;
std::map<OH_AVCodec *, std::shared_ptr<AVCodecCallbackMock>> VideoEncCapiMock::mockCbMap_;
void VideoEncCapiMock::OnError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb != nullptr) {
        mockCb->OnError(errorCode);
    }
}

void VideoEncCapiMock::OnStreamChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb != nullptr) {
        auto formatMock = std::make_shared<AVFormatCapiMock>(format);
        mockCb->OnStreamChanged(formatMock);
    }
}

void VideoEncCapiMock::OnNeedInputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb != nullptr) {
        mockCb->OnNeedInputData(index, nullptr);
    }
}

void VideoEncCapiMock::OnNewOutputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data,
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
        std::shared_ptr<AVMemoryMock> memMock = std::make_shared<AVMemoryCapiMock>(data);
        mockCb->OnNewOutputData(index, memMock, bufferInfo);
    }
}

std::shared_ptr<AVCodecCallbackMock> VideoEncCapiMock::GetCallback(OH_AVCodec *codec)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (mockCbMap_.find(codec) != mockCbMap_.end()) {
        return mockCbMap_.at(codec);
    }
    return nullptr;
}

void VideoEncCapiMock::SetCallback(OH_AVCodec *codec, std::shared_ptr<AVCodecCallbackMock> cb)
{
    std::lock_guard<std::mutex> lock(mutex_);
    mockCbMap_[codec] = cb;
}

void VideoEncCapiMock::DelCallback(OH_AVCodec *codec)
{
    auto it = mockCbMap_.find(codec);
    if (it != mockCbMap_.end()) {
        mockCbMap_.erase(it);
    }
}

int32_t VideoEncCapiMock::SetCallback(std::shared_ptr<AVCodecCallbackMock> cb)
{
    if (cb != nullptr && codec_ != nullptr) {
        SetCallback(codec_, cb);
        struct OH_AVCodecAsyncCallback callback;
        callback.onError = VideoEncCapiMock::OnError;
        callback.onStreamChanged = VideoEncCapiMock::OnStreamChanged;
        callback.onNeedInputData = VideoEncCapiMock::OnNeedInputData;
        callback.onNeedOutputData = VideoEncCapiMock::OnNewOutputData;
        return OH_VideoEncoder_SetCallback(codec_, callback, NULL);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
};

struct VEncObject : public OH_AVCodec {
    explicit VEncObject(const std::shared_ptr<AVCodecVideoEncoder> &encoder)
        : OH_AVCodec(AVMagic::MEDIA_MAGIC_VIDEO_ENCODER), videoEncoder_(encoder) {}
    ~VEncObject() = default;

    const std::shared_ptr<AVCodecVideoEncoder> videoEncoder_;
};

std::shared_ptr<SurfaceMock> VideoEncCapiMock::GetInputSurface()
{
    if (codec_ != nullptr) {
        OHNativeWindow *window;
        (void)OH_VideoEncoder_GetSurface(codec_, &window);
        if (window != nullptr) {
            return std::make_shared<SurfaceCapiMock>(window);
        }
    }
    return nullptr;
}

int32_t VideoEncCapiMock::Configure(std::shared_ptr<FormatMock> format)
{
    if (codec_ != nullptr && format != nullptr) {
        auto formatMock = std::static_pointer_cast<AVFormatCapiMock>(format);
        OH_AVFormat *avFormat = formatMock->GetFormat();
        if (avFormat != nullptr) {
            return OH_VideoEncoder_Configure(codec_, avFormat);
        } else {
            cout << "VideoEncCapiMock::Configure: avFormat is null" << endl;
        }
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoEncCapiMock::Prepare()
{
    if (codec_ != nullptr) {
        return OH_VideoEncoder_Prepare(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoEncCapiMock::Start()
{
    if (codec_ != nullptr) {
        return OH_VideoEncoder_Start(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoEncCapiMock::Stop()
{
    if (codec_ != nullptr) {
        return OH_VideoEncoder_Stop(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoEncCapiMock::Flush()
{
    if (codec_ != nullptr) {
        return OH_VideoEncoder_Flush(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoEncCapiMock::Reset()
{
    if (codec_ != nullptr) {
        return OH_VideoEncoder_Reset(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoEncCapiMock::Release()
{
    if (codec_ != nullptr) {
        return OH_VideoEncoder_Destroy(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoEncCapiMock::NotifyEos()
{
    if (codec_ != nullptr) {
        return OH_VideoEncoder_NotifyEndOfStream(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

std::shared_ptr<FormatMock> VideoEncCapiMock::GetOutputMediaDescription()
{
    if (codec_ != nullptr) {
        OH_AVFormat *format = OH_VideoEncoder_GetOutputDescription(codec_);
        return std::make_shared<AVFormatCapiMock>(format);
    }
    return nullptr;
}

int32_t VideoEncCapiMock::SetParameter(std::shared_ptr<FormatMock> format)
{
    if (codec_ != nullptr && format != nullptr) {
        auto formatMock = std::static_pointer_cast<AVFormatCapiMock>(format);
        return OH_VideoEncoder_SetParameter(codec_, formatMock->GetFormat());
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoEncCapiMock::FreeOutputData(uint32_t index)
{
    if (codec_ != nullptr) {
        return OH_VideoEncoder_FreeOutputData(codec_, index);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}
}
}