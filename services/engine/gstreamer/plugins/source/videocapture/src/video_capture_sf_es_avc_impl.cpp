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

#include "video_capture_sf_es_avc_impl.h"
#include "media_log.h"
#include "media_errors.h"
#include "scope_guard.h"
#include "securec.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoCaptureSfEsAvcImpl"};
    constexpr uint32_t MAX_SURFACE_BUFFER_SIZE = 10 * 1024 * 1024;
}

namespace OHOS {
namespace Media {
VideoCaptureSfEsAvcImpl::VideoCaptureSfEsAvcImpl()
{
}

VideoCaptureSfEsAvcImpl::~VideoCaptureSfEsAvcImpl()
{
}

GstBuffer *VideoCaptureSfEsAvcImpl::AVCDecoderConfiguration(std::vector<uint8_t> &sps,
    std::vector<uint8_t> &pps)
{
    uint32_t codecBufferSize = sps.size() + pps.size() + 11;
    GstBuffer *codec = gst_buffer_new_allocate(nullptr, codecBufferSize, nullptr);
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "no memory");
    GstMapInfo map;
    CHECK_AND_RETURN_RET_LOG(gst_buffer_map(codec, &map, GST_MAP_READ) == TRUE, nullptr, "gst_buffer_map fail");

    ON_SCOPE_EXIT(0) {
        gst_buffer_unmap(codec, &map);
        gst_buffer_unref(codec);
    };

    uint32_t offset = 0;
    map.data[offset++] = 0x01; // configurationVersion
    map.data[offset++] = sps[1]; // AVCProfileIndication
    map.data[offset++] = sps[2]; // profile_compatibility
    map.data[offset++] = sps[3]; // AVCLevelIndication
    map.data[offset++] = 0xff; // lengthSizeMinusOne

    map.data[offset++] = 0xe0 | 0x01; // numOfSequenceParameterSets
    map.data[offset++] = (sps.size() >> 8) & 0xff; // sequenceParameterSetLength high 8 bits
    map.data[offset++] = sps.size() & 0xff; // sequenceParameterSetLength low 8 bits
    // sequenceParameterSetNALUnit
    CHECK_AND_RETURN_RET_LOG(memcpy_s(map.data + offset, codecBufferSize - offset, &sps[0], sps.size()) == EOK,
                             nullptr, "memcpy_s fail");
    offset += sps.size();

    map.data[offset++] = 0x01; // numOfPictureParameterSets
    map.data[offset++] = (pps.size() >> 8) & 0xff; // pictureParameterSetLength  high 8 bits
    map.data[offset++] = pps.size() & 0xff; // pictureParameterSetLength  low 8 bits
    // pictureParameterSetNALUnit
    CHECK_AND_RETURN_RET_LOG(memcpy_s(map.data + offset, codecBufferSize - offset, &pps[0], pps.size()) == EOK,
                             nullptr, "memcpy_s fail");
    CANCEL_SCOPE_EXIT_GUARD(0);

    return codec;
}


std::shared_ptr<EsAvcCodecBuffer> VideoCaptureSfEsAvcImpl::DoGetCodecBuffer()
{
    MEDIA_LOGI("enter DoGetCodecBuffer");
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer_ != nullptr, nullptr, "surface buffer is nullptr");

    uint32_t bufferSize = static_cast<uint32_t>(dataSize_);
    CHECK_AND_RETURN_RET_LOG(bufferSize < MAX_SURFACE_BUFFER_SIZE, nullptr, "buffer size too long");

    ON_SCOPE_EXIT(0) { (void)dataConSurface_->ReleaseBuffer(surfaceBuffer_, fence_); };

    gpointer buffer = surfaceBuffer_->GetVirAddr();
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, nullptr, "surface buffer address is invalid");

    std::vector<uint8_t> sps;
    std::vector<uint8_t> pps;
    std::vector<uint8_t> sei;
    GetCodecData(reinterpret_cast<const uint8_t *>(buffer), bufferSize, sps, pps, sei);
    CHECK_AND_RETURN_RET_LOG(nalSize_ > 0 && sps.size() > 0 && pps.size() > 0 && sei.size() > 0,
        nullptr, "illegal codec buffer");

    GstBuffer *configBuffer = AVCDecoderConfiguration(sps, pps);
    CHECK_AND_RETURN_RET_LOG(configBuffer != nullptr, nullptr, "AVCDecoderConfiguration failed");

    std::shared_ptr<EsAvcCodecBuffer> codecBuffer = std::make_shared<EsAvcCodecBuffer>();
    CHECK_AND_RETURN_RET_LOG(codecBuffer != nullptr, nullptr, "no memory");
    codecBuffer->width = videoWidth_;
    codecBuffer->height = videoHeight_;
    codecBuffer->segmentStart = 0;
    codecBuffer->gstCodecBuffer = configBuffer;
    codecData_ = (char *)buffer;
    codecDataSize_ = nalSize_ * 3 + sps.size() + pps.size() + sei.size();

    CANCEL_SCOPE_EXIT_GUARD(0);
    return codecBuffer;
}

std::shared_ptr<VideoFrameBuffer> VideoCaptureSfEsAvcImpl::DoGetFrameBuffer()
{
    if ((frameSequence_ == 0) && (codecData_ == nullptr)) {
        GetCodecBuffer();
    }

    if (frameSequence_ == 0) {
        return GetIDRFrame();
    }

    uint32_t bufferSize = static_cast<uint32_t>(dataSize_);
    CHECK_AND_RETURN_RET_LOG(bufferSize < MAX_SURFACE_BUFFER_SIZE, nullptr, "buffer size too long");

    ON_SCOPE_EXIT(0) { (void)dataConSurface_->ReleaseBuffer(surfaceBuffer_, fence_); };

    gpointer buffer = surfaceBuffer_->GetVirAddr();
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, nullptr, "surface buffer address is invalid");

    if (isCodecFrame_ == 1) {
        buffer = (char *)buffer + codecDataSize_;
    }

    uint32_t frameSize = bufferSize - nalSize_;
    // there is two kind of nal head. four byte 0x00000001 or three byte 0x000001
    // standard es_avc stream should begin with frame size
    // change the nal head to frame size.
    if (nalSize_ == 4) { // 0x00000001
        ((char *)buffer)[0] = (char)((frameSize >> 24) & 0xff);
        ((char *)buffer)[1] = (char)((frameSize >> 16) & 0xff);
        ((char *)buffer)[2] = (char)((frameSize >> 8) & 0xff);
        ((char *)buffer)[3] = (char)(frameSize & 0xff);
    } else { // 0x000001
        ((char *)buffer)[0] = (char)((frameSize >> 16) & 0xff);
        ((char *)buffer)[1] = (char)((frameSize >> 8) & 0xff);
        ((char *)buffer)[2] = (char)(frameSize & 0xff);
    }

    GstBuffer *gstBuffer = gst_buffer_new_allocate(nullptr, bufferSize, nullptr);
    CHECK_AND_RETURN_RET_LOG(gstBuffer != nullptr, nullptr, "no memory");

    ON_SCOPE_EXIT(1) { gst_buffer_unref(gstBuffer); };

    gsize size = gst_buffer_fill(gstBuffer, 0, (char *)buffer, bufferSize);
    CHECK_AND_RETURN_RET_LOG(size == static_cast<gsize>(bufferSize), nullptr, "unkonwn error during gst_buffer_fill");

    std::shared_ptr<VideoFrameBuffer> frameBuffer = std::make_shared<VideoFrameBuffer>();
    frameBuffer->keyFrameFlag = 0;
    frameBuffer->timeStamp = static_cast<uint64_t>(pts_);
    frameBuffer->gstBuffer = gstBuffer;
    frameBuffer->size = static_cast<uint64_t>(bufferSize);

    CANCEL_SCOPE_EXIT_GUARD(1);
    return frameBuffer;
}

std::shared_ptr<VideoFrameBuffer> VideoCaptureSfEsAvcImpl::GetIDRFrame()
{
    ON_SCOPE_EXIT(0) { 
        (void)dataConSurface_->ReleaseBuffer(surfaceBuffer_, fence_); 
    };

    uint32_t bufferSize = static_cast<uint32_t>(dataSize_) - codecDataSize_;
    GstBuffer *gstBuffer = gst_buffer_new_allocate(nullptr, bufferSize, nullptr);
    CHECK_AND_RETURN_RET_LOG(gstBuffer != nullptr, nullptr, "no memory");

    ON_SCOPE_EXIT(1) { gst_buffer_unref(gstBuffer); };

    // there is two kind of nal head. four byte 0x00000001 or three byte 0x000001
    // standard es_avc stream should begin with frame size
    // change the nal head to frame size.
    uint32_t frameSize = bufferSize - nalSize_;
    if (nalSize_ == 4) { // 0x00000001
        codecData_[codecDataSize_] = (char)((frameSize >> 24) & 0xff);
        codecData_[codecDataSize_ + 1] = (char)((frameSize >> 16) & 0xff);
        codecData_[codecDataSize_ + 2] = (char)((frameSize >> 8) & 0xff);
        codecData_[codecDataSize_ + 3] = (char)(frameSize & 0xff);
    } else { // 0x000001
        codecData_[codecDataSize_] = (char)((frameSize >> 16) & 0xff);
        codecData_[codecDataSize_ + 1] = (char)((frameSize >> 8) & 0xff);
        codecData_[codecDataSize_ + 2] = (char)(frameSize & 0xff);
    }

    gsize size = gst_buffer_fill(gstBuffer, 0, codecData_ + codecDataSize_, bufferSize);
    CHECK_AND_RETURN_RET_LOG(size == static_cast<gsize>(bufferSize), nullptr, "unkonwn error during gst_buffer_fill");

    std::shared_ptr<VideoFrameBuffer> frameBuffer = std::make_shared<VideoFrameBuffer>();
    frameBuffer->keyFrameFlag = 0;
    frameBuffer->timeStamp = static_cast<uint64_t>(pts_);
    frameBuffer->gstBuffer = gstBuffer;
    frameBuffer->size = static_cast<uint64_t>(bufferSize);
    codecData_ = nullptr;
    frameSequence_++;

    CANCEL_SCOPE_EXIT_GUARD(1);
    return frameBuffer;
}

const uint8_t *VideoCaptureSfEsAvcImpl::FindNextNal(const uint8_t *start, const uint8_t *end, uint32_t &nalSize)
{
    CHECK_AND_RETURN_RET(start != nullptr && end != nullptr, nullptr);
    // there is two kind of nal head. four byte 0x00000001 or three byte 0x000001
    while (start <= end - 3) {
        if (start[0] == 0x00 && start[1] == 0x00 && start[2] == 0x01) {
            nalSize = 3; // 0x000001 Nal
            return start;
        }
        if (start[0] == 0x00 && start[1] == 0x00 && start[2] == 0x00 && start[3] == 0x01) {
            nalSize = 4; // 0x00000001 Nal
            return start;
        }
        start++;
    }
    return end;
}

void VideoCaptureSfEsAvcImpl::GetCodecData(const uint8_t *data, int32_t len,
    std::vector<uint8_t> &sps, std::vector<uint8_t> &pps, std::vector<uint8_t> &sei)
{
    CHECK_AND_RETURN(data != nullptr);
    const uint8_t *end = data + len - 1;
    const uint8_t *pBegin = data;
    const uint8_t *pEnd = nullptr;
    while (pBegin < end) {
        pBegin = FindNextNal(pBegin, end, nalSize_);
        if (pBegin == end) {
            break;
        }
        pBegin += nalSize_;
        pEnd = FindNextNal(pBegin, end, nalSize_);
        if (((*pBegin) & 0x1F) == 0x07) { // sps
            sps.assign(pBegin, pBegin + static_cast<int>(pEnd - pBegin));
        }
        if (((*pBegin) & 0x1F) == 0x08) { // pps
            pps.assign(pBegin, pBegin + static_cast<int>(pEnd - pBegin));
        }
        if (((*pBegin) & 0x1F) == 0x06) { // sei
            sei.assign(pBegin, pBegin + static_cast<int>(pEnd - pBegin));
        }
        pBegin = pEnd;
    }
}
}  // namespace Media
}  // namespace OHOS
