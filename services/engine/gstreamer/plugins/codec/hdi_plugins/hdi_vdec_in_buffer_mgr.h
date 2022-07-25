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

#ifndef HDI_VDEC_IN_BUFFER_MGR_H
#define HDI_VDEC_IN_BUFFER_MGR_H

#include "hdi_in_buffer_mgr.h"

namespace OHOS {
namespace Media {
class HdiVdecInBufferMgr : public HdiInBufferMgr {
public:
    HdiVdecInBufferMgr();
    ~HdiVdecInBufferMgr() override;
    int32_t UseBuffers(std::vector<GstBuffer*> buffers) override;
    int32_t Preprocessing() override;
};
} // namespace Media
} // namespace OHOS
#endif // HDI_VDEC_IN_BUFFER_MGR_H