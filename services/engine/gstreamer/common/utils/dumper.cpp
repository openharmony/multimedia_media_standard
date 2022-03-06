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
#include <mutex>
#include <sys/time.h>
#include <securec.h>
#include <unordered_map>
#include "directory_ex.h"
#include "media_errors.h"
#include "media_log.h"
#include "param_wrapper.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "GstDumper"};
    static std::mutex g_padBufCountMutex;
    static std::unordered_map<GstPad *, uint64_t> g_padBufCount = {};
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

GstPadProbeReturn Dumper::DumpGstBuffer(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    (void)user_data;
    CHECK_AND_RETURN_RET(pad != nullptr && info != nullptr, GST_PAD_PROBE_OK);

    if ((GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER) == 0) {
        return GST_PAD_PROBE_OK;
    }

    GstBuffer *buf = gst_pad_probe_info_get_buffer(info);
    CHECK_AND_RETURN_RET(buf != nullptr, GST_PAD_PROBE_OK);
    GstMapInfo mapInfo = GST_MAP_INFO_INIT;
    CHECK_AND_RETURN_RET(gst_buffer_map(buf, &mapInfo, GST_MAP_READ), GST_PAD_PROBE_OK);

    uint64_t bufSeq = 0;
    {
        std::lock_guard<std::mutex> lock(g_padBufCountMutex);
        if (g_padBufCount.count(pad) == 0) {
            g_padBufCount.emplace(pad, 0);
        }
        bufSeq = g_padBufCount[pad]++;
    }

    char fullPath[PATH_MAX] = { 0 };
    const char *format = "/data/media/dump/pad_%s_%s_buf_%" PRIu64 "";
    if (sprintf_s(fullPath, PATH_MAX, format, GST_DEBUG_PAD_NAME(pad), bufSeq) <= 0) {
        MEDIA_LOGE("dump buffer failed for 0x%{public}06" PRIXPTR ", pad is %{public}s:%{public}s",
                   FAKE_POINTER(buf), GST_DEBUG_PAD_NAME(pad));
        gst_buffer_unmap(buf, &mapInfo);
        return GST_PAD_PROBE_OK;
    }

    FILE *fp = fopen(fullPath, "wb");
    if (fp != nullptr) {
        (void)fwrite(mapInfo.data, mapInfo.size, 1, fp);
        (void)fflush(fp);
        (void)fclose(fp);
        fp = nullptr;
    } else {
        MEDIA_LOGE("open path failed, %{public}s", fullPath);
    }

    gst_buffer_unmap(buf, &mapInfo);

    MEDIA_LOGD("wrote buffer to %{public}s", fullPath);
    return GST_PAD_PROBE_OK;
}

void Dumper::AddDumpGstBufferProbe(GstElement *element, const gchar *padname)
{
    GstPad *pad = gst_element_get_static_pad(element, padname);
    CHECK_AND_RETURN(pad != nullptr);

    gulong ret = gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, &Dumper::DumpGstBuffer, nullptr, nullptr);
    if (ret == 0) {
        MEDIA_LOGE("add dump gst buffer probe to pad %{public}s:%{public}s failed", GST_DEBUG_PAD_NAME(pad));
    } else {
        MEDIA_LOGD("add dump gst buffer probe to pad %{public}s:%{public}s success", GST_DEBUG_PAD_NAME(pad));
    }

    gst_object_unref(pad);
}

bool Dumper::IsEnableDumpGstBuffer()
{
    int value = OHOS::system::GetIntParameter("sys.media.dump.gstbuffer", 0);
    if (value == 0) {
        return false;
    }

    return true;
}
} // namespace Media
} // namespace OHOS
