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

#include "sink_bytebuffer_impl.h"
#include "gst_shmem_memory.h"
#include "media_log.h"
#include "scope_guard.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SinkBytebufferImpl"};
}

namespace OHOS {
namespace Media {
SinkBytebufferImpl::SinkBytebufferImpl()
{
}

SinkBytebufferImpl::~SinkBytebufferImpl()
{
    std::unique_lock<std::mutex> lock(mutex_);
    bufferList_.clear();
    if (sink_ != nullptr) {
        gst_object_unref(sink_);
        sink_ = nullptr;
    }
}

int32_t SinkBytebufferImpl::Init()
{
    sink_ = GST_ELEMENT_CAST(gst_object_ref_sink(gst_element_factory_make("sharedmemsink", "sink")));
    CHECK_AND_RETURN_RET(sink_ != nullptr, MSERR_UNKNOWN);
    gst_base_sink_set_async_enabled(GST_BASE_SINK(sink_), FALSE);
    return MSERR_OK;
}

int32_t SinkBytebufferImpl::Configure(std::shared_ptr<ProcessorConfig> config)
{
    CHECK_AND_RETURN_RET(sink_ != nullptr && config->caps_ != nullptr, MSERR_UNKNOWN);

    g_object_set(G_OBJECT(sink_), "mem-size", static_cast<guint>(config->bufferSize_), nullptr);
    g_object_set(G_OBJECT(sink_), "caps", config->caps_, nullptr);
    (void)CapsToFormat(config->caps_, bufferFormat_);

    GstMemSinkCallbacks sinkCallbacks = { EosCb, nullptr, NewSampleCb };
    gst_mem_sink_set_callback(GST_MEM_SINK(sink_), &sinkCallbacks, this, nullptr);

    return MSERR_OK;
}

int32_t SinkBytebufferImpl::Flush()
{
    std::unique_lock<std::mutex> lock(mutex_);
    bufferList_.clear();
    isFirstFrame_ = true;
    isEos_ = false;
    return MSERR_OK;
}

std::shared_ptr<AVSharedMemory> SinkBytebufferImpl::GetOutputBuffer(uint32_t index)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(index <= bufferList_.size(), nullptr);
    CHECK_AND_RETURN_RET(bufferList_[index] != nullptr, nullptr);
    CHECK_AND_RETURN_RET(bufferList_[index]->owner_ == BufferWrapper::SERVER, nullptr);

    GstMemory *memory = gst_buffer_peek_memory(bufferList_[index]->gstBuffer_, 0);
    CHECK_AND_RETURN_RET(memory != nullptr, nullptr);
    CHECK_AND_RETURN_RET(gst_is_shmem_memory(memory), nullptr);

    GstShMemMemory *shmem = reinterpret_cast<GstShMemMemory *>(memory);
    bufferList_[index]->owner_ = BufferWrapper::APP;
    return shmem->mem;
}

int32_t SinkBytebufferImpl::ReleaseOutputBuffer(uint32_t index, bool render)
{
    (void)render;
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(index < bufferList_.size(), MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(bufferList_[index] != nullptr, MSERR_INVALID_VAL);
    CHECK_AND_RETURN_RET(bufferList_[index]->owner_ == BufferWrapper::APP, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(bufferList_[index]->gstBuffer_ != nullptr, MSERR_UNKNOWN);

    bufferList_[index]->owner_ = BufferWrapper::DOWNSTREAM;
    gst_buffer_unref(bufferList_[index]->gstBuffer_);
    bufferList_[index]->gstBuffer_ = nullptr;

    return MSERR_OK;
}

int32_t SinkBytebufferImpl::SetCallback(const std::weak_ptr<IAVCodecEngineObs> &obs)
{
    std::unique_lock<std::mutex> lock(mutex_);
    obs_ = obs;
    return MSERR_OK;
}

bool SinkBytebufferImpl::IsEos()
{
    return isEos_;
}

GstFlowReturn SinkBytebufferImpl::NewSampleCb(GstMemSink *memSink, GstBuffer *sample, gpointer userData)
{
    (void)memSink;
    auto impl = static_cast<SinkBytebufferImpl *>(userData);
    if (impl == nullptr) {
        MEDIA_LOGE("impl is nullptr");
        gst_buffer_unref(sample);
        return GST_FLOW_ERROR;
    }
    std::unique_lock<std::mutex> lock(impl->mutex_);
    CHECK_AND_RETURN_RET(impl->HandleNewSampleCb(sample) == MSERR_OK, GST_FLOW_ERROR);
    return GST_FLOW_OK;
}

void SinkBytebufferImpl::EosCb(GstMemSink *memSink, gpointer userData)
{
    (void)memSink;
    MEDIA_LOGI("EOS reached");
    auto impl = static_cast<SinkBytebufferImpl *>(userData);
    CHECK_AND_RETURN(impl != nullptr);
    std::unique_lock<std::mutex> lock(impl->mutex_);
    impl->isEos_ = true;

    auto obs = impl->obs_.lock();
    CHECK_AND_RETURN(obs != nullptr);

    AVCodecBufferInfo info;
    constexpr uint32_t invalidIndex = 1000;
    obs->OnOutputBufferAvailable(invalidIndex, info, AVCODEC_BUFFER_FLAG_EOS);
}

int32_t SinkBytebufferImpl::HandleNewSampleCb(GstBuffer *buffer)
{
    CHECK_AND_RETURN_RET(buffer != nullptr, MSERR_UNKNOWN);
    ON_SCOPE_EXIT(0) { gst_buffer_unref(buffer); };

    GstMemory *memory = gst_buffer_peek_memory(buffer, 0);
    CHECK_AND_RETURN_RET(memory != nullptr, MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(gst_is_shmem_memory(memory), MSERR_UNKNOWN);
    GstShMemMemory *shmem = reinterpret_cast<GstShMemMemory *>(memory);
    CHECK_AND_RETURN_RET(shmem->mem != nullptr, MSERR_UNKNOWN);

    uint32_t index = 0;
    CHECK_AND_RETURN_RET(FindBufferIndex(index, shmem->mem) == MSERR_OK, MSERR_UNKNOWN);

    auto obs = obs_.lock();
    CHECK_AND_RETURN_RET_LOG(obs != nullptr, MSERR_UNKNOWN, "obs is nullptr");

    if (isFirstFrame_) {
        isFirstFrame_ = false;
        obs->OnOutputFormatChanged(bufferFormat_);
    }

    isEos_ = false;

    GstMapInfo map = GST_MAP_INFO_INIT;
    CHECK_AND_RETURN_RET(gst_buffer_map(buffer, &map, GST_MAP_READ) == TRUE, MSERR_UNKNOWN);

    AVCodecBufferInfo info;
    info.offset = 0;
    if (map.size >= INT32_MAX) {
        MEDIA_LOGE("Invalid size");
        gst_buffer_unmap(buffer, &map);
        return MSERR_UNKNOWN;
    }
    info.size = static_cast<int32_t>(map.size);
    constexpr uint64_t nsToUs = 1000;
    info.presentationTimeUs = static_cast<int64_t>(GST_BUFFER_PTS(buffer) / nsToUs);
    obs->OnOutputBufferAvailable(index, info, AVCODEC_BUFFER_FLAG_NONE);

    MEDIA_LOGD("OutputBufferAvailable, index:%{public}d", index);
    gst_buffer_unmap(buffer, &map);
    bufferList_[index]->owner_ = BufferWrapper::SERVER;
    bufferList_[index]->gstBuffer_ = gst_buffer_ref(buffer);

    return MSERR_OK;
}

int32_t SinkBytebufferImpl::FindBufferIndex(uint32_t &index, std::shared_ptr<AVSharedMemory> mem)
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
