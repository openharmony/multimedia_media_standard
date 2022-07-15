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

#ifndef HDI_INIT_H_
#define HDI_INIT_H_

#include <gst/gst.h>
#include <memory>
#include <unordered_map>
#include <mutex>
#include "avcodec_info.h"
#include "codec_component_manager.h"

namespace OHOS {
namespace Media {
class HdiInit {
public:
    static HdiInit &GetInstance();
    ~HdiInit();
    int32_t GetHandle(CodecComponentType **component, uint32_t &id, std::string name,
        void *appData, CodecCallbackType *callbacks);
    int32_t FreeHandle(uint32_t id);
    std::vector<CapabilityData> GetCapabilitys();

private:
    HdiInit();
    using GetProfileLevelsFunc = std::map<int32_t, std::vector<int32_t>> (*)(CodecCompCapability &hdiCap);
    static std::string GetCodecName(std::string hdiName);
    static int32_t GetCodecType(CodecType hdiType);
    static std::string GetCodecMime(AvCodecRole &role);
    static std::vector<int32_t> GetCodecFormats(VideoPortCap &port);
    static std::vector<int32_t> GetOmxFormats(VideoPortCap &port);
    static std::vector<int32_t> GetBitrateMode(VideoPortCap &port);
    static std::map<ImgSize, Range> GetMeasuredFrameRate(VideoPortCap &port);
    static std::map<int32_t, std::vector<int32_t>> GetH264ProfileLevels(CodecCompCapability &hdiCap);
    static std::map<int32_t, std::vector<int32_t>> GetCodecProfileLevels(CodecCompCapability &hdiCap);
    void AddHdiCap(CodecCompCapability &hdiCap);
    void InitCaps();
    const static std::unordered_map<int32_t, GetProfileLevelsFunc> PROFILE_LEVEL_FUNC_MAP;
    std::vector<CapabilityData> capabilitys_;
    CodecComponentManager *mgr_ = nullptr;
};
}
}
#endif /* HDI_INIT_H_ */
