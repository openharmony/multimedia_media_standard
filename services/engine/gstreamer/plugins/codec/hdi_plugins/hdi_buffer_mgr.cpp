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

#include "hdi_buffer_mgr.h"
#include <hdf_base.h>
#include "hdi_codec_util.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "HdiBufferMgr"};
}

namespace OHOS {
namespace Media {
HdiBufferMgr::HdiBufferMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}
 
HdiBufferMgr::~HdiBufferMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t HdiBufferMgr::Start()
{
    MEDIA_LOGD("Enter Start");
    std::unique_lock<std::mutex> lock(mutex_);
    isStart_ = true;
    isFlushed_ = false;
    return GST_CODEC_OK;
}

void HdiBufferMgr::Init(CodecComponentType *handle, int32_t index, const CompVerInfo &verInfo)
{
    MEDIA_LOGD("Enter Init");
    handle_ = handle;
    verInfo_ = verInfo;
    InitParam(mPortDef_, verInfo_);
    mPortIndex_ = index;
    mPortDef_.nPortIndex = (uint32_t)index;
}

std::shared_ptr<HdiBufferWrap> HdiBufferMgr::GetCodecBuffer(GstBuffer *buffer)
{
    MEDIA_LOGD("Enter GetCodecBuffer");
    std::shared_ptr<HdiBufferWrap> codecBuffer = nullptr;
    GstBufferTypeMeta *bufferType = gst_buffer_get_buffer_type_meta(buffer);
    CHECK_AND_RETURN_RET_LOG(bufferType != nullptr, nullptr, "bufferType is nullptr");
    for (auto iter = availableBuffers_.begin(); iter != availableBuffers_.end(); ++iter) {
        if (*iter != nullptr) {
            if ((*iter)->buf == bufferType->buf) {
                codecBuffer = *iter;
                codingBuffers_.push_back(std::make_pair(codecBuffer, buffer));
                gst_buffer_ref(buffer);
                (void)availableBuffers_.erase(iter);
                UpdateCodecMeta(bufferType, codecBuffer);
                codecBuffer->hdiBuffer.pts = (int64_t)(GST_BUFFER_PTS(buffer));
                break;
            }
        }
    }
    return codecBuffer;
}

void HdiBufferMgr::UpdateCodecMeta(GstBufferTypeMeta *bufferType, std::shared_ptr<HdiBufferWrap> &codecBuffer)
{
    MEDIA_LOGD("Enter UpdateCodecMeta");
    CHECK_AND_RETURN_LOG(codecBuffer != nullptr, "bufferType is nullptr");
    CHECK_AND_RETURN_LOG(bufferType != nullptr, "bufferType is nullptr");
    codecBuffer->hdiBuffer.allocLen = bufferType->totalSize;
    codecBuffer->hdiBuffer.offset = bufferType->offset;
    codecBuffer->hdiBuffer.filledLen = bufferType->length;
    codecBuffer->hdiBuffer.fenceFd = bufferType->fenceFd;
    codecBuffer->hdiBuffer.type = bufferType->memFlag == FLAGS_READ_ONLY ? READ_ONLY_TYPE : READ_WRITE_TYPE;
}

std::vector<std::shared_ptr<HdiBufferWrap>> HdiBufferMgr::PreUseAshareMems(std::vector<GstBuffer *> &buffers)
{
    MEDIA_LOGD("Enter PreUseAshareMems");
    std::vector<std::shared_ptr<HdiBufferWrap>> preBuffers;
    auto ret = HdiGetParameter(handle_, OMX_IndexParamPortDefinition, mPortDef_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, preBuffers, "HdiGetParameter failed");
    CHECK_AND_RETURN_RET_LOG(buffers.size() == mPortDef_.nBufferCountActual, preBuffers, "BufferNum error");
    for (auto buffer : buffers) {
        CHECK_AND_RETURN_RET_LOG(buffer != nullptr, preBuffers, "buffer is nullptr");
        GstBufferTypeMeta *bufferType = gst_buffer_get_buffer_type_meta(buffer);
        CHECK_AND_RETURN_RET_LOG(bufferType != nullptr, preBuffers, "bufferType is nullptr");
        std::shared_ptr<HdiBufferWrap> codecBuffer = std::make_shared<HdiBufferWrap>();
        codecBuffer->buf = bufferType->buf;
        codecBuffer->hdiBuffer.size = sizeof(OmxCodecBuffer);
        codecBuffer->hdiBuffer.version = verInfo_.compVersion;
        codecBuffer->hdiBuffer.bufferType = CodecBufferType::CODEC_BUFFER_TYPE_AVSHARE_MEM_FD;
        codecBuffer->hdiBuffer.bufferLen = bufferType->bufLen;
        codecBuffer->hdiBuffer.buffer = reinterpret_cast<uint8_t *>(bufferType->buf);
        codecBuffer->hdiBuffer.allocLen = bufferType->totalSize;
        codecBuffer->hdiBuffer.offset = bufferType->offset;
        codecBuffer->hdiBuffer.filledLen = bufferType->length;
        codecBuffer->hdiBuffer.fenceFd = bufferType->fenceFd;
        codecBuffer->hdiBuffer.flag = 0;
        codecBuffer->hdiBuffer.type = bufferType->memFlag == FLAGS_READ_ONLY ? READ_ONLY_TYPE : READ_WRITE_TYPE;
        preBuffers.push_back(codecBuffer);
    }
    return preBuffers;
}

int32_t HdiBufferMgr::UseHdiBuffers(std::vector<std::shared_ptr<HdiBufferWrap>> &buffers)
{
    MEDIA_LOGD("Enter UseHdiBuffers");
    CHECK_AND_RETURN_RET_LOG(buffers.size() == mPortDef_.nBufferCountActual, GST_CODEC_ERROR, "BufferNum error");
    for (auto buffer : buffers) {
        CHECK_AND_RETURN_RET_LOG(buffer != nullptr, GST_CODEC_ERROR, "buffer is nullptr");
        auto ret = handle_->UseBuffer(handle_, (uint32_t)mPortIndex_, &buffer->hdiBuffer);
        CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "UseBuffer failed");
        MEDIA_LOGD("Enter buffer id %{public}d", buffer->hdiBuffer.bufferId);
        availableBuffers_.push_back(buffer);
    }
    return GST_CODEC_OK;
}

void HdiBufferMgr::FreeCodecBuffers()
{
    MEDIA_LOGD("Enter FreeCodecBuffers");
    for (auto codecBuffer : availableBuffers_) {
        auto ret = handle_->FreeBuffer(handle_, mPortIndex_, &codecBuffer->hdiBuffer);
        if (ret != HDF_SUCCESS) {
            MEDIA_LOGE("free buffer %{public}u fail", codecBuffer->hdiBuffer.bufferId);
        }
    }
    EmptyList(availableBuffers_);
    MEDIA_LOGD("Enter FreeCodecBuffers End");
}

int32_t HdiBufferMgr::Stop()
{
    MEDIA_LOGD("Enter Stop");
    std::unique_lock<std::mutex> lock(mutex_);
    isStart_ = false;
    bufferCond_.notify_all();
    flushCond_.notify_all();
    return GST_CODEC_OK;
}

int32_t HdiBufferMgr::Flush(bool enable)
{
    MEDIA_LOGD("Enter Flush %{public}d", enable);
    std::unique_lock<std::mutex> lock(mutex_);
    isFlushing_ = enable;
    if (isFlushing_) {
        bufferCond_.notify_all();
        isFlushed_ = true;
    }
    if (!isFlushing_) {
        flushCond_.notify_all();
    }
    return GST_CODEC_OK;
}

void HdiBufferMgr::WaitFlushed()
{
    MEDIA_LOGD("Enter WaitFlushed");
    std::unique_lock<std::mutex> lock(mutex_);
    flushCond_.wait(lock, [this]() { return !isFlushing_ || !isStart_; });
}

void HdiBufferMgr::NotifyAvailable()
{
    if (isStart_ == false && availableBuffers_.size() == mPortDef_.nBufferCountActual) {
        freeCond_.notify_all();
    }
}
}  // namespace Media
}  // namespace OHOS
