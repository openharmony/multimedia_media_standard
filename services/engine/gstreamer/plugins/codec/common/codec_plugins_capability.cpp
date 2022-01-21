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

#include "codec_plugins_capability.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CodecPluginsCapability"};
}

namespace OHOS {
namespace Media {
CodecPluginsCapability& CodecPluginsCapability::GetInstance()
{
    static CodecPluginsCapability instance;
    return instance;
}

CodecPluginsCapability::CodecPluginsCapability()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

CodecPluginsCapability::~CodecPluginsCapability()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void CodecPluginsCapability::RegisterCapabilitys(std::vector<CapabilityData> data)
{
    capabilityDataArray_.insert(capabilityDataArray_.end(), data.begin(), data.end());
}

std::vector<CapabilityData> CodecPluginsCapability::GetCodecPluginsCapability()
{
    return capabilityDataArray_;
}
}
}