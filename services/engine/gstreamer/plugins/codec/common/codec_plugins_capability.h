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

#ifndef CODEC_PLUGINS_CAPABILITY_H
#define CODEC_PLUGINS_CAPABILITY_H

#include "format.h"
#include "avcodec_info.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class __attribute__((visibility("default"))) CodecPluginsCapability : public NoCopyable {
public:
    ~CodecPluginsCapability();
    static CodecPluginsCapability& GetInstance();
    void RegisterCapabilitys(std::vector<CapabilityData> data);
    std::vector<CapabilityData> GetCodecPluginsCapability();

private:
    CodecPluginsCapability();
    std::vector<CapabilityData> capabilityDataArray_;
};
} // namespace Media
} // namespace OHOS
#endif // CODEC_PLUGINS_CAPABILITY_H
