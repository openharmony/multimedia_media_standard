/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "audio_converter.h"
#include <gst/gst.h>
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioConverter"};
}

namespace OHOS {
namespace Media {
int32_t AudioConverter::Init()
{
    gstElem_ = gst_element_factory_make("audioconvert", name_.c_str());
    if (gstElem_ == nullptr) {
        MEDIA_LOGE("Create audio converter gst element failed! sourceId: %{public}d", desc_.handle_);
        return MSERR_INVALID_OPERATION;
    }

    return MSERR_OK;
}

REGISTER_RECORDER_ELEMENT(AudioConverter);
} // namespace Media
} // namespace OHOS