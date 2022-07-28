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

#ifndef HDI_PARAMS_MGR_H
#define HDI_PARAMS_MGR_H

#include <memory>
#include <gst/gst.h>
#include <OMX_Core.h>
#include <OMX_Component.h>
#include "nocopyable.h"
#include "i_codec_params_mgr.h"
#include "codec_component_if.h"
#include "codec_omx_ext.h"

namespace OHOS {
namespace Media {
class HdiParamsMgr : public NoCopyable, public ICodecParamsMgr {
public:
    virtual ~HdiParamsMgr() = default;
    virtual void Init(CodecComponentType *handle,
        const OMX_PORT_PARAM_TYPE &portParam, const CompVerInfo &verInfo) = 0;
    int32_t SetParameter(GstCodecParamKey key, GstElement *element) = 0;
    int32_t GetParameter(GstCodecParamKey key, GstElement *element) = 0;
};
} // namespace Media
} // namespace OHOS
#endif // HDI_PARAMS_MGR_H