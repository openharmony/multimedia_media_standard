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

    if (matchCnt == expectedMetaFields.size()) {
        return true;
    }

    return false;
}

DecoderPerf::~DecoderPerf()
{
    for (auto &item : probes_) {
        gst_pad_remove_probe(item.first, item.second);
    }
}

void DecoderPerf::Init()
{
    if (g_list_length(decoder_.srcpads) <= 0) {
        MEDIA_LOGE("%{public}s no srcpads, ignore", ELEM_NAME(&decoder_));
        return;
    }

    GList *padNode = g_list_first(decoder_.srcpads);
    CHECK_AND_RETURN(padNode != nullptr && padNode->data != nullptr);
    GstPad *pad = reinterpret_cast<GstPad *>(padNode->data);
    gulong probeId = gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_DATA_DOWNSTREAM, OnOutputArrived, this, nullptr);
    if (probeId == 0) {
        MEDIA_LOGW("add probe for %{public}s's pad %{public}s failed", ELEM_NAME(&decoder_), PAD_NAME(&pad));
        return;
    }
    probes_.push_back({pad, probeId});
}

GstPadProbeReturn DecoderPerf::OnOutputArrived(GstPad *pad, GstPadProbeInfo *info, gpointer thiz)
{
    (void)pad;
    CHECK_AND_RETURN_RET(info != nullptr, GST_PAD_PROBE_OK);
    CHECK_AND_RETURN_RET(thiz != nullptr, GST_PAD_PROBE_OK);

    auto type = static_cast<unsigned int>(info->type);
    if ((type & (GST_PAD_PROBE_TYPE_BUFFER | GST_PAD_PROBE_TYPE_BUFFER_LIST)) == 0) {
        return GST_PAD_PROBE_OK;
    }
    
    ASYNC_PERF_STOP(thiz, "DecodeFrame");
    ASYNC_PERF_START(thiz, "DecodeFrame");
    return GST_PAD_PROBE_OK;
}
}
}
