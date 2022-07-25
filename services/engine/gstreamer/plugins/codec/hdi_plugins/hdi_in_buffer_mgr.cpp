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

#include "hdi_in_buffer_mgr.h"
#include <algorithm>
#include <hdf_base.h>
#include "media_log.h"
#include "media_errors.h"
#include "hdi_codec_util.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "HdiInBufferMgr"};
}

namespace OHOS {
namespace Media {
HdiInBufferMgr::HdiInBufferMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

HdiInBufferMgr::~HdiInBufferMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    EmptyList(preBuffers_);
}

std::shared_ptr<HdiBufferWrap> HdiInBufferMgr::GetHdiEosBuffer()
{
    std::shared_ptr<HdiBufferWrap> codecBuffer = nullptr;
    if (!availableBuffers_.empty()) {
        MEDIA_LOGD("Init eos buffer");
        codecBuffer = availableBuffers_.front();
        availableBuffers_.pop_front();
        codecBuffer->hdiBuffer.filledLen = 0;
        codecBuffer->hdiBuffer.flag |= OMX_BUFFERFLAG_EOS;
        codingBuffers_.push_back(std::make_pair(codecBuffer, nullptr));
    }
    return codecBuffer;
}

int32_t HdiInBufferMgr::PushBuffer(GstBuffer *buffer)
{
    MEDIA_LOGD("PushBuffer start");
    std::unique_lock<std::mutex> lock(mutex_);
    bufferCond_.wait(lock, [this]() {return !availableBuffers_.empty() || isFlushed_ || !isStart_;});
    if (isFlushed_ || !isStart_) {
        return GST_CODEC_FLUSH;
    }
    std::shared_ptr<HdiBufferWrap> codecBuffer = nullptr;
    if (buffer == nullptr) {
        codecBuffer = GetHdiEosBuffer();
    } else {
        codecBuffer = GetCodecBuffer(buffer);
    }
    CHECK_AND_RETURN_RET_LOG(codecBuffer != nullptr, GST_CODEC_ERROR, "Push buffer failed");
    MEDIA_LOGD("id %{public}d fillLen %{public}d", codecBuffer->hdiBuffer.bufferId, codecBuffer->hdiBuffer.filledLen);
    auto ret = HdiEmptyThisBuffer(handle_, &codecBuffer->hdiBuffer);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "EmptyThisBuffer failed");
    MEDIA_LOGD("PushBuffer end");
    return GST_CODEC_OK;
}

int32_t HdiInBufferMgr::FreeBuffers()
{
    MEDIA_LOGD("Enter FreeBuffers");
    std::unique_lock<std::mutex> lock(mutex_);
    if (!preBuffers_.empty()) {
        EmptyList(preBuffers_);
        return GST_CODEC_OK;
    }
    freeCond_.wait(lock, [this]() { return availableBuffers_.size() == mPortDef_.nBufferCountActual; });
    FreeCodecBuffers();
    return GST_CODEC_OK;
}

int32_t HdiInBufferMgr::CodecBufferAvailable(const OmxCodecBuffer *buffer)
{
    MEDIA_LOGD("Enter CodecBufferAvailable");
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, GST_CODEC_ERROR, "EmptyBufferDone failed");
    std::unique_lock<std::mutex> lock(mutex_);
    for (auto iter = codingBuffers_.begin(); iter != codingBuffers_.end(); ++iter) {
        if (iter->first != nullptr && iter->first->hdiBuffer.bufferId == buffer->bufferId) {
            availableBuffers_.push_back(iter->first);
            iter->first->hdiBuffer.flag = 0;
            gst_buffer_unref(iter->second);
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
