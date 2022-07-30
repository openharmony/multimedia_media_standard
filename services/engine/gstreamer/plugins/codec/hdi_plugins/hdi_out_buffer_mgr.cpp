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

#include "hdi_out_buffer_mgr.h"
#include <algorithm>
#include <hdf_base.h>
#include "media_log.h"
#include "media_errors.h"
#include "hdi_codec_util.h"
#include "buffer_type_meta.h"
#include "scope_guard.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "HdiOutBufferMgr"};
}

namespace OHOS {
namespace Media {
HdiOutBufferMgr::HdiOutBufferMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

HdiOutBufferMgr::~HdiOutBufferMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t HdiOutBufferMgr::Start()
{
    MEDIA_LOGD("Enter Start");
    std::unique_lock<std::mutex> lock(mutex_);
    isStart_ = true;
    isFlushed_ = false;
    while (!mBuffers.empty()) {
        GstBuffer *buffer = mBuffers.front();
        if (buffer == nullptr) {
            mBuffers.pop_front();
            MEDIA_LOGI("nullptr, continue!");
            continue;
        }
        std::shared_ptr<HdiBufferWrap> codecBuffer = GetCodecBuffer(buffer);
        CHECK_AND_RETURN_RET_LOG(codecBuffer != nullptr, GST_CODEC_ERROR, "Push buffer failed");
        auto ret = HdiFillThisBuffer(handle_, &codecBuffer->hdiBuffer);
        CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "FillThisBuffer failed");
        mBuffers.pop_front();
        gst_buffer_unref(buffer);
    }
    MEDIA_LOGD("Quit Start");
    return GST_CODEC_OK;
}

int32_t HdiOutBufferMgr::PushBuffer(GstBuffer *buffer)
{
    MEDIA_LOGD("Enter PushBuffer");
    std::unique_lock<std::mutex> lock(mutex_);
    ON_SCOPE_EXIT(0) { gst_buffer_unref(buffer); };
    if (isFlushed_ || !isStart_) {
        MEDIA_LOGD("isFlush %{public}d isStart %{public}d", isFlushed_, isStart_);
        return GST_CODEC_FLUSH;
    }
    std::shared_ptr<HdiBufferWrap> codecBuffer = nullptr;
    codecBuffer = GetCodecBuffer(buffer);
    CHECK_AND_RETURN_RET_LOG(codecBuffer != nullptr, GST_CODEC_ERROR, "Push buffer failed");
    auto ret = HdiFillThisBuffer(handle_, &codecBuffer->hdiBuffer);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "FillThisBuffer failed");
    return GST_CODEC_OK;
}

int32_t HdiOutBufferMgr::PullBuffer(GstBuffer **buffer)
{
    MEDIA_LOGD("Enter PullBuffer");
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, GST_CODEC_ERROR, "buffer is nullptr");
    std::unique_lock<std::mutex> lock(mutex_);
    bufferCond_.wait(lock, [this]() { return !mBuffers.empty() || isFlushed_ || !isStart_; });
    if (isFlushed_ || !isStart_) {
        MEDIA_LOGD("isFlush %{public}d isStart %{public}d", isFlushed_, isStart_);
        return GST_CODEC_FLUSH;
    }
    if (!mBuffers.empty()) {
        MEDIA_LOGD("mBuffers %{public}zu, available %{public}zu", mBuffers.size(), availableBuffers_.size());
        (*buffer) = mBuffers.front();
        mBuffers.pop_front();
    }
    if ((*buffer) == nullptr) {
        MEDIA_LOGD("buffer is in eos");
        return GST_CODEC_EOS;
    }
    return GST_CODEC_OK;
}

int32_t HdiOutBufferMgr::FreeBuffers()
{
    MEDIA_LOGD("FreeBuffers");
    std::unique_lock<std::mutex> lock(mutex_);
    freeCond_.wait(lock, [this]() { return availableBuffers_.size() == mPortDef_.nBufferCountActual; });
    FreeCodecBuffers();
    std::for_each(mBuffers.begin(), mBuffers.end(), [&](GstBuffer *buffer) { gst_buffer_unref(buffer); });
    EmptyList(mBuffers);
    return GST_CODEC_OK;
}

int32_t HdiOutBufferMgr::CodecBufferAvailable(const OmxCodecBuffer *buffer)
{
    MEDIA_LOGD("codecBufferAvailable");
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, GST_CODEC_ERROR, "FillBufferDone failed");
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGD("mBuffers %{public}zu, available %{public}zu", mBuffers.size(), availableBuffers_.size());
    for (auto iter = codingBuffers_.begin(); iter != codingBuffers_.end(); ++iter) {
        if (iter->first != nullptr && iter->first->hdiBuffer.bufferId == buffer->bufferId) {
            availableBuffers_.push_back(iter->first);
            if (buffer->flag & OMX_BUFFERFLAG_EOS) {
                MEDIA_LOGD("Bufferavailable, but buffer is eos");
                mBuffers.push_back(nullptr);
                gst_buffer_unref(iter->second);
            } else {
                mBuffers.push_back(iter->second);
                gst_buffer_resize(iter->second, buffer->offset, buffer->filledLen);
            }
            (void)codingBuffers_.erase(iter);
            break;
        }
    }
    NotifyAvailable();
    bufferCond_.notify_all();
    return GST_CODEC_OK;
}
}  // namespace Media
}  // namespace OHOS
