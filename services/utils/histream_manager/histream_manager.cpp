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

#include "histream_manager.h"
#include <string>
#include <regex>
#include <map>
#include <queue>
#include <glib.h>
#include <gst/gst.h>
#include "param_wrapper.h"
#include "errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "HistreamManager"};
    const std::map<char, GstDebugLevel> LOG_LEVEL_TO_GST_LEVEL = {
        {'E', GST_LEVEL_ERROR},
        {'W', GST_LEVEL_WARNING},
        {'I', GST_LEVEL_INFO},
        {'D', GST_LEVEL_DEBUG},
        {'L', GST_LEVEL_LOG},
        {'T', GST_LEVEL_TRACE},
    };
    const std::string g_gstDftTag = "*";
    const std::vector<const gchar *> GST_ARGS = {
        "ohos_media_service",
        "--gst-disable-registry-fork",
        "--gst-plugin-path=/system/lib"
    };
}

namespace OHOS {
namespace Media  {
struct GstLogPrintInfo {
    GstDebugLevel level;
    const gchar *file;
    const gchar *function;
    gint line;
    GObject *object;
    const char *logMsg;
    const char *modeName;
};

static void GstLogPrint(const GstLogPrintInfo &info)
{
    const gchar *objName = GST_IS_OBJECT(info.object) ? GST_OBJECT_NAME(info.object) : " ";
    objName = objName ? objName : " ";
    OHOS::HiviewDFX::HiLogLabel gstLable = {LOG_CORE, LOG_DOMAIN, info.modeName};

    switch (info.level) {
        case GST_LEVEL_TRACE: // no break
        case GST_LEVEL_LOG:   // no break
        case GST_LEVEL_DEBUG:
            ::OHOS::HiviewDFX::HiLog::Debug(gstLable,
                "{%{public}s():%{public}d} [gst::%{public}s:%{public}" PRIXPTR "] %{public}s",
                info.function, info.line, objName, FAKE_POINTER(info.object), info.logMsg);
            break;
        case GST_LEVEL_INFO:
            ::OHOS::HiviewDFX::HiLog::Info(gstLable,
                "{%{public}s():%{public}d} [gst::%{public}s:%{public}" PRIXPTR "] %{public}s",
                info.function, info.line, objName, FAKE_POINTER(info.object), info.logMsg);
            break;
        case GST_LEVEL_WARNING:
            ::OHOS::HiviewDFX::HiLog::Warn(gstLable,
                "{%{public}s():%{public}d} [gst::%{public}s:%{public}" PRIXPTR "] %{public}s",
                info.function, info.line, objName, FAKE_POINTER(info.object), info.logMsg);
            break;
        case GST_LEVEL_ERROR:
            ::OHOS::HiviewDFX::HiLog::Error(gstLable,
                "{%{public}s():%{public}d} [gst::%{public}s:%{public}" PRIXPTR "] %{public}s",
                info.function, info.line, objName, FAKE_POINTER(info.object), info.logMsg);
            break;
        default:
            break;
    }
}

static void GstLogCallbackFunc(GstDebugCategory *category, GstDebugLevel level, const gchar *file,
    const gchar *function, gint line, GObject *object, GstDebugMessage *message, gpointer userData)
{
    const gchar *modeName = nullptr;
    (void)userData;

    if (!message) {
        return;
    }
    if (category) {
        if (level > gst_debug_category_get_threshold(category)) {
            return;
        }
        modeName = gst_debug_category_get_name(category);
    } else {
        if (level > gst_debug_get_default_threshold()) {
            return;
        }
    }

    if (!modeName) {
        modeName = LABEL.tag;
    }

    const gchar *logMsg = gst_debug_message_get (message);
    if (!logMsg) {
        return;
    }
    GstLogPrintInfo printInfo = {level, file ? file : " ", function ? function : " ", line, object, logMsg, modeName};
    GstLogPrint(printInfo);
}


static void GLogCallbackFunc(const gchar *logDomain, GLogLevelFlags level, const gchar *message, gpointer userData)
{
    const gchar *modeName = logDomain;
    (void)userData;

    if (!message || level > G_LOG_LEVEL_WARNING) {
        return;
    }

    if (!modeName) {
        modeName = LABEL.tag;
    }
    OHOS::HiviewDFX::HiLogLabel glibLable = {LOG_CORE, LOG_DOMAIN, modeName};

    switch (level) {
        case G_LOG_LEVEL_WARNING:
            ::OHOS::HiviewDFX::HiLog::Warn(glibLable, "[glog] %{public}s", message);
            break;
        case G_LOG_LEVEL_CRITICAL: // no break
        case G_LOG_LEVEL_ERROR:    // no break
        case G_LOG_FLAG_RECURSION:
            ::OHOS::HiviewDFX::HiLog::Error(glibLable, "[glog] %{public}s", message);
            break;
        default:
            break;
    }
}

static void EnableGLog(GLogFunc func)
{
    // map glib default log handler, and gstreamer log hander to GLogMap
    (void)g_log_set_default_handler(func, nullptr);
    (void)g_log_set_handler ("GStreamer", static_cast<GLogLevelFlags>(0xFFFFFFFF), func, nullptr);

    return;
}


static void SplitString(const std::string &input, const std::string &delimiter, std::queue<std::string> &strQueue)
{
    std::regex reg(delimiter);
    std::sregex_token_iterator pos(input.begin(), input.end(), reg, -1);
    decltype(pos) end;
    for (; pos != end; pos++) {
        strQueue.push(pos->str());
    }
}

static void SetGstLogLevelFromSysPara()
{
    std::string levelPara;
    int res = OHOS::system::GetStringParameter("sys.media.log.level", levelPara, "");
    if (res != 0 || levelPara.empty()) {
        gst_debug_set_default_threshold (GST_LEVEL_WARNING);
        MEDIA_LOGD("sys.media.log.level not find");
        return;
    }
    MEDIA_LOGD("sys.media.log.level=%{public}s", levelPara.c_str());

    static std::map<std::string, char> logTagLevelMap = {{g_gstDftTag, 'I'}, };
    std::queue<std::string> tagLevelQ;
    SplitString(levelPara, ",", tagLevelQ); // different module tag split by ","
    while (!tagLevelQ.empty()) {
        std::string tagLevel = tagLevelQ.front();
        tagLevelQ.pop();

        std::queue<std::string> item;
        SplitString(tagLevel, ":", item); // module format:"tagname:level"
        if (item.empty()) {
            continue;
        }
        std::string tag = item.front();
        item.pop();
        if (item.empty() || tag.size() >= 128) { // max tag size is 128
            continue;
        }
        if (logTagLevelMap.size() >= 512 && logTagLevelMap.count(tag) == 0) { // 512 is max tag number
            continue;
        }

        std::string level = item.front();
        item.pop();
        if (level.empty() || LOG_LEVEL_TO_GST_LEVEL.count(level.c_str()[0]) == 0) {
            continue;
        }

        logTagLevelMap[tag] = level.c_str()[0];
        MEDIA_LOGI("logPrara:%{public}s:%{public}c", tag.c_str(), level.c_str()[0]);
    }
    gst_debug_set_default_threshold (LOG_LEVEL_TO_GST_LEVEL.at(logTagLevelMap[g_gstDftTag]));
    for (auto &&[tag, levelCode] : logTagLevelMap) {
        if (tag == g_gstDftTag) {
            continue;
        }
        GstDebugCategory *cat = _gst_debug_get_category (tag.c_str());
        if (!cat) {
            continue;
        }
        gst_debug_category_set_threshold(cat, LOG_LEVEL_TO_GST_LEVEL.at(logTagLevelMap[tag]));
    }
}

static gchar*** CreateGstInitArgv()
{
    gchar ***argv = nullptr;
    argv = static_cast<gchar ***>(new (std::nothrow) (gchar **));
    if (!argv) {
        MEDIA_LOGI("new argv failed");
        return nullptr;
    }
    *argv = static_cast<gchar **>(new (std::nothrow) gchar *[GST_ARGS.size()]);
    if (!*argv) {
        delete argv;
        return nullptr;
    }
    for (size_t i = 0; i < GST_ARGS.size(); i++) {
        (*argv)[i] = const_cast<gchar *>(GST_ARGS[i]);
    }

    return argv;
}
static void DestroyGstInitArgv(gchar ***argv)
{
    if (!argv) {
        return;
    }
    if (!*argv) {
        return;
    }
    delete [] *argv;
    delete argv;
}

int32_t HistreamManager::SetUp()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (isInit_) {
        return ERR_OK;
    }

    EnableGLog(GLogCallbackFunc);
    gst_debug_remove_log_function(gst_debug_log_default);
    gst_debug_add_log_function (GstLogCallbackFunc, nullptr, nullptr);
    SetGstLogLevelFromSysPara();
    int32_t argc = static_cast<int32_t>(GST_ARGS.size());
    MEDIA_LOGI("SetUp HistreamManager argc=%{public}d", argc);
    gchar ***argv = CreateGstInitArgv();
    if (!argv) {
        return ERR_NO_MEMORY;
    }
    gst_init (&argc, argv);
    DestroyGstInitArgv(argv);
    isInit_ = true;

    MEDIA_LOGI("SetUp HistreamManager finished!");

    return ERR_OK;
}

void HistreamManager::UpdateLogLevel() const
{
    SetGstLogLevelFromSysPara();
}
}
}