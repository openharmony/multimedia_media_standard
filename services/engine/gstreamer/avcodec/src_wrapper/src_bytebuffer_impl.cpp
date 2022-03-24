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

#include "src_bytebuffer_impl.h"
#include <mutex>
#include "gst_shmem_memory.h"
#include "media_log.h"
#include "scope_guard.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SrcBytebufferImpl"};
    constexpr uint32_t DEFAULT_BUFFER_NUM = 10;
}

namespace OHOS {
namespace Media {
SrcBytebufferImpl::SrcBytebufferImpl()
{
}

SrcBytebufferImpl::~SrcBytebufferImpl()
{
    bufferList_.clear();
    if (src_ != nullptr) {
        gst_object_unref(src_);
        src_ = nullptr;
    }
    if (caps_ != nullptr) {
        gst_caps_unref(caps_);
        caps_ = nullptr;
    }
}

int32_t SrcBytebufferImpl::Init()
{
    src_ = GST_ELEMENT_CAST(gst_object_ref(gst_element_factory_make("shmemsrc", "src")));
    CHECK_AND_RETURN_RET_LOG(src_ != nullptr, MSERR_UNKNOWN, "Failed to gst_element_factory_make");
    return MSERR_OK;
}

int32_t SrcBytebufferImpl::Configure(std::shared_ptr<ProcessorConfig> config)
{
    CHECK_AND_RETURN_RET(config != nullptr, MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(src_ != nullptr, MSERR_UNKNOWN);

    g_object_set(G_OBJECT(src_), "buffer-num", DEFAULT_BUFFER_NUM, nullptr);
    g_object_set(G_OBJECT(src_), "buffer-size", config->bufferSize_, nullptr);
    g_object_set(G_OBJECT(src_), "caps", config->caps_, nullptr);
    gst_mem_pool_src_set_callback(GST_MEM_POOL_SRC(src_), BufferAvailable, this, nullptr);

    needCodecData_ = config->needCodecData_;
    if (needCodecData_) {
        caps_ = config->caps_;
        gst_caps_ref(caps_);
    }
    return MSERR_OK;
}

int32_t SrcBytebufferImpl::Start()
{
    std::unique_lock<std::mutex> lock(mutex_);
    start_ = true;
    return MSERR_OK;
}

int32_t SrcBytebufferImpl::Stop()
{
    std::unique_lock<std::mutex> lock(mutex_);
    start_ = false;
    return MSERR_OK;
}

int32_t SrcBytebufferImpl::Flush()
{
    std::unique_lock<std::mutex> lock(mutex_);
    for (auto it = bufferList_.begin(); it != bufferList_.end(); it++) {
        if ((*it)->owner_ != BufferWrapper::DOWNSTREAM) {
            (*it)->owner_ = BufferWrapper::DOWNSTREAM;
            if ((*it)->gstBuffer_ != nullptr) {
                gst_buffer_unref((*it)->gstBuffer_);
                (*it)->gstBuffer_ = nullptr;
            }
        }
    }
    return MSERR_OK;
}

bool SrcBytebufferImpl::Needflush()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return !bufferList_.empty();
}

std::shared_ptr<AVSharedMemory> SrcBytebufferImpl::GetInputBuffer(uint32_t index)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(index <= bufferList_.size(), nullptr);
    CHECK_AND_RETURN_RET(bufferList_[index]->owner_ == BufferWrapper::SERVER, nullptr);

    GstMemory *memory = gst_buffer_peek_memory(bufferList_[index]->gstBuffer_, 0);
    CHECK_AND_RETURN_RET(memory != nullptr, nullptr);

    GstShMemMemory *shmem = reinterpret_cast<GstShMemMemory *>(memory);
    bufferList_[index]->owner_ = BufferWrapper::APP;

    return shmem->mem;
}

int32_t SrcBytebufferImpl::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(index <= bufferList_.size(), MSERR_INVALID_VAL);

    auto &bufWrapper = bufferList_[index];
    CHECK_AND_RETURN_RET(bufWrapper->owner_ == BufferWrapper::APP, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(bufWrapper->gstBuffer_ != nullptr, MSERR_INVALID_OPERATION);
    gst_buffer_resize(bufWrapper->gstBuffer_, info.offset, info.size);

    if (needCodecData_) {
        if (HandleCodecBuffer(index, info, flag) == MSERR_OK) {
            needCodecData_ = false;
            bufWrapper->owner_ = BufferWrapper::SERVER;
            auto obs = obs_.lock();
            CHECK_AND_RETURN_RET(obs != nullptr, MSERR_UNKNOWN);
            obs->OnInputBufferAvailable(index);
            MEDIA_LOGD("OnInputBufferAvailable, index:%{public}u", index);
            return MSERR_OK;
        }
        return MSERR_UNKNOWN;
    }

    uint8_t *address = bufWrapper->mem_->GetBase();
    CHECK_AND_RETURN_RET(address != nullptr, MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET((info.offset + info.size) <= bufWrapper->mem_->GetSize(), MSERR_INVALID_VAL);

    constexpr int32_t usToNs = 1000;
    if (info.presentationTimeUs < 0) {
        MEDIA_LOGE("Invalid pts: < 0, use 0 as default");
        GST_BUFFER_PTS(bufWrapper->gstBuffer_) = 0;
    } else if ((INT64_MAX / usToNs) <= info.presentationTimeUs) {
        MEDIA_LOGE("Invalid pts: too big, use 0 as default");
        GST_BUFFER_PTS(bufWrapper->gstBuffer_) = 0;
    } else {
        GST_BUFFER_PTS(bufWrapper->gstBuffer_) = static_cast<uint64_t>(info.presentationTimeUs * usToNs);
    }

    CHECK_AND_RETURN_RET(src_ != nullptr, MSERR_UNKNOWN);
    (void)gst_mem_pool_src_push_buffer(GST_MEM_POOL_SRC(src_), bufWrapper->gstBuffer_);
    bufWrapper->owner_ = BufferWrapper::DOWNSTREAM;
    bufWrapper->gstBuffer_ = nullptr; // src elem take ownership of this buffer.

    MEDIA_LOGD("QueueInputBuffer, index = %{public}u", index);
    return MSERR_OK;
}

int32_t SrcBytebufferImpl::SetCallback(const std::weak_ptr<IAVCodecEngineObs> &obs)
{
    std::unique_lock<std::mutex> lock(mutex_);
    obs_ = obs;
    return MSERR_OK;
}

int32_t SrcBytebufferImpl::HandleCodecBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    bool hasCodecFlag = static_cast<uint32_t>(flag) & static_cast<uint32_t>(AVCODEC_BUFFER_FLAG_CODEC_DATA);
    CHECK_AND_RETURN_RET_LOG(hasCodecFlag == true, MSERR_INVALID_VAL, "First buffer must be codec buffer");

    uint8_t *address = bufferList_[index]->mem_->GetBase();
    CHECK_AND_RETURN_RET(address != nullptr, MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET((info.offset + info.size) <= bufferList_[index]->mem_->GetSize(), MSERR_INVALID_VAL);

    GstBuffer *codecBuffer = gst_buffer_new_allocate(nullptr, info.size, nullptr);
    CHECK_AND_RETURN_RET_LOG(codecBuffer != nullptr, MSERR_NO_MEMORY, "no memory");

    ON_SCOPE_EXIT(0) { gst_buffer_unref(codecBuffer); };

    gsize size = gst_buffer_fill(codecBuffer, 0, (char *)address + info.offset, info.size);
    CHECK_AND_RETURN_RET(size == static_cast<gsize>(info.size), MSERR_UNKNOWN);

    CHECK_AND_RETURN_RET(caps_ != nullptr, MSERR_UNKNOWN);
    gst_caps_set_simple(caps_, "codec_data", GST_TYPE_BUFFER, codecBuffer, nullptr);

    CHECK_AND_RETURN_RET(src_ != nullptr, MSERR_UNKNOWN);
    g_object_set(G_OBJECT(src_), "caps", caps_, nullptr);

    CHECK_AND_RETURN_RET(gst_base_src_set_caps(GST_BASE_SRC(src_), caps_) == TRUE, MSERR_UNKNOWN);
    return MSERR_OK;
}

GstFlowReturn SrcBytebufferImpl::BufferAvailable(GstMemPoolSrc *memsrc, gpointer userdata)
{
    CHECK_AND_RETURN_RET(memsrc != nullptr, GST_FLOW_ERROR);
    CHECK_AND_RETURN_RET(userdata != nullptr, GST_FLOW_ERROR);

    GstBuffer *buffer = gst_mem_pool_src_pull_buffer(memsrc);
    CHECK_AND_RETURN_RET(buffer != nullptr, GST_FLOW_ERROR);

    SrcBytebufferImpl *thiz = reinterpret_cast<SrcBytebufferImpl *>(userdata);
    int32_t ret = thiz->HandleBufferAvailable(buffer);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, GST_FLOW_ERROR);

    return GST_FLOW_OK;
}

int32_t SrcBytebufferImpl::HandleBufferAvailable(GstBuffer *buffer)
{
    std::unique_lock<std::mutex> lock(mutex_);
    ON_SCOPE_EXIT(0) { gst_buffer_unref(buffer); };
    if (!start_) {
        MEDIA_LOGD("Codec source is stop, unref available buffer");
        return MSERR_OK;
    }

    GstMemory *memory = gst_buffer_peek_memory(buffer, 0);
    CHECK_AND_RETURN_RET(memory != nullptr, MSERR_UNKNOWN);
    GstShMemMemory *shmem = reinterpret_cast<GstShMemMemory *>(memory);
    CHECK_AND_RETURN_RET(shmem->mem != nullptr, MSERR_UNKNOWN);

    uint32_t index = 0;
    int32_t ret = FindBufferIndex(index, shmem->mem);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, MSERR_UNKNOWN);

    auto obs = obs_.lock();
    CHECK_AND_RETURN_RET_LOG(obs != nullptr, MSERR_UNKNOWN, "obs is nullptr");
    obs->OnInputBufferAvailable(index);

    MEDIA_LOGD("OnInputBufferAvailable, index:%{public}u", index);
    bufferList_[index]->owner_ = BufferWrapper::SERVER;
    bufferList_[index]->gstBuffer_ = gst_buffer_ref(buffer);

    return MSERR_OK;
}

int32_t SrcBytebufferImpl::FindBufferIndex(uint32_t &index, std::shared_ptr<AVSharedMemory> mem)
{
    CHECK_AND_RETURN_RET(mem != nullptr, MSERR_UNKNOWN);

    index = 0;
    for (auto it = bufferList_.begin(); it != bufferList_.end(); it++) {
        if ((*it) != nullptr && (*it)->mem_ == mem.get()) {
            break;
        }
        index++;
    }

    if (index == bufferList_.size()) {
        auto bufWrap = std::make_shared<BufferWrapper>(BufferWrapper::SERVER);
        CHECK_AND_RETURN_RET(bufWrap != nullptr, MSERR_NO_MEMORY);
        bufWrap->mem_ = mem.get();
        bufferList_.push_back(bufWrap);
    }

    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
