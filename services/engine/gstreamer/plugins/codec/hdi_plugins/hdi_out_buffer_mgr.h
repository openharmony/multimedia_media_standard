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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KOUTD, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef HDI_OUT_BUFFER_MGR_H
#define HDI_OUT_BUFFER_MGR_H

#include <list>
#include "hdi_buffer_mgr.h"

namespace OHOS {
namespace Media {
struct GstBufferWrap {
    bool isEos = false;
    GstBuffer *gstBuffer = nullptr;
};
class HdiOutBufferMgr : public HdiBufferMgr {
public:
    HdiOutBufferMgr();
    ~HdiOutBufferMgr() override;
    int32_t Start() override;
    int32_t PushBuffer(GstBuffer *buffer) override;
    int32_t PullBuffer(GstBuffer **buffer) override;
    int32_t FreeBuffers() override;
    int32_t CodecBufferAvailable(const OmxCodecBuffer *buffer) override;

protected:
    std::list<GstBufferWrap> mBuffers;
};
} // namespace Media
} // namespace OHOS
#endif // HDI_OUT_BUFFER_MGR_H
