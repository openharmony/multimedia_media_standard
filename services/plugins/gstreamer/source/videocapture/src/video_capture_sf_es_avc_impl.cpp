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
#include "errors.h"
#include "scope_guard.h"
#include "securec.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoCaptureSfEsAvcImpl"};
}

namespace OHOS {
namespace Media {
static constexpr uint32_t MAX_SURFACE_BUFFER_SIZE = 3 * 1024 * 1024;
VideoCaptureSfEsAvcImpl::VideoCaptureSfEsAvcImpl()
{
}

VideoCaptureSfEsAvcImpl::~VideoCaptureSfEsAvcImpl()
{
}

std::shared_ptr<EsAvcCodecBuffer> VideoCaptureSfEsAvcImpl::DoGetCodecBuffer()
{
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer_ != nullptr, nullptr, "surface buffer is nullptr");

    uint32_t bufferSize = surfaceBuffer_->GetSize();
    CHECK_AND_RETURN_RET_LOG(bufferSize < MAX_SURFACE_BUFFER_SIZE, nullptr, "buffer size too long");

    ON_SCOPE_EXIT(0) { dataConSurface_->ReleaseBuffer(surfaceBuffer_, fence_); };

    gpointer buffer = surfaceBuffer_->GetVirAddr();
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, nullptr, "surface buffer address is invalid");

    std::vector<uint8_t> sps;
    std::vector<uint8_t> pps;
    uint32_t nalSize = 0;
    GetCodecData(reinterpret_cast<const uint8_t *>(buffer), bufferSize, sps, pps, nalSize);
    CHECK_AND_RETURN_RET_LOG(nalSize > 0 && sps.size() > 0 && pps.size() > 0, nullptr, "illegal codec buffer");

    // 11 is the length of AVCDecoderConfigurationRecord field except sps and pps
    uint32_t codecBufferSize = sps.size() + pps.size() + 11;
    GstBuffer *codec = gst_buffer_new_allocate(nullptr, codecBufferSize, nullptr);
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "no memory");
    GstMapInfo map;
    CHECK_AND_RETURN_RET_LOG(gst_buffer_map(codec, &map, GST_MAP_READ) == TRUE, nullptr, "gst_buffer_map fail");

    ON_SCOPE_EXIT(1) {
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

    std::shared_ptr<EsAvcCodecBuffer> codecBuffer = std::make_shared<EsAvcCodecBuffer>();
    CHECK_AND_RETURN_RET_LOG(codecBuffer != nullptr, nullptr, "no memory");
    codecBuffer->width = static_cast<uint32_t>(surfaceBuffer_->GetWidth());
    codecBuffer->height = static_cast<uint32_t>(surfaceBuffer_->GetHeight());
    codecBuffer->segmentStart = 0;
    codecBuffer->gstCodecBuffer = codec;
    codecData_ = (char *)buffer;
    codecDataSize_ = codecBufferSize;

    CANCEL_SCOPE_EXIT_GUARD(0);
    CANCEL_SCOPE_EXIT_GUARD(1);
    return codecBuffer;
}

std::shared_ptr<VideoFrameBuffer> VideoCaptureSfEsAvcImpl::DoGetFrameBuffer()
{
    if ((frameSequence_ == 0) && (codecData_ == nullptr)) {
        GetCodecBuffer();
    }

    if (frameSequence_ == 0) {
        return GetFirstBuffer();
    }

    uint32_t bufferSize = surfaceBuffer_->GetSize();
    CHECK_AND_RETURN_RET_LOG(bufferSize < MAX_SURFACE_BUFFER_SIZE, nullptr, "buffer size too long");

    ON_SCOPE_EXIT(0) { dataConSurface_->ReleaseBuffer(surfaceBuffer_, fence_); };

    gpointer buffer = surfaceBuffer_->GetVirAddr();
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, nullptr, "surface buffer address is invalid");

    GstBuffer *gstBuffer = gst_buffer_new_allocate(nullptr, bufferSize, nullptr);
    CHECK_AND_RETURN_RET_LOG(gstBuffer != nullptr, nullptr, "no memory");

    ON_SCOPE_EXIT(1) { gst_buffer_unref(gstBuffer); };

    gsize size = gst_buffer_fill(gstBuffer, 0, (char *)buffer, bufferSize);
    CHECK_AND_RETURN_RET_LOG(size == static_cast<gsize>(bufferSize), nullptr, "unkonwn error during gst_buffer_fill");

    std::shared_ptr<VideoFrameBuffer> frameBuffer = std::make_shared<VideoFrameBuffer>();
    frameBuffer->keyFrameFlag = 0;
    frameBuffer->timeStamp = 0;
    frameBuffer->gstBuffer = gstBuffer;
    frameBuffer->duration = 0;
    frameBuffer->size = bufferSize;
    frameSequence_++;

    CANCEL_SCOPE_EXIT_GUARD(1);
    return frameBuffer;
}

std::shared_ptr<VideoFrameBuffer> VideoCaptureSfEsAvcImpl::GetFirstBuffer()
{
    ON_SCOPE_EXIT(0) { dataConSurface_->ReleaseBuffer(surfaceBuffer_, fence_); };

    int32_t bufferSize = surfaceBuffer_->GetSize() - codecDataSize_;
    GstBuffer *gstBuffer = gst_buffer_new_allocate(nullptr, bufferSize, nullptr);
    CHECK_AND_RETURN_RET_LOG(gstBuffer != nullptr, nullptr, "no memory");

    ON_SCOPE_EXIT(1) { gst_buffer_unref(gstBuffer); };

    gsize size = gst_buffer_fill(gstBuffer, 0, codecData_ + codecDataSize_, bufferSize);
    CHECK_AND_RETURN_RET_LOG(size == static_cast<gsize>(bufferSize), nullptr, "unkonwn error during gst_buffer_fill");

    std::shared_ptr<VideoFrameBuffer> frameBuffer = std::make_shared<VideoFrameBuffer>();
    frameBuffer->keyFrameFlag = 0;
    frameBuffer->timeStamp = 0;
    frameBuffer->gstBuffer = gstBuffer;
    frameBuffer->duration = 0;
    frameBuffer->size = bufferSize;
    codecData_ = nullptr;
    frameSequence_++;

    CANCEL_SCOPE_EXIT_GUARD(1);
    return frameBuffer;
}

const uint8_t *VideoCaptureSfEsAvcImpl::FindNextNal(const uint8_t *start, const uint8_t *end, uint32_t &nalSize)
{
    CHECK_AND_RETURN_RET(start != nullptr && end != nullptr, nullptr);
    while (start <= end - 3) {
        // 0x000001 Nal
        if (start[0] == 0x00 && start[1] == 0x00 && start[2] == 0x01) {
            nalSize = 3;
            return start;
        }
        // 0x00000001 Nal
        if (start[0] == 0x00 && start[1] == 0x00 && start[2] == 0x00 && start[3] == 0x01) {
            nalSize = 4;
            return start;
        }
        start++;
    }
    return end;
}

void VideoCaptureSfEsAvcImpl::GetCodecData(const uint8_t *data, int32_t len,
    std::vector<uint8_t> &sps, std::vector<uint8_t> &pps, uint32_t &nalSize)
{
    CHECK_AND_RETURN(data != nullptr);
    const uint8_t *end = data + len;
    const uint8_t *pBegin = data;
    const uint8_t *pEnd = nullptr;
    while (pBegin < end) {
        pBegin = FindNextNal(pBegin, end, nalSize);
        if (pBegin == end) {
            break;
        }
        pBegin += nalSize;
        pEnd = FindNextNal(pBegin, end, nalSize);
        if (((*pBegin) & 0x1F) == 0x07) { // sps
            sps.assign(pBegin, pBegin + static_cast<int>(pEnd - pBegin));
        }
        if (((*pBegin) & 0x1F) == 0x08) { // pps
            pps.assign(pBegin, pBegin + static_cast<int>(pEnd - pBegin));
        }
        pBegin = pEnd;
    }
}
}  // namespace Media
}  // namespace OHOS
