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

#include "recorder_pipeline_link_helper.h"
#include <gst/gst.h>
#include "errors.h"
#include "media_log.h"
#include "scope_guard.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderPipelineLinkHelper"};
}

namespace OHOS {
namespace Media {
#define UNREF_STATIC_PAD(pad, isStaticPad) \
    do { \
        if ((isStaticPad) && ((pad) != nullptr)) { \
            gst_object_unref(pad); \
        } \
    } while (false)

#define ADD_ELEM_TO_PIPELINE(elem, elemSet, pipeline)                      \
    do {                                                                   \
        if (elemSet.find(elem) == elemSet.end())  {                        \
            elemSet.emplace(elem);                                         \
            gst_bin_add(GST_BIN(pipeline_->gstPipeline_), elem->gstElem_); \
        }                                                                  \
    } while (false)

GstPad *RecorderPipelineLinkHelper::GetGstPad(
    const std::shared_ptr<RecorderElement>& elem, bool isStaticPad, const std::string padName)
{
    GstPad *pad = nullptr;
    if (isStaticPad) {
        pad = gst_element_get_static_pad(elem->gstElem_, padName.c_str());
    } else {
        MEDIA_LOGI("request pad %{public}s from element  %{public}s", padName.c_str(), elem->GetName().c_str());
        pad = gst_element_get_request_pad(elem->gstElem_, padName.c_str());
    }

    if (pad == nullptr)  {
        MEDIA_LOGE("Get pad %{public}s from element %{public}s failed !", padName.c_str(), elem->GetName().c_str());
        return nullptr;
    }

    if (!isStaticPad) {
        if (requestedPads_.count(elem->gstElem_) == 0) {
            requestedPads_.emplace(elem->gstElem_, std::vector<GstPad *>());
        }
        requestedPads_[elem->gstElem_].push_back(pad);
    }

    return pad;
}

RecorderPipelineLinkHelper::RecorderPipelineLinkHelper(
    const std::shared_ptr<RecorderPipeline>& pipeline, const std::shared_ptr<RecorderPipelineDesc>& desc)
    : pipeline_(pipeline), desc_(desc)
{
}

RecorderPipelineLinkHelper::~RecorderPipelineLinkHelper()
{
    Clear();
}

int32_t RecorderPipelineLinkHelper::ExecuteOneLink(
    const std::shared_ptr<RecorderElement>& srcElem, const RecorderPipelineDesc::LinkDesc &linkDesc)
{
    GstPad *srcPad;
    GstPad *sinkPad;

    ON_SCOPE_EXIT(0) {
        UNREF_STATIC_PAD(srcPad, linkDesc.isSrcPadStatic);
        UNREF_STATIC_PAD(sinkPad, linkDesc.isSinkPadStatic);
    };

    srcPad = GetGstPad(srcElem, linkDesc.isSrcPadStatic, linkDesc.srcPad.c_str());
    CHECK_AND_RETURN_RET(srcPad != nullptr, ERR_INVALID_OPERATION);

    sinkPad = GetGstPad(linkDesc.dstElem, linkDesc.isSinkPadStatic, linkDesc.sinkPad.c_str());
    CHECK_AND_RETURN_RET(sinkPad != nullptr, ERR_INVALID_OPERATION);

    GstPadLinkReturn ret = gst_pad_link(srcPad, sinkPad);
    if (ret != GST_PAD_LINK_OK) {
        MEDIA_LOGE("link elem(%{public}s)'s pad %{public}s to elem(%{public}s)'s pad %{public}s failed !",
                   srcElem->GetName().c_str(), linkDesc.srcPad.c_str(),
                   linkDesc.dstElem->GetName().c_str(), linkDesc.sinkPad.c_str());
        return ERR_INVALID_OPERATION;
    }

    return ERR_OK;
}

int32_t RecorderPipelineLinkHelper::ExecuteLink()
{
    if (desc_ == nullptr) {
        MEDIA_LOGE("pipeline desc is nullptr");
        return ERR_INVALID_OPERATION;
    }

    std::set<std::shared_ptr<RecorderElement>> uniqueElems;
    for (auto &srcElemLinks : desc_->allLinkDescs) {
        ADD_ELEM_TO_PIPELINE(srcElemLinks.first, uniqueElems, pipeline_);
        for (auto &linkDesc : srcElemLinks.second) {
            ADD_ELEM_TO_PIPELINE(linkDesc.dstElem, uniqueElems, pipeline_);
            int32_t ret = ExecuteOneLink(srcElemLinks.first, linkDesc);
            if (ret != ERR_OK) {
                Clear();
                MEDIA_LOGE("Pipeline link failed !");
                return ret;
            }
        }
    }

    return ERR_OK;
}

void RecorderPipelineLinkHelper::Clear()
{
    MEDIA_LOGD("enter");

    for (auto &items : requestedPads_) {
        for (auto &pad : items.second) {
            gst_element_release_request_pad(items.first, pad);
            gst_object_unref(pad);
        }
    }

    requestedPads_.clear();
    pipeline_ = nullptr;
    desc_ = nullptr;
}
}
}