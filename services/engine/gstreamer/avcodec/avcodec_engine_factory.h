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

#ifndef AVCODEC_ENGINE_FACTORY_H
#define AVCODEC_ENGINE_FACTORY_H

#include <memory>
#include "format_processor/processor_base.h"
#include "sink_wrapper/sink_base.h"
#include "src_wrapper/src_base.h"

namespace OHOS {
namespace Media {
class AVCodecEngineFactory {
public:
    AVCodecEngineFactory() = delete;
    ~AVCodecEngineFactory() = delete;

    static std::unique_ptr<ProcessorBase> CreateProcessor(AVCodecType type);
    static std::unique_ptr<SinkBase> CreateSink(SinkType type);
    static std::unique_ptr<SrcBase> CreateSrc(SrcType type);
};
} // namespace Media
} // namespace OHOS
#endif // AVCODEC_ENGINE_FACTORY_H
