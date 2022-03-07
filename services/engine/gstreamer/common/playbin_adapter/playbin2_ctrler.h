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

#ifndef PLAYBIN2_CTRLER_H
#define PLAYBIN2_CTRLER_H

#include "playbin_ctrler_base.h"

namespace OHOS {
namespace Media {
class PlayBin2Ctrler : public PlayBinCtrlerBase {
public:
    using PlayBinCtrlerBase::PlayBinCtrlerBase;
    virtual ~PlayBin2Ctrler();

protected:
    int32_t OnInit() override;
};
} // namespace Media
} // namespace OHOS
#endif // PLAYBIN2_CTRLER_H