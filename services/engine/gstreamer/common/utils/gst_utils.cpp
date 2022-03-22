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

#include "gst_utils.h"
#include "string_ex.h"
#include "media_errors.h"
#include "media_log.h"
#include "time_perf.h"

namespace {
    [[maybe_unused]] constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "GstUtils"};
}

namespace OHOS {
namespace Media {
bool MatchElementByMeta(
    const GstElement &elem, const std::string_view &metaKey, const std::vector<std::string_view> &expectedMetaFields)
{
    const gchar *metadata = gst_element_get_metadata(const_cast<GstElement *>(&elem), metaKey.data());
    if (metadata == nullptr) {
        return false;
    }

    std::vector<std::string> klassDesc;
    SplitStr(metadata, "/", klassDesc, false, true);

    size_t matchCnt = 0;
    for (auto &expectedField : expectedMetaFields) {
        for (auto &item : klassDesc) {
            if (item.compare(expectedField) == 0) {
                matchCnt += 1;
                break;
            }
        }
    }

    return matchCnt == expectedMetaFields.size();
}
} // namespace Media
} // namespace OHOS