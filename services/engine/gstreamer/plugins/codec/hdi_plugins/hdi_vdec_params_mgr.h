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

#ifndef HDI_VDEC_PARAMS_MGR_H
#define HDI_VDEC_PARAMS_MGR_H

#include <memory>
#include <gst/gst.h>
#include "nocopyable.h"
#include "hdi_params_mgr.h"
#include "OMX_Core.h"
#include "OMX_Component.h"

namespace OHOS {
namespace Media {
class HdiVdecParamsMgr : public HdiParamsMgr {
public:
    HdiVdecParamsMgr();
    ~HdiVdecParamsMgr() override;
    void Init(CodecComponentType *handle, const OMX_PORT_PARAM_TYPE &portParam, const CompVerInfo &verInfo) override;
    int32_t SetParameter(GstCodecParamKey key, GstElement *element) override;
    int32_t GetParameter(GstCodecParamKey key, GstElement *element) override;
protected:
    int32_t SetInputVideoCommon(GstElement *element);
    int32_t SetOutputVideoCommon(GstElement *element);
    int32_t SetVideoFormat(GstElement *element);
    int32_t VideoSurfaceInit(GstElement *element);
    int32_t GetInputVideoCommon(GstElement *element);
    int32_t GetOutputVideoCommon(GstElement *element);
    int32_t GetVideoFormat(GstElement *element);
private:
    CodecComponentType *handle_ = nullptr;
    OMX_PARAM_PORTDEFINITIONTYPE inPortDef_ = {};
    OMX_PARAM_PORTDEFINITIONTYPE outPortDef_ = {};
    CodecVideoPortFormatParam videoFormat_ = {};
    CompVerInfo verInfo_ = {};
};
} // namespace Media
} // namespace OHOS
#endif // HDI_VDEC_PARAMS_MGR_H