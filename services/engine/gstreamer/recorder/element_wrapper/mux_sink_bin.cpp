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

#include "mux_sink_bin.h"
#include <unistd.h>
#include <fcntl.h>
#include <gst/gst.h>
#include "datetime_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "media_log.h"
#include "recorder_private_param.h"
#include "scope_guard.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MuxSinkBin"};
}

namespace OHOS {
namespace Media {
MuxSinkBin::~MuxSinkBin()
{
    MEDIA_LOGD("enter, dtor");
    (void)Reset();
}

int32_t MuxSinkBin::Init()
{
    ON_SCOPE_EXIT(0) {
        if (gstElem_ != nullptr) {
            gst_object_unref(gstElem_);
            gstElem_ = nullptr;
        }
    };

    gstElem_ = gst_element_factory_make("splitmuxsink", name_.c_str());
    if (gstElem_ == nullptr) {
        MEDIA_LOGE("Create splitmuxsink gst element failed !");
        return MSERR_INVALID_OPERATION;
    }

    gstSink_ = gst_element_factory_make("fdsink", "fdsink");
    if (gstSink_ == nullptr) {
        MEDIA_LOGE("Create fdsink gst element failed !");
        return MSERR_INVALID_OPERATION;
    }

    CANCEL_SCOPE_EXIT_GUARD(0);
    g_object_set(gstElem_, "sink", gstSink_, nullptr);
    return MSERR_OK;
}

int32_t MuxSinkBin::Configure(const RecorderParam &recParam)
{
    int32_t ret = MSERR_OK;
    switch (recParam.type) {
        case RecorderPrivateParamType::OUTPUT_FORMAT:
            ret = ConfigureOutputFormat(recParam);
            break;
        case RecorderPublicParamType::OUT_PATH:
            /* fallthrough */
        case RecorderPublicParamType::OUT_FD:
            ret = ConfigureOutputTarget(recParam);
            break;
        case RecorderPublicParamType::MAX_DURATION:
            ret = ConfigureMaxDuration(recParam);
            break;
        case RecorderPublicParamType::MAX_SIZE:
            ret = ConfigureMaxFileSize(recParam);
            break;
        default:
            break;
    }
    return ret;
}

int32_t MuxSinkBin::ConfigureOutputFormat(const RecorderParam &recParam)
{
    if (recParam.type != RecorderPrivateParamType::OUTPUT_FORMAT) {
        return MSERR_OK;
    }

    const OutputFormat &param = static_cast<const OutputFormat &>(recParam);
    if ((param.format_ == OutputFormatType::FORMAT_MPEG_4) || (param.format_ == OutputFormatType::FORMAT_M4A)) {
        int ret = CreateMuxerElement("mp4mux");
        CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);
    } else {
        MEDIA_LOGE("output format type unsupported currently, format: %{public}d", param.format_);
        return MSERR_INVALID_VAL;
    }

    MEDIA_LOGI("configure output format: %{public}d", param.format_);
    format_ = param.format_;
    MarkParameter(param.type);

    return MSERR_OK;
}

int32_t MuxSinkBin::ConfigureOutputTarget(const RecorderParam &recParam)
{
    isReg_ = false;
    if (recParam.type == RecorderPublicParamType::OUT_PATH) {
        const OutFilePath &param = static_cast<const OutFilePath &>(recParam);
        std::string realPath;
        if (!PathToRealPath(param.path, realPath)) {
            MEDIA_LOGE("Configured output path invalid: %{public}s, ignore !", param.path.c_str());
            return MSERR_INVALID_VAL;
        }
        struct stat s;
        if (stat(realPath.c_str(), &s) != 0) {
            MEDIA_LOGE("Configured output path invalid: %{public}s, ignore !", param.path.c_str());
            return MSERR_INVALID_VAL;
        }
        if (((s.st_mode & S_IFREG) == 0) && ((s.st_mode & S_IFDIR) == 0)) {
            MEDIA_LOGE("Configured output path invalid: %{public}s, ignore !", param.path.c_str());
            return MSERR_INVALID_VAL;
        }
        if ((s.st_mode & S_IFREG) == S_IFREG) {
            isReg_ = true;
        }
        outPath_ = realPath;
        MEDIA_LOGI("Configure output path ok: %{public}s", outPath_.c_str());
    }

    if (recParam.type == RecorderPublicParamType::OUT_FD) {
        const OutFd &param = static_cast<const OutFd &>(recParam);
        int flags = fcntl(param.fd, F_GETFL);
        if (flags == -1) {
            MEDIA_LOGE("Fail to get File Status Flags");
            return MSERR_INVALID_VAL;
        }
        if ((static_cast<unsigned int>(flags) & (O_RDWR | O_WRONLY)) == 0) {
            MEDIA_LOGE("File descriptor is not in read-write mode or write-only mode");
            return MSERR_INVALID_VAL;
        }
        MEDIA_LOGI("Configure output fd ok");
        if (outFd_ > 0) {
            (void)::close(outFd_);
        }
        outFd_ = dup(param.fd);
        g_object_set(gstSink_, "fd", outFd_, nullptr);
    }

    MarkParameter(recParam.type);
    return MSERR_OK;
}

int32_t MuxSinkBin::ConfigureMaxDuration(const RecorderParam &recParam)
{
    const MaxDuration &param = static_cast<const MaxDuration &>(recParam);
    if (param.duration <= 0) {
        MEDIA_LOGE("Invalid max record duration: %{public}d", param.duration);
        return MSERR_INVALID_VAL;
    }
    MEDIA_LOGI("Set max duration success: %{public}d", param.duration);

    MarkParameter(recParam.type);
    maxDuration_ = param.duration;
    return MSERR_OK;
}

int32_t MuxSinkBin::ConfigureMaxFileSize(const RecorderParam &recParam)
{
    const MaxFileSize &param = static_cast<const MaxFileSize &>(recParam);
    if (param.size <= 0) {
        MEDIA_LOGE("Invalid max record file size: (%{public}" PRId64 ")", param.size);
        return MSERR_INVALID_VAL;
    }
    MEDIA_LOGI("Set max filesize success: (%{public}" PRId64 ")", param.size);

    MarkParameter(recParam.type);
    maxSize_ = param.size;
    return MSERR_OK;
}

int32_t MuxSinkBin::CheckConfigReady()
{
    std::set<int32_t> expectedParam = { RecorderPrivateParamType::OUTPUT_FORMAT };
    bool configed = CheckAllParamsConfiged(expectedParam);
    CHECK_AND_RETURN_RET(configed == true, MSERR_INVALID_OPERATION);

    std::set<int32_t>({ RecorderPublicParamType::OUT_PATH, RecorderPublicParamType::OUT_FD }).swap(expectedParam);
    configed = CheckAnyParamConfiged(expectedParam);
    CHECK_AND_RETURN_RET(configed == true, MSERR_INVALID_OPERATION);

    return MSERR_OK;
}

int32_t MuxSinkBin::Prepare()
{
    int32_t ret = SetOutFilePath();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    return MSERR_OK;
}

int32_t MuxSinkBin::SetOutFilePath()
{
    if (outPath_.empty() || CheckParameter(RecorderPublicParamType::OUT_FD) || isReg_) {
        return MSERR_OK;
    }

    struct tm now;
    bool success = GetSystemCurrentTime(&now);
    if (!success) {
        MEDIA_LOGE("Get system current time failed !");
        return MSERR_INVALID_OPERATION;
    }

    std::string outFilePath = IncludeTrailingPathDelimiter(outPath_);
    std::string suffix;

    if (format_ == OutputFormatType::FORMAT_MPEG_4) {
        outFilePath += "video_";
        suffix = ".mp4";
    } else if (format_ == OutputFormatType::FORMAT_M4A) {
        outFilePath += "audio_";
        suffix = ".m4a";
    } else {
        MEDIA_LOGE("Output format type unsupported currently, format: %{public}d", format_);
        return MSERR_INVALID_VAL;
    }

    outFilePath += std::to_string(now.tm_year) + std::to_string(now.tm_mon) + std::to_string(now.tm_mday) + "_";
    outFilePath += std::to_string(now.tm_hour) + std::to_string(now.tm_min) + std::to_string(now.tm_sec);
    outFilePath += suffix;
    MEDIA_LOGI("out file path: %{public}s", outFilePath.c_str());

    outFd_ = open(outFilePath.c_str(), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if (outFd_ < 0) {
        MEDIA_LOGE("Open file failed! filePath: %{public}s", outFilePath.c_str());
        return MSERR_INVALID_OPERATION;
    }

    g_object_set(gstSink_, "fd", outFd_, nullptr);

    return MSERR_OK;
}

int32_t MuxSinkBin::Reset()
{
    if (outFd_ > 0) {
        (void)::close(outFd_);
        outFd_ = -1;
    }

    return MSERR_OK;
}

int32_t MuxSinkBin::SetParameter(const RecorderParam &recParam)
{
    (void)recParam;
    return MSERR_OK;
}

int32_t MuxSinkBin::CreateMuxerElement(const std::string &name)
{
    gstMuxer_ = gst_element_factory_make(name.c_str(), name.c_str());
    if (gstMuxer_ == nullptr) {
        MEDIA_LOGE("Create %{public}s gst element failed !", name.c_str());
        return MSERR_INVALID_OPERATION;
    }
    g_object_set(gstElem_, "muxer", gstMuxer_, nullptr);
    return MSERR_OK;
}

void MuxSinkBin::Dump()
{
    MEDIA_LOGI("file format = %{public}d, max duration = %{public}d, "
               "max size = %{public}" PRId64 ", fd = %{public}d, path = %{public}s",
               format_, maxDuration_,  maxSize_, outFd_, outPath_.c_str());
}

REGISTER_RECORDER_ELEMENT(MuxSinkBin);
}
}
