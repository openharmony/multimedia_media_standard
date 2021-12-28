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
#include "media_log.h"
#include "scope_guard.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SrcBytebufferImpl"};
    const uint32_t DEFAULT_BUFFER_COUNT = 5;
    const uint32_t DEFAULT_BUFFER_SIZE = 30000;
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
    src_ = GST_ELEMENT_CAST(gst_object_ref(gst_element_factory_make("appsrc", "src")));
    CHECK_AND_RETURN_RET_LOG(src_ != nullptr, MSERR_UNKNOWN, "Failed to gst_element_factory_make");

    bufferCount_ = DEFAULT_BUFFER_COUNT;
    bufferSize_ = DEFAULT_BUFFER_SIZE;

    return MSERR_OK;
}

int32_t SrcBytebufferImpl::Configure(std::shared_ptr<ProcessorConfig> config)
{
    CHECK_AND_RETURN_RET(src_ != nullptr, MSERR_UNKNOWN);
    g_object_set(G_OBJECT(src_), "caps", config->caps_, nullptr);
    g_object_set(G_OBJECT(src_), "format", GST_FORMAT_TIME, nullptr);
    g_object_set(G_OBJECT(src_), "is-live", TRUE, nullptr);
    g_object_set(G_OBJECT(src_), "block", TRUE, nullptr);

    for (uint32_t i = 0; i < bufferCount_; i++) {
        auto mem = AVSharedMemory::Create(bufferSize_, AVSharedMemory::Flags::FLAGS_READ_WRITE, "input");
        CHECK_AND_RETURN_RET(mem != nullptr, MSERR_NO_MEMORY);

        auto bufWrap = std::make_shared<BufferWrapper>(mem, nullptr, bufferList_.size(), BufferWrapper::SERVER);
        CHECK_AND_RETURN_RET(bufWrap != nullptr, MSERR_NO_MEMORY);
        bufferList_.push_back(bufWrap);
    }
    needCodecData_ = config->needCodecData_;
    if (needCodecData_) {
        caps_ = config->caps_;
        gst_caps_ref(caps_);
    }
    return MSERR_OK;
}

int32_t SrcBytebufferImpl::Flush()
{
    std::unique_lock<std::mutex> lock(mutex_);
    for (auto it = bufferList_.begin(); it != bufferList_.end(); it++) {
        (*it)->owner_ = BufferWrapper::SERVER;
    }
    return MSERR_OK;
}

uint32_t SrcBytebufferImpl::GetBufferCount()
{
    return bufferCount_;
}

std::shared_ptr<AVSharedMemory> SrcBytebufferImpl::GetInputBuffer(uint32_t index)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(index < bufferCount_ && index <= bufferList_.size(), nullptr);
    CHECK_AND_RETURN_RET(bufferList_[index]->owner_ == BufferWrapper::SERVER, nullptr);

    bufferList_[index]->owner_ = BufferWrapper::APP;
    return bufferList_[index]->mem_;
}

int32_t SrcBytebufferImpl::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(index < bufferCount_ && index <= bufferList_.size(), MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(bufferList_[index]->owner_ == BufferWrapper::APP, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(bufferList_[index]->mem_ != nullptr, MSERR_UNKNOWN);

    if (needCodecData_) {
        if (HandleCodecBuffer(index, info, flag) == MSERR_OK) {
            needCodecData_ = false;
            bufferList_[index]->owner_ = BufferWrapper::SERVER;
            auto obs = obs_.lock();
            CHECK_AND_RETURN_RET(obs != nullptr, MSERR_UNKNOWN);
            obs->OnInputBufferAvailable(index);
            return MSERR_OK;
        }
        return MSERR_UNKNOWN;
    }

    uint8_t *address = bufferList_[index]->mem_->GetBase();
    CHECK_AND_RETURN_RET(address != nullptr, MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET((info.offset + info.size) <= bufferList_[index]->mem_->GetSize(), MSERR_INVALID_VAL);

    GstBuffer *buffer = gst_buffer_new_allocate(nullptr, static_cast<gsize>(info.size), nullptr);
    CHECK_AND_RETURN_RET(buffer != nullptr, MSERR_NO_MEMORY);

    gsize size = gst_buffer_fill(buffer, 0, (char *)address + info.offset, info.size);
    CHECK_AND_RETURN_RET(size == static_cast<gsize>(info.size), MSERR_UNKNOWN);

    const int32_t usToNs = 1000;
    GST_BUFFER_PTS(buffer) = info.presentationTimeUs * usToNs;

    int32_t ret = GST_FLOW_OK;
    CHECK_AND_RETURN_RET(src_ != nullptr, MSERR_UNKNOWN);
    g_signal_emit_by_name(src_, "push-buffer", buffer, &ret);
    gst_buffer_unref(buffer);
    bufferList_[index]->owner_ = BufferWrapper::SERVER;

    auto obs = obs_.lock();
    CHECK_AND_RETURN_RET(obs != nullptr, MSERR_UNKNOWN);
    obs->OnInputBufferAvailable(index);

    return MSERR_OK;
}

int32_t SrcBytebufferImpl::SetCallback(const std::weak_ptr<IAVCodecEngineObs> &obs)
{
    std::unique_lock<std::mutex> lock(mutex_);
    obs_ = obs;
    return MSERR_OK;
}

int32_t SrcBytebufferImpl::SetParameter(const Format &format)
{
    int32_t value = 0;
    if (format.GetIntValue("repeat_frame_after", value) == true) {
        g_object_set(src_, "repeat-frame-after", value, nullptr);
    }

    return MSERR_OK;
}

int32_t SrcBytebufferImpl::HandleCodecBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    bool hasCodecFlag = static_cast<int32_t>(flag) & static_cast<int32_t>(AVCODEC_BUFFER_FLAG_CODEDC_DATA);
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
} // Media
} // OHOS
