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

#include "recorder_element.h"
#include <algorithm>
#include <iterator>
#include <unordered_map>
#include "errors.h"
#include "media_log.h"
#include "recorder_private_param.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderElement"};
}

namespace OHOS {
namespace Media {
#define PARAM_TYPE_NAME_ITEM(paramType, captionString) { paramType, captionString }
static const std::unordered_map<uint32_t, std::string> PARAM_TYPE_NAME_MAP = {
    PARAM_TYPE_NAME_ITEM(VID_ENC_FMT, "video encode format"),
    PARAM_TYPE_NAME_ITEM(VID_RECTANGLE, "video size"),
    PARAM_TYPE_NAME_ITEM(VID_BITRATE, "video bitrate"),
    PARAM_TYPE_NAME_ITEM(VID_FRAMERATE, "video framerate"),
    PARAM_TYPE_NAME_ITEM(VID_CAPTURERATE, "video capture rate"),
    PARAM_TYPE_NAME_ITEM(AUD_ENC_FMT, "audio encode format"),
    PARAM_TYPE_NAME_ITEM(AUD_SAMPLERATE, "audio samplerate"),
    PARAM_TYPE_NAME_ITEM(AUD_CHANNEL, "audio channels"),
    PARAM_TYPE_NAME_ITEM(AUD_BITRATE, "audio bitrate"),
    PARAM_TYPE_NAME_ITEM(MAX_DURATION, "max record duration"),
    PARAM_TYPE_NAME_ITEM(MAX_SIZE, "max record size"),
    PARAM_TYPE_NAME_ITEM(OUT_PATH, "output path"),
    PARAM_TYPE_NAME_ITEM(OUT_FD, "out file descripter"),
    PARAM_TYPE_NAME_ITEM(NEXT_OUT_FD, "next out file descripter"),
    PARAM_TYPE_NAME_ITEM(OUTPUT_FORMAT, "output file format"),
};

int32_t RecorderElementFactory::RegisterElement(std::string key, ElementCreator creator)
{
    std::unique_lock<std::mutex> lock(tblMutex_);
    if (creatorTbl_.find(key) != creatorTbl_.end()) {
        MEDIA_LOGE("key %{public}s already registered !", key.c_str());
        return ERR_ALREADY_EXISTS;
    }

    creatorTbl_.emplace(key, creator);
    return ERR_OK;
}

std::shared_ptr<RecorderElement> RecorderElementFactory::CreateElement(
    std::string key, const RecorderElement::CreateParam &param)
{
    std::shared_ptr<RecorderElement> elem;
    {
        std::unique_lock<std::mutex> lock(tblMutex_);
        if (creatorTbl_.find(key) == creatorTbl_.end()) {
            MEDIA_LOGE("key %{public}s not registered !", key.c_str());
            return nullptr;
        }

        elem = creatorTbl_[key](param);
        if (elem == nullptr) {
            MEDIA_LOGE("create element for key(%{public}s) failed !", key.c_str());
            return nullptr;
        }
    }

    int32_t ret = elem->Init();
    if (ret != ERR_OK) {
        MEDIA_LOGE("init element for key(%{public}s) failed !", key.c_str());
        return nullptr;
    }

    return elem;
}

RecorderElement::RecorderElement(const CreateParam &createParam)
    : desc_(createParam.srcDesc), name_(createParam.name)
{
    MEDIA_LOGD("enter %{public}s ctor", name_.c_str());
}

RecorderElement::~RecorderElement()
{
    MEDIA_LOGD("enter %{public}s dtor", name_.c_str());
}

bool RecorderElement::CheckAllParamsConfiged(const std::set<int32_t>& expectedParams)
{
    std::set<int32_t> intersection;
    std::set_intersection(expectedParams.begin(), expectedParams.end(),
                          configedParams_.begin(), configedParams_.end(),
                          std::inserter(intersection, intersection.end()));
    if (intersection == expectedParams) {
        return true;
    }

    std::string errStr = "Fail ! Not all expected parameters are configured: ";
    for (auto &param : expectedParams) {
        auto iter = PARAM_TYPE_NAME_MAP.find(param);
        if (iter == PARAM_TYPE_NAME_MAP.end()) {
            errStr += "unknown param type:" + std::to_string(param);
        } else {
            errStr += iter->second;
        }
        errStr += ", ";
    }
    MEDIA_LOGE("%{public}s, %{public}s", name_.c_str(), errStr.c_str());

    return false;
}

bool RecorderElement::CheckAnyParamConfiged(const std::set<int32_t>& expectedParams)
{
    std::set<int32_t> intersection;
    std::set_intersection(expectedParams.begin(), expectedParams.end(),
                          configedParams_.begin(), configedParams_.end(),
                          std::inserter(intersection, intersection.end()));
    if (!intersection.empty()) {
        return true;
    }

    std::string errStr = "Fail ! No any one of expected parameters are configured: ";
    for (auto &param : expectedParams) {
        auto iter = PARAM_TYPE_NAME_MAP.find(param);
        if (iter == PARAM_TYPE_NAME_MAP.end()) {
            errStr += "unknown param type:" + std::to_string(param);
        } else {
            errStr += iter->second;
        }
        errStr += ", ";
    }
    MEDIA_LOGE("%{public}s, %{public}s", name_.c_str(), errStr.c_str());

    return false;
}
}
}