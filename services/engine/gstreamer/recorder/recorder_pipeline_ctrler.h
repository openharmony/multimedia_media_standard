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

#ifndef RECORDER_PIPELINE_CTRLER
#define RECORDER_PIPELINE_CTRLER

#include <memory>
#include "nocopyable.h"
#include "i_recorder_engine.h"
#include "recorder_pipeline.h"
#include "task_queue.h"

namespace OHOS {
namespace Media {
class RecorderPipelineCtrler {
public:
    RecorderPipelineCtrler();
    ~RecorderPipelineCtrler();

    void SetObs(const std::weak_ptr<IRecorderEngineObs> &obs);
    void SetPipeline(std::shared_ptr<RecorderPipeline> pipeline);

    int32_t Init();
    int32_t Prepare();
    int32_t Start();
    int32_t Pause();
    int32_t Resume();
    int32_t Stop(bool isDrainAll);
    int32_t Reset();

    DISALLOW_COPY_AND_MOVE(RecorderPipelineCtrler);

private:
    void Notify(const RecorderMessage &msg);

    std::weak_ptr<IRecorderEngineObs> obs_;
    std::shared_ptr<RecorderPipeline> pipeline_;

    std::unique_ptr<TaskQueue> cmdQ_;
    std::unique_ptr<TaskQueue> msgQ_;
};
}
}
#endif
