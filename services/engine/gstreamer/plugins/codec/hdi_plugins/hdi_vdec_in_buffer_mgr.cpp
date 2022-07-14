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

#include "hdi_vdec_in_buffer_mgr.h"
#include <algorithm>
#include <hdf_base.h>
#include "media_log.h"
#include "media_errors.h"
#include "hdi_codec_util.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "HdiVdecInBufferMgr"};
}

namespace OHOS {
namespace Media {
HdiVdecInBufferMgr::HdiVdecInBufferMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

HdiVdecInBufferMgr::~HdiVdecInBufferMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t HdiVdecInBufferMgr::UseBuffers(std::vector<GstBuffer *> buffers)
{
    MEDIA_LOGD("Enter UseBuffers");
    preBuffers_ = PreUseAshareMems(buffers);
    return GST_CODEC_OK;
}

int32_t HdiVdecInBufferMgr::Preprocessing()
{
    MEDIA_LOGD("Enter Preprocessing");
    std::unique_lock<std::mutex> lock(mutex_);
    int32_t ret = UseHdiBuffers(preBuffers_);
    EmptyList(preBuffers_);
    return ret;
}
}  // namespace Media
}  // namespace OHOS
