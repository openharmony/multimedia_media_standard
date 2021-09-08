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

#include "dumper.h"
#include <string>
#include <unistd.h>
#include <cstdio>
#include <sys/time.h>
#include <securec.h>
#include "directory_ex.h"
#include "media_errors.h"
#include "media_log.h"
#include "param_wrapper.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "GstDumper"};
}

namespace OHOS {
namespace Media {
void Dumper::DumpDotGraph(GstPipeline &pipeline, int32_t oldState, int32_t newState)
{
    if ((oldState < GST_STATE_VOID_PENDING) || (oldState > GST_STATE_PLAYING) ||
        (newState < GST_STATE_VOID_PENDING) || (newState > GST_STATE_PLAYING)) {
        MEDIA_LOGE("invalid state, oldState: %{public}d, newState: %{public}d", oldState, newState);
        return;
    }

    std::string dumpDir;
    int res = OHOS::system::GetStringParameter("sys.media.dump.dot.path", dumpDir, "");
    if (res != 0 || dumpDir.empty()) {
        return;
    }

    std::string realPath;
    CHECK_AND_RETURN_LOG(PathToRealPath(dumpDir, realPath), "invalid dump path: %{public}s", dumpDir.c_str());

    struct timeval tv;
    int ret = gettimeofday(&tv, nullptr);
    CHECK_AND_RETURN_LOG(ret >= 0, "get time of day failed");

    constexpr int32_t secPerHour = 60 * 60;
    constexpr int32_t minutePerHour = 60;
    constexpr int32_t secPerMinute = 60;
    constexpr int32_t usecPerMSec = 1000;

    uint64_t hour = static_cast<uint64_t>(tv.tv_sec / secPerHour);
    uint32_t minute = static_cast<uint32_t>((tv.tv_sec / secPerMinute) % minutePerHour);
    uint32_t sec = static_cast<uint32_t>(tv.tv_sec % minutePerHour);
    uint32_t millsec = static_cast<uint32_t>(tv.tv_usec / usecPerMSec);

    const gchar *oldName = gst_element_state_get_name(static_cast<GstState>(oldState));
    const gchar *newName = gst_element_state_get_name(static_cast<GstState>(newState));

    char fullPath[PATH_MAX] = { 0 };
    const char *format = "%s/%" PRIu64 ".%u.%u.%u-media-pipeline.0x%06" PRIXPTR ".%s_%s.dot";
    ret = sprintf_s(fullPath, PATH_MAX, format, realPath.c_str(), hour, minute, sec, millsec,
                    FAKE_POINTER(&pipeline), oldName, newName);
    if (ret <= 0) {
        MEDIA_LOGE("dump dot failed for 0x%{public}06" PRIXPTR " %{public}s to %{public}s",
                   FAKE_POINTER(&pipeline), oldName, newName);
        return;
    }

    FILE *fp = fopen(fullPath, "wb");
    CHECK_AND_RETURN_LOG(fp != nullptr, "open path failed, %{public}s", fullPath);

    gchar *buf = gst_debug_bin_to_dot_data(GST_BIN(&pipeline), GST_DEBUG_GRAPH_SHOW_ALL);
    if (buf != nullptr) {
        (void)fputs(buf, fp);
        g_free(buf);
    } else {
        MEDIA_LOGD("buf is nullptr");
    }

    (void)fclose(fp);
    MEDIA_LOGD("wrote pipeline graph to : '%{public}s'", fullPath);
}
}
}
