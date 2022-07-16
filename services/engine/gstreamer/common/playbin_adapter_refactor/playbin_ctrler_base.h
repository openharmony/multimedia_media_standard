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

#ifndef PLAYBIN_CTRLER_BASE_H
#define PLAYBIN_CTRLER_BASE_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <gst/gst.h>
#include "nocopyable.h"
#include "i_playbin_ctrler.h"
#include "state_machine.h"
#include "gst_msg_processor.h"
#include "task_queue.h"

namespace OHOS {
namespace Media {
class PlayBinCtrlerBase
    : public IPlayBinCtrler,
      public StateMachine,
      public std::enable_shared_from_this<PlayBinCtrlerBase> {
public:
    explicit PlayBinCtrlerBase(const PlayBinCreateParam &createParam);
    virtual ~PlayBinCtrlerBase();

    int32_t Init();
    int32_t SetSource(const std::string &url)  override;
    int32_t SetSource(const std::shared_ptr<GstAppsrcWrap> &appsrcWrap) override;
    int32_t Prepare() override;
    int32_t Play() override;
    int32_t Pause() override;
    int32_t Seek(int64_t timeUs, int32_t seekOption) override;
    int32_t Stop() override;
    int64_t GetDuration() override;
    int64_t GetPosition() override;
    int32_t SetRate(double rate) override;
    double GetRate() override;

protected:
    virtual int32_t OnInit() = 0;

    GstPipeline *playbin_ = nullptr;
};
} // namespace Media
} // namespace OHOS
#endif // PLAYBIN_CTRLER_BASE_H