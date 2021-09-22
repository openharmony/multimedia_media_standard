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

#include "recorder_pipeline_builder.h"
#include "errors.h"
#include "media_log.h"
#include "recorder_private_param.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderPipelineBuilder"};
}

namespace OHOS {
namespace Media {
#define ADD_LINK_DESC(srcElem, dstElem, srcPad, sinkPad, isSrcPadStatic, isSinkPadStatic)                       \
    do {                                                                                                        \
        RecorderPipelineDesc::LinkDesc linkDesc = { dstElem, srcPad, sinkPad, isSrcPadStatic, isSinkPadStatic}; \
        pipelineDesc_->allLinkDescs[srcElem] = linkDesc;                                                        \
    } while (false)

RecorderPipelineBuilder::RecorderPipelineBuilder()
{
    MEDIA_LOGD("enter, ctor");
}

RecorderPipelineBuilder::~RecorderPipelineBuilder()
{
    MEDIA_LOGD("enter, dtor");
    Reset();
}

void RecorderPipelineBuilder::EnsureSourceOrder(bool isVideo)
{
    auto srcIter = std::next(pipelineDesc_->allElems.end(), -1);
    if (isVideo) {
        auto insertPos = std::next(pipelineDesc_->allElems.begin(), videoSrcCount);
        pipelineDesc_->allElems.insert(insertPos, *srcIter);
        videoSrcCount += 1;
    } else {
        auto insertPos = std::next(pipelineDesc_->allElems.begin(), videoSrcCount + otherSrcCount);
        pipelineDesc_->allElems.insert(insertPos, *srcIter);
        otherSrcCount += 1;
    }
    pipelineDesc_->allElems.erase(srcIter);
}

std::shared_ptr<RecorderElement> RecorderPipelineBuilder::CreateElement(
    const std::string &name,
    const RecorderSourceDesc &desc,
    bool isSource)
{
    if (pipelineDesc_ == nullptr) {
        pipelineDesc_ = std::make_shared<RecorderPipelineDesc>();
    }

    RecorderElement::CreateParam createParam = { desc, name };
    std::shared_ptr<RecorderElement> element = RecorderElementFactory::GetInstance().CreateElement(name, createParam);
    if (element == nullptr) {
        std::string sourceKind = desc.IsVideo() ? "video" : (desc.IsAudio() ? "audio" : "unknown");
        MEDIA_LOGE("Unable to create element for %{public}s source type: %{public}d, element name: %{public}s",
                   sourceKind.c_str(), desc.type_, name.c_str());
        return nullptr;
    }

    pipelineDesc_->allElems.push_back(element);
    if (isSource) {
        pipelineDesc_->srcElems.emplace(desc.handle_, element);
        EnsureSourceOrder(desc.IsVideo());
    }

    return element;
}

int32_t RecorderPipelineBuilder::CreateMuxSink()
{
    if (muxSink_ != nullptr) {
        return ERR_OK;
    }

    RecorderSourceDesc desc {}; // default initialization, meaninglessly
    muxSink_ = CreateElement("MuxSinkBin", desc, false);
    if (muxSink_ == nullptr) {
        MEDIA_LOGE("Unable to create element for MuxSinkBin !");
        return ERR_INVALID_OPERATION;
    }
    pipelineDesc_->muxerSinkBin = muxSink_;

    return ERR_OK;
}

int32_t RecorderPipelineBuilder::SetVideoSource(const RecorderSourceDesc &desc)
{
    int32_t ret = CreateMuxSink();
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    std::shared_ptr<RecorderElement> element;
    // currently only the ES Source is supported.
    if (desc.type_ == VideoSourceType::VIDEO_SOURCE_SURFACE_ES) {
        element = CreateElement("VideoSource", desc, true);
    } else {
        MEDIA_LOGE("Video source type %{public}d currently unsupported", desc.type_);
    }

    CHECK_AND_RETURN_RET(element != nullptr, ERR_INVALID_VALUE);

    // for the second video source, the sinkpad name should be video_aux_%u
    ADD_LINK_DESC(element, muxSink_, "src", "video", true, false);

    return ERR_OK;
}

int32_t RecorderPipelineBuilder::SetAudioSource(const RecorderSourceDesc &desc)
{
    int32_t ret = CreateMuxSink();
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    std::shared_ptr<RecorderElement> audioSrcElem;
    // currently only the mic is supported.
    if (desc.type_ == AudioSourceType::AUDIO_MIC) {
        audioSrcElem = CreateElement("AudioSource", desc, true);
    } else {
        MEDIA_LOGE("Audio source type %{public}d currently unsupported", desc.type_);
    }

    CHECK_AND_RETURN_RET(audioSrcElem != nullptr, ERR_INVALID_VALUE);

    std::shared_ptr<RecorderElement> audioConvert = CreateElement("AudioConverter", desc, false);
    CHECK_AND_RETURN_RET(audioConvert != nullptr, ERR_INVALID_VALUE);

    std::shared_ptr<RecorderElement> audioEncElem = CreateElement("AudioEncoder", desc, false);
    CHECK_AND_RETURN_RET(audioEncElem != nullptr, ERR_INVALID_VALUE);

    ADD_LINK_DESC(audioSrcElem, audioConvert, "src", "sink", true, true);
    ADD_LINK_DESC(audioConvert, audioEncElem, "src", "sink", true, true);
    ADD_LINK_DESC(audioEncElem, muxSink_, "src", "audio_%u", true, false);

    return ERR_OK;
}

int32_t RecorderPipelineBuilder::SetSource(const RecorderSourceDesc &desc)
{
    if (desc.IsVideo()) {
        return SetVideoSource(desc);
    } else if (desc.IsAudio()) {
        return SetAudioSource(desc);
    }

    // should never go to here.
    MEDIA_LOGE("Invalid source description !");
    return ERR_INVALID_VALUE;
}

int32_t RecorderPipelineBuilder::SetOutputFormat(OutputFormatType formatType)
{
    if (muxSink_ == nullptr) {
        MEDIA_LOGE("No source set, set the output format invalid !");
        return ERR_INVALID_OPERATION;
    }

    muxSink_->Configure(OutputFormat(formatType));

    outputFormatConfiged_ = true;
    return ERR_OK;
}

int32_t RecorderPipelineBuilder::Configure(int32_t sourceId, const RecorderParam &param)
{
    if (!outputFormatConfiged_) {
        MEDIA_LOGE("Output format not set, configure the pipeline is invalid !");
        return ERR_INVALID_OPERATION;
    }

    // distribute parameters to elements
    int ret = ERR_OK;
    for (auto &elem : pipelineDesc_->allElems) {
        if (elem->GetSourceId() == sourceId) {
            ret = elem->Configure(param);
            CHECK_AND_RETURN_RET(ret == ERR_OK, ret);
        }
    }

    return ERR_OK;
}

std::shared_ptr<RecorderPipeline> RecorderPipelineBuilder::Build()
{
    if (!outputFormatConfiged_) {
        MEDIA_LOGE("Output format not configured, build pipeline failed !");
        return nullptr;
    }

    /*
     * Execute a series of policies to filter pipeline graphs or check pipeline parameter configurations.
     *
     * | Remove stream policy | -> | Check parameter completeness policy | -> | Add elements policy |
     *
     * 1. Execute those policies firsly that maybe remove the whole audio stream or video stream
     * 2. Now, all streams are needed. Then, execute those policies that check whether all parameters are
     *    configured completely. If not completely, refuse to build pipeline.
     * 3. Execute those policies that maybe need to add some elements into pipeline.
     *
     * Specifically:
     * 1. Process the mismatch between capture rate and frame rate. If capture rate less than the frame rate, the
     *    all audio stream need be removed.
     * 2. Check whether the parameter fully configured. For example, the audio encoder required, but the audio encoder
     *    format is not configured or erroneously configured. The element itself can judge whether the required
     *    parameters are complete. If the parameter configured not completely, refuse to build pipeline.
     * 3. Process the audio caps mismatch between audio source and audio encoder. If caps mismatch is true, add the
     *    audio converter element into audio stream.
     */

    int ret;
    for (auto &elem : pipelineDesc_->allElems) {
        ret = elem->CheckConfigReady();  // Check whether the parameter fully configured
        CHECK_AND_RETURN_RET(ret == ERR_OK, nullptr);
    }

    ret = ExecuteLink();
    CHECK_AND_RETURN_RET(ret == ERR_OK, nullptr);

    return pipeline_;
}

int32_t RecorderPipelineBuilder::ExecuteLink()
{
    auto pipeline = std::make_shared<RecorderPipeline>(pipelineDesc_);
    int32_t ret = pipeline->Init();
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    auto linkHelper = std::make_unique<RecorderPipelineLinkHelper>(pipeline, pipelineDesc_);
    ret = linkHelper->ExecuteLink();
    CHECK_AND_RETURN_RET(ret == ERR_OK, ret);

    pipeline_ = pipeline;
    linkHelper_ = std::move(linkHelper);
    return ERR_OK;
}

void RecorderPipelineBuilder::Reset()
{
    linkHelper_ = nullptr;
    muxSink_ = nullptr;
    if (pipeline_ != nullptr) {
        pipeline_->Reset();
    }
    pipeline_ = nullptr;
    pipelineDesc_ = nullptr;

    outputFormatConfiged_ = false;
}
}
}
