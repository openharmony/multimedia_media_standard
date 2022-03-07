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

#include "processor_base.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "ProcessorBase"};
}

namespace OHOS {
namespace Media {
int32_t ProcessorBase::Init(const InnerCodecMimeType &name, bool isSoftWare)
{
    codecName_ = name;
    isSoftWare_ = isSoftWare;
    return MSERR_OK;
}

int32_t ProcessorBase::DoProcess(const Format &format)
{
    CHECK_AND_RETURN_RET(ProcessMandatory(format) == MSERR_OK, MSERR_INVALID_VAL);
    CHECK_AND_RETURN_RET(ProcessOptional(format) == MSERR_OK, MSERR_INVALID_VAL);
    CHECK_AND_RETURN_RET(ProcessVendor(format) == MSERR_OK, MSERR_INVALID_VAL);
    return MSERR_OK;
}

int32_t ProcessorBase::ProcessVendor(const Format &format)
{
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
