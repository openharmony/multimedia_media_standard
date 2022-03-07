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

#ifndef PROCESSOR_VDEC_IMPL_H
#define PROCESSOR_VDEC_IMPL_H

#include "processor_base.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class ProcessorVdecImpl : public ProcessorBase, public NoCopyable {
public:
    ProcessorVdecImpl();
    ~ProcessorVdecImpl() override;

    std::shared_ptr<ProcessorConfig> GetInputPortConfig() override;
    std::shared_ptr<ProcessorConfig> GetOutputPortConfig() override;

protected:
    int32_t ProcessMandatory(const Format &format) override;
    int32_t ProcessOptional(const Format &format) override;

private:
    int32_t width_ = 0;
    int32_t height_ = 0;
    int32_t pixelFormat_ = 0;
    int32_t frameRate_ = 0;
    std::string gstPixelFormat_;
    int32_t maxInputSize_ = 0;
};
} // namespace Media
} // namespace OHOS
#endif // PROCESSOR_VDEC_IMPL_H