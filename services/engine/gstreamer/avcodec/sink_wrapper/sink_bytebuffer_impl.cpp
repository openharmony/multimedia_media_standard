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
#include "media_log.h"
#include "securec.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SinkBytebufferImpl"};
    const uint32_t DEFAULT_BUFFER_COUNT = 5;
    const uint32_t DEFAULT_BUFFER_SIZE = 30000;
}

namespace OHOS {
namespace Media {
SinkBytebufferImpl::SinkBytebufferImpl()
{
}

SinkBytebufferImpl::~SinkBytebufferImpl()
{
    g_signal_handler_disconnect(G_OBJECT(element_), signalId_);
    bufferList_.clear();
    if (element_ != nullptr) {
        gst_object_unref(element_);
        element_ = nullptr;
    }
}

int32_t SinkBytebufferImpl::Init()
{
    element_ = GST_ELEMENT_CAST(gst_object_ref(gst_element_factory_make("appsink", "sink")));
    CHECK_AND_RETURN_RET(element_ != nullptr, MSERR_UNKNOWN);
    gst_base_sink_set_async_enabled(GST_BASE_SINK(element_), FALSE);

    bufferCount_ = DEFAULT_BUFFER_COUNT;
    bufferSize_ = DEFAULT_BUFFER_SIZE;

    return MSERR_OK;
}

int32_t SinkBytebufferImpl::Configure(std::shared_ptr<ProcessorConfig> config)
{
    CHECK_AND_RETURN_RET(element_ != nullptr, MSERR_UNKNOWN);
    g_object_set(G_OBJECT(element_), "caps", config->caps_, nullptr);

    for (uint32_t i = 0; i < bufferCount_; i++) {
        auto mem = AVSharedMemory::Create(bufferSize_, AVSharedMemory::Flags::FLAGS_READ_WRITE, "output");
        CHECK_AND_RETURN_RET(mem != nullptr, MSERR_NO_MEMORY);

        GstBuffer *buffer = gst_buffer_new_allocate(nullptr, static_cast<gsize>(DEFAULT_BUFFER_SIZE), nullptr);
        CHECK_AND_RETURN_RET(buffer != nullptr, MSERR_NO_MEMORY);

        auto bufWrap = std::make_shared<BufferWrapper>(mem, buffer, bufferList_.size(), BufferWrapper::DOWNSTREAM);
        CHECK_AND_RETURN_RET(bufWrap != nullptr, MSERR_NO_MEMORY);
        bufferList_.push_back(bufWrap);
    }

    signalId_ = g_signal_connect(G_OBJECT(element_), "new_sample",
                                 G_CALLBACK(OutputAvailableCb), reinterpret_cast<gpointer>(this));
    g_object_set(G_OBJECT(element_), "emit-signals", TRUE, nullptr);
    return MSERR_OK;
}

int32_t SinkBytebufferImpl::Flush()
{
    std::unique_lock<std::mutex> lock(mutex_);
    for (auto it = bufferList_.begin(); it != bufferList_.end(); it++) {
        (*it)->owner_ = BufferWrapper::DOWNSTREAM;
    }
    return MSERR_OK;
}

std::shared_ptr<AVSharedMemory> SinkBytebufferImpl::GetOutputBuffer(uint32_t index)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(index < bufferCount_ && index <= bufferList_.size(), nullptr);
    CHECK_AND_RETURN_RET(bufferList_[index]->owner_ == BufferWrapper::APP, nullptr);

    return bufferList_[index]->mem_;
}

int32_t SinkBytebufferImpl::ReleaseOutputBuffer(uint32_t index, bool render)
{
    (void)render;
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(index < bufferCount_ && index < bufferList_.size(), MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(bufferList_[index]->owner_ == BufferWrapper::APP, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(bufferList_[index]->gstBuffer_ != nullptr, MSERR_UNKNOWN);
    bufferList_[index]->owner_ = BufferWrapper::DOWNSTREAM;
    return MSERR_OK;
}

int32_t SinkBytebufferImpl::SetCallback(const std::weak_ptr<IAVCodecEngineObs> &obs)
{
    std::unique_lock<std::mutex> lock(mutex_);
    obs_ = obs;
    return MSERR_OK;
}

void SinkBytebufferImpl::SetEOS(uint32_t count)
{
    std::unique_lock<std::mutex> lock(mutex_);
    finishCount_ = count;
}

GstFlowReturn SinkBytebufferImpl::OutputAvailableCb(GstElement *sink, gpointer userData)
{
    (void)sink;
    auto impl = static_cast<SinkBytebufferImpl *>(userData);
    CHECK_AND_RETURN_RET(impl != nullptr, GST_FLOW_ERROR);
    std::unique_lock<std::mutex> lock(impl->mutex_);
    CHECK_AND_RETURN_RET(impl->HandleOutputCb() == MSERR_OK, GST_FLOW_ERROR);
    return GST_FLOW_OK;
}

int32_t SinkBytebufferImpl::HandleOutputCb()
{
    GstSample *sample = nullptr;
    g_signal_emit_by_name(G_OBJECT(element_), "pull-sample", &sample);
    CHECK_AND_RETURN_RET(sample != nullptr, MSERR_UNKNOWN);

    GstBuffer *buf = gst_sample_get_buffer(sample);
    if (buf == nullptr) {
        MEDIA_LOGE("Failed to gst_sample_get_buffer");
        gst_sample_unref(sample);
        return MSERR_UNKNOWN;
    }

    uint32_t bufSize = 0;
    uint32_t index = 0;
    for (auto it = bufferList_.begin(); it != bufferList_.end(); it++) {
        if ((*it)->owner_ == BufferWrapper::DOWNSTREAM) {
            if ((*it)->mem_ == nullptr) {
                index++;
                continue;
            }
            GstMapInfo map = GST_MAP_INFO_INIT;
            if (gst_buffer_map(buf, &map, GST_MAP_READ) != TRUE) {
                index++;
                continue;
            }
            (*it)->owner_ = BufferWrapper::APP;
            (*it)->gstBuffer_ = buf;
            bufSize = map.size;
            if (memcpy_s((*it)->mem_->GetBase(), (*it)->mem_->GetSize(), map.data, map.size) != EOK) {
                MEDIA_LOGE("Failed to copy output buffer");
            }
            gst_buffer_unmap(buf, &map);
            break;
        }
        index++;
    }

    bufferCount_++;
    gst_sample_unref(sample);

    if (index == bufferCount_ || index == bufferList_.size()) {
        MEDIA_LOGW("The output buffer queue is full, discard this buffer");
        return MSERR_INVALID_OPERATION;
    }

    auto obs = obs_.lock();
    CHECK_AND_RETURN_RET_LOG(obs != nullptr, MSERR_UNKNOWN, "obs is nullptr");
    AVCodecBufferInfo info;
    info.offset = 0;
    info.size = bufSize;
    info.presentationTimeUs = GST_BUFFER_PTS(buf);

    if (bufferCount_ == finishCount_) {
        MEDIA_LOGD("EOS reach");
        obs->OnOutputBufferAvailable(index, info, AVCODEC_BUFFER_FLAG_EOS);
    } else {
        obs->OnOutputBufferAvailable(index, info, AVCODEC_BUFFER_FLAG_NONE);
    }
    MEDIA_LOGD("OutputBuffer available, index:%{public}d", index);

    return MSERR_OK;
}
} // Media
} // OHOS
