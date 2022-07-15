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

#include "hdi_vdec_params_mgr.h"
#include <hdf_base.h>
#include "media_log.h"
#include "media_errors.h"
#include "gst_vdec_base.h"
#include "hdi_codec_util.h"
#include "hdi_codec.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "HdiVdecParamsMgr"};
    constexpr uint32_t HDI_FRAME_RATE_MOVE = 16; // hdi frame rate need move 16
}

namespace OHOS {
namespace Media {
HdiVdecParamsMgr::HdiVdecParamsMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

HdiVdecParamsMgr::~HdiVdecParamsMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void HdiVdecParamsMgr::Init(CodecComponentType *handle,
    const OMX_PORT_PARAM_TYPE &portParam, const CompVerInfo &verInfo)
{
    handle_ = handle;
    verInfo_ = verInfo;
    InitParam(inPortDef_, verInfo_);
    InitParam(outPortDef_, verInfo_);
    InitParam(videoFormat_, verInfo_);
    inPortDef_.nPortIndex = portParam.nStartPortNumber;
    outPortDef_.nPortIndex = portParam.nStartPortNumber + 1;
    videoFormat_.nPortIndex = portParam.nStartPortNumber + 1;
}

int32_t HdiVdecParamsMgr::SetParameter(GstCodecParamKey key, GstElement *element)
{
    CHECK_AND_RETURN_RET_LOG(element != nullptr, GST_CODEC_ERROR, "Element is nullptr");
    switch (key) {
        case GST_VIDEO_INPUT_COMMON:
            return SetInputVideoCommon(element);
        case GST_VIDEO_OUTPUT_COMMON:
            return SetOutputVideoCommon(element);
        case GST_VIDEO_FORMAT:
            return SetVideoFormat(element);
        case GST_VIDEO_SURFACE_INIT:
            return VideoSurfaceInit(element);
        case GST_VENDOR:
            MEDIA_LOGD("Set vendor property");
            break;
        default:
            break;
    }
    return GST_CODEC_OK;
}

int32_t HdiVdecParamsMgr::SetInputVideoCommon(GstElement *element)
{
    MEDIA_LOGD("SetInputVideoCommon");
    GstVdecBase *base = GST_VDEC_BASE(element);
    inPortDef_.format.video.eCompressionFormat = HdiCodecUtil::CompressionGstToHdi(base->compress_format);
    inPortDef_.format.video.nFrameHeight = (uint32_t)base->height;
    inPortDef_.format.video.nFrameWidth = (uint32_t)base->width;
    inPortDef_.format.video.xFramerate = (uint32_t)(base->frame_rate) << HDI_FRAME_RATE_MOVE;
    inPortDef_.nBufferCountActual = base->input.buffer_cnt;
    MEDIA_LOGD("frame_rate %{public}d", base->frame_rate);
    auto ret = HdiSetParameter(handle_, OMX_IndexParamPortDefinition, inPortDef_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiSetParameter failed");
    return GST_CODEC_OK;
}

int32_t HdiVdecParamsMgr::SetOutputVideoCommon(GstElement *element)
{
    MEDIA_LOGD("SetOutputVideoCommon");
    GstVdecBase *base = GST_VDEC_BASE(element);
    outPortDef_.format.video.nFrameHeight = (uint32_t)base->height;
    outPortDef_.format.video.nFrameWidth = (uint32_t)base->width;
    outPortDef_.format.video.xFramerate = (uint32_t)(base->frame_rate) << HDI_FRAME_RATE_MOVE;
    outPortDef_.nBufferCountActual = base->output.buffer_cnt;
    MEDIA_LOGD("frame_rate %{public}d", base->frame_rate);
    auto ret = HdiSetParameter(handle_, OMX_IndexParamPortDefinition, outPortDef_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiSetParameter failed");
    return GST_CODEC_OK;
}

int32_t HdiVdecParamsMgr::SetVideoFormat(GstElement *element)
{
    MEDIA_LOGD("SetVideoFormat");
    GstVdecBase *base = GST_VDEC_BASE(element);
    videoFormat_.eColorFormat = (OMX_COLOR_FORMATTYPE)HdiCodecUtil::FormatGstToOmx(base->format); // need to do
    MEDIA_LOGD("videoFormat_.eColorFormat %{public}d", videoFormat_.eColorFormat);
    auto ret = HdiSetParameter(handle_, OMX_IndexParamVideoPortFormat, videoFormat_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiSetParameter failed");
    return GST_CODEC_OK;
}

int32_t HdiVdecParamsMgr::GetInputVideoCommon(GstElement *element)
{
    MEDIA_LOGD("GetInputVideoCommon");
    GstVdecBase *base = GST_VDEC_BASE(element);
    auto ret = HdiGetParameter(handle_, OMX_IndexParamPortDefinition, inPortDef_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiGetParameter failed");
    base->input.min_buffer_cnt = inPortDef_.nBufferCountMin;
    base->input.buffer_cnt = inPortDef_.nBufferCountActual;
    base->input.buffer_size = inPortDef_.nBufferSize;
    base->input.height = (int32_t)inPortDef_.format.video.nFrameHeight;
    base->input.width = (int32_t)inPortDef_.format.video.nFrameWidth;
    base->input.frame_rate = (int32_t)inPortDef_.format.video.xFramerate;
    return GST_CODEC_OK;
}

int32_t HdiVdecParamsMgr::GetOutputVideoCommon(GstElement *element)
{
    MEDIA_LOGD("GetOutputVideoCommon");
    GstVdecBase *base = GST_VDEC_BASE(element);
    auto ret = HdiGetParameter(handle_, OMX_IndexParamPortDefinition, outPortDef_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiGetParameter failed");
    base->output.min_buffer_cnt = outPortDef_.nBufferCountMin;
    base->output.buffer_cnt = outPortDef_.nBufferCountActual;
    base->output.buffer_size = outPortDef_.nBufferSize;
    base->output.height = (int32_t)outPortDef_.format.video.nFrameHeight;
    base->output.width = (int32_t)outPortDef_.format.video.nFrameWidth;
    base->output.frame_rate = (int32_t)outPortDef_.format.video.xFramerate;
    base->stride = outPortDef_.format.video.nStride;
    base->stride_height = (int32_t)outPortDef_.format.video.nSliceHeight;
    MEDIA_LOGD("frame width %{public}d height %{public}d nStride %{public}d nSliceHeight %{public}d",
        base->output.width, base->output.height, base->stride, base->stride_height);
    base->rect.width = base->output.width;
    base->rect.height = base->output.height;

    return GST_CODEC_OK;
}

int32_t HdiVdecParamsMgr::GetVideoFormat(GstElement *element)
{
    MEDIA_LOGD("GetVideoFormat");
    GstVdecBase *base = GST_VDEC_BASE(element);
    auto ret = HdiGetParameter(handle_, OMX_IndexParamVideoPortFormat, videoFormat_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiGetParameter failed");

    while (HdiGetParameter(handle_, OMX_IndexParamVideoPortFormat, videoFormat_) == HDF_SUCCESS) {
        base->formats.push_back(HdiCodecUtil::FormatOmxToGst(videoFormat_.eColorFormat)); // need to do
        videoFormat_.nIndex++;
    }
    return GST_CODEC_OK;
}

int32_t HdiVdecParamsMgr::VideoSurfaceInit(GstElement *element)
{
    MEDIA_LOGD("VideoSurfaceInit, inport %{public}d outport %{public}d", inPortDef_.nPortIndex, outPortDef_.nPortIndex);
    SupportBufferType supportBufferTypes;
    InitHdiParam(supportBufferTypes, verInfo_);
    supportBufferTypes.portIndex = outPortDef_.nPortIndex;
    auto ret = HdiGetParameter(handle_, OMX_IndexParamSupportBufferType, supportBufferTypes); // need to do
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiGetParameter failed");
    if (!(supportBufferTypes.bufferTypes & CODEC_BUFFER_TYPE_HANDLE)) {
        MEDIA_LOGD("No CODEC_BUFFER_TYPE_HANDLE, support bufferType %{public}d", supportBufferTypes.bufferTypes);
        return GST_CODEC_ERROR;
    }

    UseBufferType useBufferTypes;
    InitHdiParam(useBufferTypes, verInfo_);
    useBufferTypes.portIndex = outPortDef_.nPortIndex;
    useBufferTypes.bufferType = CODEC_BUFFER_TYPE_HANDLE;
    ret = HdiSetParameter(handle_, OMX_IndexParamUseBufferType, useBufferTypes); // need to do
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiSetParameter failed");

    GetBufferHandleUsageParams usageParams;
    InitHdiParam(usageParams, verInfo_);
    usageParams.portIndex = outPortDef_.nPortIndex;
    ret = HdiGetParameter(handle_, OMX_IndexParamGetBufferHandleUsage, usageParams); // need to do
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiSetParameter failed");

    GstVdecBase *base = GST_VDEC_BASE(element);
    base->usage = usageParams.usage;
    MEDIA_LOGD("Usage %{public}d", base->usage);
    return GST_CODEC_OK;
}

int32_t HdiVdecParamsMgr::GetParameter(GstCodecParamKey key, GstElement *element)
{
    CHECK_AND_RETURN_RET_LOG(element != nullptr, GST_CODEC_ERROR, "Element is nullptr");
    CHECK_AND_RETURN_RET_LOG(handle_ != nullptr, GST_CODEC_ERROR, "Handle is nullptr");
    switch (key) {
        case GST_VIDEO_INPUT_COMMON:
            return GetInputVideoCommon(element);
        case GST_VIDEO_OUTPUT_COMMON:
            return GetOutputVideoCommon(element);
        case GST_VIDEO_FORMAT:
            return GetVideoFormat(element);
        default:
            break;
    }
    return GST_CODEC_OK;
}
}  // namespace Media
}  // namespace OHOS
