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
#include "media_errors.h"
#include "media_log.h"
#include "recorder_private_param.h"

namespace {
using namespace OHOS::Media;
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderElement"};
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
}

namespace OHOS {
namespace Media {
int32_t RecorderElementFactory::RegisterElement(const std::string &key, const ElementCreator creator)
{
    std::unique_lock<std::mutex> lock(tblMutex_);
    if (creatorTbl_.find(key) != creatorTbl_.end()) {
        MEDIA_LOGE("key %{public}s already registered !", key.c_str());
        return MSERR_INVALID_OPERATION;
    }

    (void)creatorTbl_.emplace(key, creator);
    return MSERR_OK;
}

std::shared_ptr<RecorderElement> RecorderElementFactory::CreateElement(
    const std::string key, const RecorderElement::CreateParam &param)
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
    if (ret != MSERR_OK) {
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
    if (gstElem_ != nullptr) {
        gst_object_unref(gstElem_);
    }

    MEDIA_LOGD("enter %{public}s dtor", name_.c_str());
}

int32_t RecorderElement::DrainAll(bool isDrain)
{
    MEDIA_LOGI("perform drainAll for %{public}s", name_.c_str());

    if (!isDrain) {
        auto block = [] (GstPad *pad, GstPadProbeInfo *info, gpointer userdata) {
            (void)userdata;
            if (pad == nullptr || info == nullptr) {
                return GST_PAD_PROBE_PASS;
            }
            MEDIA_LOGI("During flushing, pad %{public}s's probe is processing the probeInfo", GST_PAD_NAME(pad));
            if ((static_cast<unsigned int>(info->type) & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM) == 0) {
                return GST_PAD_PROBE_DROP;
            }
            return GST_PAD_PROBE_PASS;
        };

        GList *allSrcPads = gstElem_->srcpads;
        for (GList *padNode = g_list_first(allSrcPads); padNode != nullptr; padNode = padNode->next) {
            if (padNode->data == nullptr)  {
                continue;
            }
            MEDIA_LOGI("add probe for %{public}s...", name_.c_str());
            (void)gst_pad_add_probe((GstPad *)padNode->data, GST_PAD_PROBE_TYPE_DATA_DOWNSTREAM,
                                    (GstPadProbeCallback)block, nullptr, nullptr);
        }

        GstEvent *event = gst_event_new_flush_start();
        CHECK_AND_RETURN_RET(event != nullptr, MSERR_NO_MEMORY);
        (void)gst_element_send_event(gstElem_, event);

        event = gst_event_new_flush_stop(FALSE);
        CHECK_AND_RETURN_RET(event != nullptr, MSERR_NO_MEMORY);
        (void)gst_element_send_event(gstElem_, event);
    }

    GstEvent *event = gst_event_new_eos();
    CHECK_AND_RETURN_RET(event != nullptr, MSERR_NO_MEMORY);
    (void)gst_element_send_event(gstElem_, event);

    return MSERR_OK;
}

RecorderMsgProcResult RecorderElement::OnMessageReceived(GstMessage &rawMsg, RecorderMessage &prettyMsg)
{
    if (rawMsg.src != GST_OBJECT_CAST(gstElem_)) {
        return RecorderMsgProcResult::REC_MSG_PROC_IGNORE;
    }

    if (GST_MESSAGE_TYPE(&rawMsg) == GST_MESSAGE_ERROR) { // error msg is fatal, elements can not handle.
        return RecorderMsgProcResult::REC_MSG_PROC_IGNORE;
    }

    // if the message is extended format, translate it at here and return OK.

    RecorderMsgProcResult ret = DoProcessMessage(rawMsg, prettyMsg);
    prettyMsg.sourceId = GetSourceId();

    return ret;
}

bool RecorderElement::CheckAllParamsConfiged(const std::set<int32_t> &expectedParams) const
{
    std::set<int32_t> intersection;
    (void)std::set_intersection(expectedParams.begin(), expectedParams.end(),
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

bool RecorderElement::CheckAnyParamConfiged(const std::set<int32_t> &expectedParams) const
{
    std::set<int32_t> intersection;
    (void)std::set_intersection(expectedParams.begin(), expectedParams.end(),
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
