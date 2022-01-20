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

#ifndef FORMAT_PROCESSOR_BASE_H
#define FORMAT_PROCESSOR_BASE_H

#include "codec_common.h"
#include "format.h"

namespace OHOS {
namespace Media {
class ProcessorBase {
public:
    virtual ~ProcessorBase() = default;
    int32_t Init(const CodecMimeType &name, bool isSoftWare);
    int32_t DoProcess(const Format &format);
    virtual std::shared_ptr<ProcessorConfig> GetInputPortConfig() = 0;
    virtual std::shared_ptr<ProcessorConfig> GetOutputPortConfig() = 0;

protected:
    virtual int32_t ProcessMandatory(const Format &format) = 0;
    virtual int32_t ProcessOptional(const Format &format) = 0;

    CodecMimeType codecName_ = CODEC_MIMIE_TYPE_AUDIO_AAC;
    bool isSoftWare_ = false;
private:
    int32_t ProcessVendor(const Format &format);
};
} // Media
} // OHOS
#endif // FORMAT_PROCESSOR_BASE_H