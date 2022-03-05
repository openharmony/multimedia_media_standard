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

#include "sink_surface_impl.h"
#include "display_type.h"
#include "media_log.h"
#include "scope_guard.h"
#include "gst_surface_memory.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SinkSurfaceImpl"};
}

namespace OHOS {
namespace Media {
SinkSurfaceImpl::SinkSurfaceImpl()
{
}

SinkSurfaceImpl::~SinkSurfaceImpl()
{
    std::unique_lock<std::mutex> lock(mutex_);
    bufferList_.clear();
    if (sink_ != nullptr) {
        gst_object_unref(sink_);
        sink_ = nullptr;
    }
}

int32_t SinkSurfaceImpl::Init()
{
    sink_ = GST_ELEMENT_CAST(gst_object_ref(gst_element_factory_make("surfacememsink", "sink")));
    CHECK_AND_RETURN_RET(sink_ != nullptr, MSERR_UNKNOWN);
    gst_base_sink_set_async_enabled(GST_BASE_SINK(sink_), FALSE);
    return MSERR_OK;
}

int32_t SinkSurfaceImpl::Configure(std::shared_ptr<ProcessorConfig> config)
{
    CHECK_AND_RETURN_RET(sink_ != nullptr && config->caps_ != nullptr, MSERR_UNKNOWN);

    g_object_set(G_OBJECT(sink_), "caps", config->caps_, nullptr);
    (void)CapsToFormat(config->caps_, bufferFormat_);

    GstMemSinkCallbacks sinkCallbacks = { EosCb, nullptr, NewSampleCb };
    gst_mem_sink_set_callback(GST_MEM_SINK(sink_), &sinkCallbacks, this, nullptr);

    return MSERR_OK;
}

int32_t SinkSurfaceImpl::Flush()
{
    std::unique_lock<std::mutex> lock(mutex_);
    for (auto it = bufferList_.begin(); it != bufferList_.end(); it++) {
        if ((*it)->owner_ != BufferWrapper::DOWNSTREAM) {
            (*it)->owner_ = BufferWrapper::DOWNSTREAM;
            if ((*it)->gstBuffer_ != nullptr) {
                gst_buffer_unref((*it)->gstBuffer_);
            }
        }
    }
    isFirstFrame_ = true;
    return MSERR_OK;
}

int32_t SinkSurfaceImpl::SetOutputSurface(sptr<Surface> surface)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(sink_ != nullptr, MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(surface != nullptr, MSERR_INVALID_VAL);
    g_object_set(G_OBJECT(sink_), "surface", static_cast<gpointer>(surface), nullptr);
    return MSERR_OK;
}

int32_t SinkSurfaceImpl::SetParameter(const Format &format)
{
    int32_t value = 0;
    if (format.GetIntValue("rect_top", value) == true) {
        g_object_set(sink_, "rect-top", value, nullptr);
    }

    if (format.GetIntValue("rect_bottom", value) == true) {
        g_object_set(sink_, "rect-top", value, nullptr);
    }

    if (format.GetIntValue("rect_left", value) == true) {
        g_object_set(sink_, "rect-top", value, nullptr);
    }

    if (format.GetIntValue("rect_right", value) == true) {
        g_object_set(sink_, "rect-top", value, nullptr);
    }
    return MSERR_OK;
}

int32_t SinkSurfaceImpl::ReleaseOutputBuffer(uint32_t index, bool render)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(index < bufferList_.size(), MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(bufferList_[index]->owner_ == BufferWrapper::SERVER, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(bufferList_[index]->gstBuffer_ != nullptr, MSERR_UNKNOWN);

    if (render) {
        (void)gst_mem_sink_app_render(GST_MEM_SINK(sink_), bufferList_[index]->gstBuffer_);
    }

    bufferList_[index]->owner_ = BufferWrapper::DOWNSTREAM;
    gst_buffer_unref(bufferList_[index]->gstBuffer_);
    return MSERR_OK;
}

int32_t SinkSurfaceImpl::SetCallback(const std::weak_ptr<IAVCodecEngineObs> &obs)
{
    std::unique_lock<std::mutex> lock(mutex_);
    obs_ = obs;
    return MSERR_OK;
}

GstFlowReturn SinkSurfaceImpl::NewSampleCb(GstMemSink *memSink, GstBuffer *sample, gpointer userData)
{
    (void)memSink;
    auto impl = static_cast<SinkSurfaceImpl *>(userData);
    CHECK_AND_RETURN_RET(impl != nullptr, GST_FLOW_ERROR);
    std::unique_lock<std::mutex> lock(impl->mutex_);
    CHECK_AND_RETURN_RET(impl->HandleNewSampleCb(sample) == MSERR_OK, GST_FLOW_ERROR);
    return GST_FLOW_OK;
}

void SinkSurfaceImpl::EosCb(GstMemSink *memSink, gpointer userData)
{
    (void)memSink;
    MEDIA_LOGI("EOS reached");
    auto impl = static_cast<SinkSurfaceImpl *>(userData);
    CHECK_AND_RETURN(impl != nullptr);
    std::unique_lock<std::mutex> lock(impl->mutex_);

    auto obs = impl->obs_.lock();
    CHECK_AND_RETURN(obs != nullptr);

    AVCodecBufferInfo info;
    constexpr uint32_t invalidIndex = 1000;
    obs->OnOutputBufferAvailable(invalidIndex, info, AVCODEC_BUFFER_FLAG_EOS);
}

int32_t SinkSurfaceImpl::HandleNewSampleCb(GstBuffer *buffer)
{
    CHECK_AND_RETURN_RET(buffer != nullptr, MSERR_UNKNOWN);
    ON_SCOPE_EXIT(0) { gst_buffer_unref(buffer); };

    GstMemory *memory = gst_buffer_peek_memory(buffer, 0);
    CHECK_AND_RETURN_RET(memory != nullptr, MSERR_UNKNOWN);
    GstSurfaceMemory *surfaceMem = reinterpret_cast<GstSurfaceMemory *>(memory);
    CHECK_AND_RETURN_RET(surfaceMem->buf != nullptr, MSERR_UNKNOWN);

    uint32_t index = 0;
    CHECK_AND_RETURN_RET(FindBufferIndex(index, surfaceMem->buf) == MSERR_OK, MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(index < bufferList_.size(), MSERR_UNKNOWN);
    bufferList_[index]->gstBuffer_ = buffer;

    auto obs = obs_.lock();
    CHECK_AND_RETURN_RET_LOG(obs != nullptr, MSERR_UNKNOWN, "obs is nullptr");

    if (isFirstFrame_) {
        isFirstFrame_ = false;
        obs->OnOutputFormatChanged(bufferFormat_);
    }

    AVCodecBufferInfo info;
    info.offset = 0;
    info.size = 0;
    constexpr uint64_t nsToUs = 1000;
    info.presentationTimeUs = static_cast<int64_t>(GST_BUFFER_PTS(buffer) / nsToUs);
    obs->OnOutputBufferAvailable(index, info, AVCODEC_BUFFER_FLAG_NONE);

    MEDIA_LOGD("OutputBufferAvailable, index:%{public}d", index);
    bufferList_[index]->owner_ = BufferWrapper::SERVER;
    gst_buffer_ref(buffer);
    CANCEL_SCOPE_EXIT_GUARD(0);

    return MSERR_OK;
}

int32_t SinkSurfaceImpl::FindBufferIndex(uint32_t &index, sptr<SurfaceBuffer> buffer)
{
    CHECK_AND_RETURN_RET(buffer != nullptr, MSERR_UNKNOWN);

    index = 0;
    for (auto it = bufferList_.begin(); it != bufferList_.end(); it++) {
        if ((*it) != nullptr && (*it)->surfaceBuffer_->GetVirAddr() == buffer->GetVirAddr()) {
            break;
        }
        index++;
    }

    if (index == bufferList_.size()) {
        auto bufWrap = std::make_shared<BufferWrapper>(BufferWrapper::SERVER);
        CHECK_AND_RETURN_RET(bufWrap != nullptr, MSERR_NO_MEMORY);
        bufWrap->surfaceBuffer_ = buffer;
        bufferList_.push_back(bufWrap);
    }

    MEDIA_LOGD("bufferList_ size is: %{public}zu", bufferList_.size());

    CHECK_AND_RETURN_RET(index < bufferList_.size(), MSERR_UNKNOWN);
    return MSERR_OK;
}
} // Media
} // OHOS
