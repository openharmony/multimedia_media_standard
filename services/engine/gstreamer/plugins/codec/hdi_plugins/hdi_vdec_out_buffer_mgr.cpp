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

#include "hdi_vdec_out_buffer_mgr.h"
#include <hdf_base.h>
#include "media_log.h"
#include "media_errors.h"
#include "hdi_codec_util.h"
#include "buffer_type_meta.h"
#include "scope_guard.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "HdiVdecOutBufferMgr"};
}

namespace OHOS {
namespace Media {
HdiVdecOutBufferMgr::HdiVdecOutBufferMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

HdiVdecOutBufferMgr::~HdiVdecOutBufferMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t HdiVdecOutBufferMgr::UseHandleMems(std::vector<GstBuffer *> &buffers)
{
    MEDIA_LOGD("Enter UseHandleMems");
    auto ret = HdiGetParameter(handle_, OMX_IndexParamPortDefinition, mPortDef_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiGetParameter failed");
    CHECK_AND_RETURN_RET_LOG(buffers.size() == mPortDef_.nBufferCountActual, GST_CODEC_ERROR, "BufferNum error");
    for (auto buffer : buffers) {
        GstBufferTypeMeta *bufferType = gst_buffer_get_buffer_type_meta(buffer);
        CHECK_AND_RETURN_RET_LOG(bufferType != nullptr, GST_CODEC_ERROR, "bufferType is nullptr");
        std::shared_ptr<HdiBufferWrap> codecBuffer = std::make_shared<HdiBufferWrap>();
        codecBuffer->buf = bufferType->buf;
        codecBuffer->hdiBuffer.size = sizeof(OmxCodecBuffer);
        codecBuffer->hdiBuffer.version = verInfo_.compVersion;
        codecBuffer->hdiBuffer.bufferType = CodecBufferType::CODEC_BUFFER_TYPE_HANDLE;
        codecBuffer->hdiBuffer.buffer = reinterpret_cast<uint8_t *>(bufferType->buf);
        codecBuffer->hdiBuffer.bufferLen = bufferType->bufLen;
        auto ret = handle_->UseBuffer(handle_, (uint32_t)mPortIndex_, &codecBuffer->hdiBuffer);
        CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "UseBuffer failed");
        availableBuffers_.push_back(codecBuffer);
        mBuffers.push_back(buffer);
        gst_buffer_ref(buffer);
    }
    return GST_CODEC_OK;
}

int32_t HdiVdecOutBufferMgr::UseBuffers(std::vector<GstBuffer *> buffers)
{
    MEDIA_LOGD("Enter UseBuffers");
    CHECK_AND_RETURN_RET_LOG(!buffers.empty(), GST_CODEC_ERROR, "buffers is empty");
    CHECK_AND_RETURN_RET_LOG(buffers[0] != nullptr, GST_CODEC_ERROR, "first buffer is empty");
    ON_SCOPE_EXIT(0) {
        std::for_each(buffers.begin(), buffers.end(), [&](GstBuffer *buffer) { gst_buffer_unref(buffer); });
    };
    GstBufferTypeMeta *bufferType = gst_buffer_get_buffer_type_meta(buffers[0]);
    CHECK_AND_RETURN_RET_LOG(bufferType != nullptr, GST_CODEC_ERROR, "bufferType is nullptr");
    enableNativeBuffer_ = bufferType->type == GstBufferType::BUFFER_TYPE_HANDLE ? true : false;
    int32_t ret = GST_CODEC_OK;
    if (enableNativeBuffer_) {
        ret = UseHandleMems(buffers);
    } else {
        auto omxBuffers = PreUseAshareMems(buffers);
        ret = UseHdiBuffers(omxBuffers);
    }
    MEDIA_LOGD("UseBuffer end");
    return GST_CODEC_OK;
}

void HdiVdecOutBufferMgr::UpdateCodecMeta(GstBufferTypeMeta *bufferType, std::shared_ptr<HdiBufferWrap> &codecBuffer)
{
    MEDIA_LOGD("Enter UpdateCodecMeta");
    CHECK_AND_RETURN_LOG(codecBuffer != nullptr, "bufferType is nullptr");
    CHECK_AND_RETURN_LOG(bufferType != nullptr, "bufferType is nullptr");
    if (enableNativeBuffer_) {
        codecBuffer->hdiBuffer.fenceFd = bufferType->fenceFd;
    } else {
        HdiBufferMgr::UpdateCodecMeta(bufferType, codecBuffer);
    }
}
}  // namespace Media
}  // namespace OHOS
