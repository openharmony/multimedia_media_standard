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

#ifndef RECORDER_PIPELINE_LINK_HELPER
#define RECORDER_PIPELINE_LINK_HELPER

#include <gst/gstelement.h>
#include <gst/gstpad.h>
#include "recorder_element.h"
#include "recorder_pipeline.h"

namespace OHOS {
namespace Media {
class RecorderPipelineLinkHelper : public NoCopyable {
public:
    RecorderPipelineLinkHelper(const std::shared_ptr<RecorderPipeline> &pipeline,
                               const std::shared_ptr<RecorderPipelineDesc> &desc);
    ~RecorderPipelineLinkHelper();

    int32_t ExecuteLink();

private:
    GstPad *GetGstPad(const std::shared_ptr<RecorderElement>& elem, bool isStaticPad, const std::string padName);
    int32_t ExecuteOneLink(const std::shared_ptr<RecorderElement>& srcElem,
                           const RecorderPipelineDesc::LinkDesc &linkDesc);
    void Clear();

    std::shared_ptr<RecorderPipeline> pipeline_;
    std::shared_ptr<RecorderPipelineDesc> desc_;
    std::map<GstElement *, std::vector<GstPad *>> requestedPads_;
};
} // namespace Media
} // namespace OHOS
#endif
