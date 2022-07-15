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

#ifndef HDI_IN_BUFFER_MGR_H
#define HDI_IN_BUFFER_MGR_H

#include <vector>
#include "hdi_buffer_mgr.h"

namespace OHOS {
namespace Media {
class HdiInBufferMgr : public HdiBufferMgr {
public:
    HdiInBufferMgr();
    ~HdiInBufferMgr() override;

    int32_t PushBuffer(GstBuffer *buffer) override;

    int32_t FreeBuffers() override;

    int32_t CodecBufferAvailable(const OmxCodecBuffer *buffer) override;

protected:
    std::vector<std::shared_ptr<HdiBufferWrap>> preBuffers_;
    std::shared_ptr<HdiBufferWrap> GetHdiEosBuffer();
};
} // namespace Media
} // namespace OHOS
#endif // HDI_IN_BUFFER_MGR_H
