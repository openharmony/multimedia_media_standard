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

#include "avcodec_engine_factory.h"
#include <cstdlib>
#include "format_processor/processor_adec_impl.h"
#include "format_processor/processor_aenc_impl.h"
#include "format_processor/processor_vdec_impl.h"
#include "format_processor/processor_venc_impl.h"
#include "sink_wrapper/sink_bytebuffer_impl.h"
#include "sink_wrapper/sink_surface_impl.h"
#include "src_wrapper/src_bytebuffer_impl.h"
#include "src_wrapper/src_surface_impl.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecEngineFactory"};
}

namespace OHOS {
namespace Media {
std::unique_ptr<ProcessorBase> AVCodecEngineFactory::CreateProcessor(AVCodecType type)
{
    std::unique_ptr<ProcessorBase> processor;
    switch (type) {
        case AVCODEC_TYPE_VIDEO_ENCODER:
            processor = std::make_unique<ProcessorVencImpl>();
            break;
        case AVCODEC_TYPE_VIDEO_DECODER:
            processor = std::make_unique<ProcessorVdecImpl>();
            break;
        case AVCODEC_TYPE_AUDIO_ENCODER:
            processor = std::make_unique<ProcessorAencImpl>();
            break;
        case AVCODEC_TYPE_AUDIO_DECODER:
            processor = std::make_unique<ProcessorAdecImpl>();
            break;
        default:
            MEDIA_LOGE("Unknown type");
            break;
    }
    return processor;
}

std::unique_ptr<SinkBase> AVCodecEngineFactory::CreateSink(SinkType type)
{
    std::unique_ptr<SinkBase> sink;
    switch (type) {
        case SINK_TYPE_BYTEBUFFER:
            sink = std::make_unique<SinkBytebufferImpl>();
            break;
        case SINK_TYPE_SURFACE:
            sink = std::make_unique<SinkSurfaceImpl>();
            break;
        default:
            MEDIA_LOGE("Unknown type");
            break;
    }
    return sink;
}

std::unique_ptr<SrcBase> AVCodecEngineFactory::CreateSrc(SrcType type)
{
    std::unique_ptr<SrcBase> src;
    switch (type) {
        case SRC_TYPE_BYTEBUFFER:
            src = std::make_unique<SrcBytebufferImpl>();
            break;
        case SRC_TYPE_SURFACE:
            src = std::make_unique<SrcSurfaceImpl>();
            break;
        default:
            MEDIA_LOGE("Unknown type");
            break;
    }
    return src;
}
} // namespace Media
} // namespace OHOS
