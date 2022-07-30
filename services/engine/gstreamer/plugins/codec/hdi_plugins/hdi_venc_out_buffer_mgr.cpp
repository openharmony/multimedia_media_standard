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

#include "hdi_venc_out_buffer_mgr.h"
#include "media_log.h"
#include "media_errors.h"
#include "hdi_codec_util.h"
#include "scope_guard.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "HdiVencOutBufferMgr"};
}

namespace OHOS {
namespace Media {
HdiVencOutBufferMgr::HdiVencOutBufferMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

HdiVencOutBufferMgr::~HdiVencOutBufferMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t HdiVencOutBufferMgr::UseBuffers(std::vector<GstBuffer *> buffers)
{
    MEDIA_LOGD("UseBuffers start");
    std::unique_lock<std::mutex> lock(mutex_);
    auto omxBuffers = PreUseAshareMems(buffers);
    int32_t ret = UseHdiBuffers(omxBuffers);
    for (auto buffer : buffers) {
        GstBufferWrap bufferWarp = {};
        bufferWarp.gstBuffer = buffer;
        mBuffers.push_back(bufferWarp);
        gst_buffer_ref(buffer);
    }
    MEDIA_LOGD("UseBuffers end");
    return ret;
}
}  // namespace Media
}  // namespace OHOS
