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

#include "hdi_venc_in_buffer_mgr.h"
#include <hdf_base.h>
#include <algorithm>
#include "media_log.h"
#include "media_errors.h"
#include "hdi_codec_util.h"
#include "buffer_type_meta.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "HdiVencInBufferMgr"};
}

namespace OHOS {
namespace Media {
HdiVencInBufferMgr::HdiVencInBufferMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

HdiVencInBufferMgr::~HdiVencInBufferMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

std::vector<std::shared_ptr<HdiBufferWrap>> HdiVencInBufferMgr::PreUseHandleMems(std::vector<GstBuffer *> &buffers)
{
    std::vector<std::shared_ptr<HdiBufferWrap>> preBuffers;
    auto ret = HdiGetParameter(handle_, OMX_IndexParamPortDefinition, mPortDef_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, preBuffers, "HdiGetParameter failed");
    CHECK_AND_RETURN_RET_LOG(buffers.size() == mPortDef_.nBufferCountActual, preBuffers, "BufferNum error");
    for (uint32_t i = 0; i < buffers.size(); ++i) {
        std::shared_ptr<HdiBufferWrap> codecBuffer = std::make_shared<HdiBufferWrap>();
        codecBuffer->hdiBuffer.size = sizeof(OmxCodecBuffer);
        codecBuffer->hdiBuffer.version = verInfo_.compVersion;
        codecBuffer->hdiBuffer.bufferType = CodecBufferType::CODEC_BUFFER_TYPE_DYNAMIC_HANDLE;
        codecBuffer->hdiBuffer.buffer = nullptr;
        preBuffers.push_back(codecBuffer);
    }
    return preBuffers;
}

int32_t HdiVencInBufferMgr::Preprocessing()
{
    int32_t ret = GST_CODEC_OK;
    UseHdiBuffers(preBuffers_);
    EmptyList(preBuffers_);
    return ret;
}

int32_t HdiVencInBufferMgr::UseBuffers(std::vector<GstBuffer *> buffers)
{
    CHECK_AND_RETURN_RET_LOG(!buffers.empty(), GST_CODEC_ERROR, "buffer num error");
    if (buffers[0] == nullptr) {
        enableNativeBuffer_ = true;
        preBuffers_ = PreUseHandleMems(buffers);
    } else {
        enableNativeBuffer_ = false;
        preBuffers_ = PreUseAshareMems(buffers);
    }
    return GST_CODEC_OK;
}

std::shared_ptr<HdiBufferWrap> HdiVencInBufferMgr::GetCodecBuffer(GstBuffer *buffer)
{
    MEDIA_LOGD("Enter GetCodecBuffer");
    if (enableNativeBuffer_) {
        GstBufferTypeMeta *bufferType = gst_buffer_get_buffer_type_meta(buffer);
        CHECK_AND_RETURN_RET_LOG(bufferType != nullptr, nullptr, "bufferType is nullptr");
        std::shared_ptr<HdiBufferWrap> codecBuffer = availableBuffers_.front();
        availableBuffers_.pop_front();
        codingBuffers_.push_back(std::make_pair(codecBuffer, buffer));
        gst_buffer_ref(buffer);
        codecBuffer->hdiBuffer.size = sizeof(OmxCodecBuffer);
        codecBuffer->hdiBuffer.version = verInfo_.compVersion;
        codecBuffer->hdiBuffer.bufferType = CodecBufferType::CODEC_BUFFER_TYPE_DYNAMIC_HANDLE;
        codecBuffer->hdiBuffer.fenceFd = bufferType->fenceFd;
        codecBuffer->hdiBuffer.buffer = reinterpret_cast<uint8_t *>(bufferType->buf);
        codecBuffer->hdiBuffer.bufferLen = bufferType->bufLen;
        codecBuffer->hdiBuffer.pts = GST_BUFFER_PTS(buffer);
        return codecBuffer;
    }
    return HdiBufferMgr::GetCodecBuffer(buffer);
}
}  // namespace Media
}  // namespace OHOS
