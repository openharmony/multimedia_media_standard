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

#include "hdi_venc_params_mgr.h"
#include <hdf_base.h>
#include "media_log.h"
#include "media_errors.h"
#include "gst_venc_base.h"
#include "hdi_codec_util.h"
#include "hdi_codec.h"
#include "avcodec_info.h"
namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "HdiVencParamsMgr"};
    constexpr uint32_t OMX_FRAME_RATE_MOVE = 16; // hdi frame rate need move 16
    constexpr uint32_t MSEC_PER_S = 1000;
}

namespace OHOS {
namespace Media {
const std::map<int32_t, OMX_VIDEO_CONTROLRATETYPE> BITRATE_MODE_MAP = {
    {CBR, OMX_Video_ControlRateConstant},
    {VBR, OMX_Video_ControlRateVariable},
};

const std::map<int32_t, OMX_VIDEO_AVCPROFILETYPE> AVC_PROFILE_MAP = {
    {AVC_PROFILE_BASELINE, OMX_VIDEO_AVCProfileBaseline},
    {AVC_PROFILE_EXTENDED, OMX_VIDEO_AVCProfileExtended},
    {AVC_PROFILE_HIGH, OMX_VIDEO_AVCProfileHigh},
    {AVC_PROFILE_HIGH_10, OMX_VIDEO_AVCProfileHigh10},
    {AVC_PROFILE_HIGH_422, OMX_VIDEO_AVCProfileHigh422},
    {AVC_PROFILE_HIGH_444, OMX_VIDEO_AVCProfileHigh444},
    {AVC_PROFILE_MAIN, OMX_VIDEO_AVCProfileMain},
};

const std::map<int32_t, OMX_VIDEO_AVCLEVELTYPE> AVC_LEVEL_MAP = {
    {AVC_LEVEL_1, OMX_VIDEO_AVCLevel1},
    {AVC_LEVEL_1b, OMX_VIDEO_AVCLevel1b},
    {AVC_LEVEL_11, OMX_VIDEO_AVCLevel11},
    {AVC_LEVEL_12, OMX_VIDEO_AVCLevel12},
    {AVC_LEVEL_13, OMX_VIDEO_AVCLevel13},
    {AVC_LEVEL_2, OMX_VIDEO_AVCLevel2},
    {AVC_LEVEL_21, OMX_VIDEO_AVCLevel21},
    {AVC_LEVEL_22, OMX_VIDEO_AVCLevel22},
    {AVC_LEVEL_3, OMX_VIDEO_AVCLevel3},
    {AVC_LEVEL_31, OMX_VIDEO_AVCLevel31},
    {AVC_LEVEL_32, OMX_VIDEO_AVCLevel32},
    {AVC_LEVEL_4, OMX_VIDEO_AVCLevel4},
    {AVC_LEVEL_41, OMX_VIDEO_AVCLevel41},
    {AVC_LEVEL_42, OMX_VIDEO_AVCLevel42},
    {AVC_LEVEL_5, OMX_VIDEO_AVCLevel5},
    {AVC_LEVEL_51, OMX_VIDEO_AVCLevel51},
};

HdiVencParamsMgr::HdiVencParamsMgr()
    : handle_(nullptr)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

HdiVencParamsMgr::~HdiVencParamsMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void HdiVencParamsMgr::Init(CodecComponentType *handle,
    const OMX_PORT_PARAM_TYPE &portParam, const CompVerInfo &verInfo)
{
    handle_ = handle;
    verInfo_ = verInfo;
    InitParam(inPortDef_, verInfo_);
    InitParam(outPortDef_, verInfo_);
    InitHdiParam(videoFormat_, verInfo_);
    inPortDef_.nPortIndex = portParam.nStartPortNumber;
    outPortDef_.nPortIndex = portParam.nStartPortNumber + 1;
    videoFormat_.portIndex = portParam.nStartPortNumber + 1;
}

int32_t HdiVencParamsMgr::SetParameter(GstCodecParamKey key, GstElement *element)
{
    CHECK_AND_RETURN_RET_LOG(element != nullptr, GST_CODEC_ERROR, "Element is nullptr");
    switch (key) {
        case GST_VIDEO_INPUT_COMMON:
            SetInputVideoCommon(element);
            break;
        case GST_VIDEO_OUTPUT_COMMON:
            SetOutputVideoCommon(element);
            break;
        case GST_VIDEO_FORMAT:
            SetVideoFormat(element);
            break;
        case GST_VIDEO_SURFACE_INIT:
            VideoSurfaceInit(element);
            break;
        case GST_REQUEST_I_FRAME:
            RequestIFrame();
            break;
        case GST_VENDOR:
            MEDIA_LOGD("Set vendor property");
            break;
        case GST_DYNAMIC_BITRATE:
            SetDynamicBitrate(element);
            break;
        case GST_VIDEO_ENCODER_CONFIG:
            ConfigEncoderParams(element);
            break;
        default:
            break;
    }
    return GST_CODEC_OK;
}

int32_t HdiVencParamsMgr::ConfigEncoderParams(GstElement *element)
{
    GstVencBase *base = GST_VENC_BASE(element);
    InitBitRateMode(element);
    switch (base->compress_format) {
        case GST_AVC:
            return InitAvcParamters(element);
        default:
            break;
    }
    return GST_CODEC_OK;
}

int32_t HdiVencParamsMgr::SetDynamicBitrate(GstElement *element)
{
    GstVencBase *base = GST_VENC_BASE(element);
    OMX_VIDEO_CONFIG_BITRATETYPE bitrateConfig;
    InitParam(bitrateConfig, verInfo_);
    bitrateConfig.nPortIndex = outPortDef_.nPortIndex;
    bitrateConfig.nEncodeBitrate = base->bitrate;
    auto ret = HdiSetConfig(handle_, OMX_IndexConfigVideoBitrate, bitrateConfig);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiSetConfig failed");
    return GST_CODEC_OK;
}

int32_t HdiVencParamsMgr::SetInputVideoCommon(GstElement *element)
{
    GstVencBase *base = GST_VENC_BASE(element);
    MEDIA_LOGD("Set video frame rate %{public}d", base->frame_rate);
    inPortDef_.format.video.nFrameHeight = (uint32_t)base->height;
    inPortDef_.format.video.nFrameWidth = (uint32_t)base->width;
    inPortDef_.format.video.xFramerate = (uint32_t)(base->frame_rate) << OMX_FRAME_RATE_MOVE;
    inPortDef_.format.video.nSliceHeight = (uint32_t)base->nslice_height;
    inPortDef_.format.video.nStride = base->nstride;
    inPortDef_.nBufferSize = base->input.buffer_size;
    inPortDef_.nBufferCountActual = base->input.buffer_cnt;
    auto ret = HdiSetParameter(handle_, OMX_IndexParamPortDefinition, inPortDef_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiSetParameter failed");
    OMX_CONFIG_FRAMERATETYPE frameRateConfig;
    InitParam(frameRateConfig, verInfo_);
    frameRateConfig.nPortIndex = outPortDef_.nPortIndex;
    frameRateConfig.xEncodeFramerate = (uint32_t)(base->frame_rate) << OMX_FRAME_RATE_MOVE;
    HdiSetConfig(handle_, OMX_IndexConfigVideoFramerate, frameRateConfig);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiSetConfig failed");
    return GST_CODEC_OK;
}

int32_t HdiVencParamsMgr::SetOutputVideoCommon(GstElement *element)
{
    GstVencBase *base = GST_VENC_BASE(element);
    MEDIA_LOGD("Set video frame rate %{public}d", base->frame_rate);
    outPortDef_.format.video.nFrameHeight = (uint32_t)base->height;
    outPortDef_.format.video.nFrameWidth = (uint32_t)base->width;
    outPortDef_.format.video.xFramerate = (uint32_t)(base->frame_rate) << OMX_FRAME_RATE_MOVE;
    outPortDef_.nBufferCountActual = base->output.buffer_cnt;
    outPortDef_.nBufferSize = base->output.buffer_size;
    auto ret = HdiSetParameter(handle_, OMX_IndexParamPortDefinition, outPortDef_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiSetParameter failed");
    OMX_CONFIG_FRAMERATETYPE frameRateConfig;
    InitParam(frameRateConfig, verInfo_);
    frameRateConfig.nPortIndex = outPortDef_.nPortIndex;
    frameRateConfig.xEncodeFramerate = (uint32_t)(base->frame_rate) << OMX_FRAME_RATE_MOVE;
    HdiSetConfig(handle_, OMX_IndexConfigVideoFramerate, frameRateConfig);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiSetConfig failed");
    return GST_CODEC_OK;
}

int32_t HdiVencParamsMgr::SetVideoFormat(GstElement *element)
{
    GstVencBase *base = GST_VENC_BASE(element);
    videoFormat_.codecColorFormat = (uint32_t)HdiCodecUtil::FormatGstToHdi(base->format); // need to do
    videoFormat_.framerate = (uint32_t)(base->frame_rate) << OMX_FRAME_RATE_MOVE;
    auto ret = HdiSetParameter(handle_, OMX_IndexCodecVideoPortFormat, videoFormat_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiSetParameter failed");
    return GST_CODEC_OK;
}

int32_t HdiVencParamsMgr::GetInputVideoCommon(GstElement *element)
{
    GstVencBase *base = GST_VENC_BASE(element);
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

int32_t HdiVencParamsMgr::GetOutputVideoCommon(GstElement *element)
{
    GstVencBase *base = GST_VENC_BASE(element);
    auto ret = HdiGetParameter(handle_, OMX_IndexParamPortDefinition, outPortDef_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiGetParameter failed");
    base->output.min_buffer_cnt = outPortDef_.nBufferCountMin;
    base->output.buffer_cnt = outPortDef_.nBufferCountActual;
    base->output.buffer_size = outPortDef_.nBufferSize;
    base->output.height = (int32_t)outPortDef_.format.video.nFrameHeight;
    base->output.width = (int32_t)outPortDef_.format.video.nFrameWidth;
    base->output.frame_rate = (int32_t)outPortDef_.format.video.xFramerate;
    return GST_CODEC_OK;
}

int32_t HdiVencParamsMgr::GetVideoFormat(GstElement *element)
{
    GstVencBase *base = GST_VENC_BASE(element);
    auto ret = HdiGetParameter(handle_, OMX_IndexCodecVideoPortFormat, videoFormat_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiGetParameter failed");

    while (HdiGetParameter(handle_, OMX_IndexCodecVideoPortFormat, videoFormat_) == HDF_SUCCESS) {
        base->formats.push_back(HdiCodecUtil::FormatHdiToGst((PixelFormat)videoFormat_.codecColorFormat));
        videoFormat_.portIndex++;
    }
    return GST_CODEC_OK;
}

int32_t HdiVencParamsMgr::VideoSurfaceInit(GstElement *element)
{
    (void)element;
    MEDIA_LOGD("Enter surface init");
    SupportBufferType supportBufferTypes;
    InitHdiParam(supportBufferTypes, verInfo_);
    supportBufferTypes.portIndex = inPortDef_.nPortIndex;
    auto ret = HdiGetParameter(handle_, OMX_IndexParamSupportBufferType, supportBufferTypes); // need to do
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiGetParameter failed");
    if (!(supportBufferTypes.bufferTypes & CODEC_BUFFER_TYPE_DYNAMIC_HANDLE)) {
        MEDIA_LOGD("No CODEC_BUFFER_TYPE_DYNAMIC_HANDLE, support bufferType %{public}d",
            supportBufferTypes.bufferTypes);
        return GST_CODEC_ERROR;
    }

    UseBufferType useBufferTypes;
    InitHdiParam(useBufferTypes, verInfo_);
    useBufferTypes.portIndex = inPortDef_.nPortIndex;
    useBufferTypes.bufferType = CODEC_BUFFER_TYPE_DYNAMIC_HANDLE;
    ret = HdiSetParameter(handle_, OMX_IndexParamUseBufferType, useBufferTypes); // need to do
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiSetParameter failed");
    return GST_CODEC_OK;
}

int32_t HdiVencParamsMgr::RequestIFrame()
{
    MEDIA_LOGD("Request I frame");

    OMX_CONFIG_INTRAREFRESHVOPTYPE param;
    InitParam(param, verInfo_);
    param.nPortIndex = outPortDef_.nPortIndex;
    param.IntraRefreshVOP = OMX_TRUE;
    auto ret = HdiSetConfig(handle_, (OMX_INDEXTYPE)OMX_IndexConfigVideoIntraVOPRefresh, param);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "Set I frame Config Failed");

    return GST_CODEC_OK;
}

int32_t HdiVencParamsMgr::InitBitRateMode(GstElement *element)
{
    MEDIA_LOGD("Init bitrate");
    GstVencBase *base = GST_VENC_BASE(element);
    if (BITRATE_MODE_MAP.find(base->bitrate_mode) == BITRATE_MODE_MAP.end()) {
        ControlRateConstantQuality constantQualityConfig_ = {};
        InitHdiParam(constantQualityConfig_, verInfo_);
        constantQualityConfig_.portIndex = outPortDef_.nPortIndex;
        auto ret = HdiGetParameter(handle_, OMX_IndexParamControlRateConstantQuality, constantQualityConfig_);
        CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "OMX_IndexRateConstantQuality Failed");
        if (base->bitrate != 0) {
            constantQualityConfig_.qualityValue = (uint32_t)base->codec_quality;
        }
        ret = HdiSetParameter(handle_, OMX_IndexParamControlRateConstantQuality, constantQualityConfig_);
        CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "OMX_IndexRateConstantQuality Failed");
    } else {
        OMX_VIDEO_PARAM_BITRATETYPE bitrateConfig_ = {};
        InitParam(bitrateConfig_, verInfo_);
        bitrateConfig_.nPortIndex = outPortDef_.nPortIndex;
        bitrateConfig_.eControlRate = BITRATE_MODE_MAP.at(base->bitrate_mode);
        auto ret = HdiGetParameter(handle_, OMX_IndexParamVideoBitrate, bitrateConfig_);
        CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "OMX_IndexParamVideoBitrate Failed");
        if (base->bitrate != 0) {
            bitrateConfig_.nTargetBitrate = base->bitrate;
        }
        ret = HdiSetParameter(handle_, OMX_IndexParamVideoBitrate, bitrateConfig_);
        CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "OMX_IndexParamVideoBitrate Failed");
    }
    return GST_CODEC_OK;
}

void HdiVencParamsMgr::InitAvcCommonParamters(GstElement *element, OMX_VIDEO_PARAM_AVCTYPE &avcType)
{
    GstVencBase *base = GST_VENC_BASE(element);
    if (AVC_PROFILE_MAP.find(base->codec_profile) != AVC_PROFILE_MAP.end()) {
        avcType.eProfile = AVC_PROFILE_MAP.at(base->codec_profile);
    } else {
        avcType.eProfile = OMX_VIDEO_AVCProfileBaseline;
    }
    if (AVC_LEVEL_MAP.find(base->codec_level) != AVC_LEVEL_MAP.end()) {
        avcType.eLevel = AVC_LEVEL_MAP.at(base->codec_level);
    } else {
        avcType.eLevel = OMX_VIDEO_AVCLevel1;
    }
    avcType.nSliceHeaderSpacing = 0;
    avcType.bUseHadamard = OMX_TRUE;
    avcType.nRefIdx10ActiveMinus1 = 0;
    avcType.nRefIdx11ActiveMinus1 = 0;
    avcType.bEnableUEP = OMX_FALSE;
    avcType.bEnableFMO = OMX_FALSE;
    avcType.bEnableASO = OMX_FALSE;
    avcType.bEnableRS = OMX_FALSE;
    avcType.bFrameMBsOnly = OMX_TRUE;
    avcType.bMBAFF = OMX_FALSE;
    avcType.eLoopFilterMode = OMX_VIDEO_AVCLoopFilterEnable;
}

int32_t HdiVencParamsMgr::InitAvcParamters(GstElement *element)
{
    GstVencBase *base = GST_VENC_BASE(element);
    OMX_VIDEO_PARAM_AVCTYPE avcType;
    InitParam(avcType, verInfo_);
    avcType.nPortIndex = outPortDef_.nPortIndex;
    auto ret = HdiGetParameter(handle_, OMX_IndexParamVideoAvc, avcType);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "OMX_IndexParamVideoAvc Failed");
    InitAvcCommonParamters(element, avcType);

    if (avcType.eProfile == OMX_VIDEO_AVCProfileBaseline) {
        avcType.nBFrames = 0;
        avcType.nRefFrames = 1;
        avcType.nPFrames = (uint32_t)(base->frame_rate * base->i_frame_interval_new / MSEC_PER_S - 1);
        avcType.bEntropyCodingCABAC = OMX_FALSE;
        avcType.bWeightedPPrediction = OMX_FALSE;
        avcType.bconstIpred = OMX_FALSE;
        avcType.bDirect8x8Inference = OMX_FALSE;
        avcType.bDirectSpatialTemporal = OMX_FALSE;
        avcType.nCabacInitIdc = 0;
    } else {
        // need more paramters
        avcType.nBFrames = 0;
        // when have b frame default ref frame is 2
        avcType.nRefFrames = avcType.nBFrames == 0 ? 1 : 2;
        avcType.nPFrames =
            (uint32_t)(base->frame_rate * base->i_frame_interval_new / MSEC_PER_S) / (avcType.nBFrames + 1) - 1;
        avcType.bEntropyCodingCABAC = OMX_TRUE;
        avcType.bWeightedPPrediction = OMX_TRUE;
        avcType.bconstIpred = OMX_TRUE;
        avcType.bDirect8x8Inference = OMX_TRUE;
        avcType.bDirectSpatialTemporal = OMX_TRUE;
        avcType.nCabacInitIdc = 1;
    }
    avcType.nAllowedPictureTypes = OMX_VIDEO_PictureTypeI;
    if (avcType.nPFrames != 0) {
        avcType.nAllowedPictureTypes |= OMX_VIDEO_PictureTypeP;
    }
    if (avcType.nBFrames != 0) {
        avcType.nAllowedPictureTypes |= OMX_VIDEO_PictureTypeB;
    }
    ret = HdiSetParameter(handle_, OMX_IndexParamVideoAvc, avcType);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "OMX_IndexParamVideoAvc Failed");
    return GST_CODEC_OK;
}

int32_t HdiVencParamsMgr::GetParameter(GstCodecParamKey key, GstElement *element)
{
    CHECK_AND_RETURN_RET_LOG(element != nullptr, GST_CODEC_ERROR, "Element is nullptr");
    CHECK_AND_RETURN_RET_LOG(handle_ != nullptr, GST_CODEC_ERROR, "handle_ is nullptr");
    switch (key) {
        case GST_VIDEO_INPUT_COMMON:
            GetInputVideoCommon(element);
            break;
        case GST_VIDEO_OUTPUT_COMMON:
            GetOutputVideoCommon(element);
            break;
        case GST_VIDEO_FORMAT:
            SetVideoFormat(element);
            break;
        default:
            break;
    }
    return GST_CODEC_OK;
}
}  // namespace Media
}  // namespace OHOS
