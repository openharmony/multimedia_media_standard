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

#ifndef SRC_SURFACE_IMPL_H
#define SRC_SURFACE_IMPL_H

#include "src_base.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class SrcSurfaceImpl : public SrcBase, public NoCopyable {
public:
    SrcSurfaceImpl();
    ~SrcSurfaceImpl() override;

    int32_t Init() override;
    int32_t Configure(const std::shared_ptr<ProcessorConfig> &config) override;
    sptr<Surface> CreateInputSurface(const std::shared_ptr<ProcessorConfig> &inputConfig) override;
    int32_t SetParameter(const Format &format) override;
    int32_t NotifyEos() override;
};
} // namespace Media
} // namespace OHOS
#endif // SRC_SURFACE_IMPL_H