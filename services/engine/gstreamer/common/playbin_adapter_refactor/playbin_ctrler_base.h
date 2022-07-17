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
#include <mutex>
#include <unordered_map>
#include <gst/gst.h>
#include "nocopyable.h"
#include "i_playbin_ctrler.h"
#include "gst_msg_dispatcher.h"

namespace OHOS {
namespace Media {
namespace PlayBin {
class PlayBinCtrlerBase : public IPlayBinCtrler {
public:
    explicit PlayBinCtrlerBase(const CreateParam &createParam);
    virtual ~PlayBinCtrlerBase();

    int32_t Init();
    int32_t SetSource(const std::shared_ptr<SourceBase> &source) override;
    int32_t Prepare() override;
    int32_t Play() override;
    int32_t Pause() override;
    int32_t Seek(int64_t timeUs, SeekMode seekOption) override;
    int32_t SetSpeed(double speed) override;
    int32_t Stop() override;
    int32_t Reset() override;
    int64_t GetDuration() override;
    int64_t GetPosition() override;
    double GetSpeed() override;

private:
    int32_t DoSetup();
    int32_t DoSetupSource();
    int32_t DoSetupSink();
    int32_t DoSetupRenderPipeline();
    int32_t DoSetupSignal();
    int32_t DoSetupMsgDispatcher();
    void DoUnSetup();
    void UpdateStateAndReport(PlayBinState newState);
    void OnMessageReceived(const InnerMessage &msg);

    RenderMode renderMode_;
    InnerMsgNotifier notifier_;
    std::shared_ptr<SinkProvider> sinkProvider_;
    std::shared_ptr<SourceBase> source_;
    std::shared_ptr<IGstMsgConverter> msgConverter_;
    std::unique_ptr<GstMsgDispatcher> msgDispatcher_;
    std::shared_ptr<TaskQueue> workLooper_;

    std::mutex mutex_;
    PlayBinState selfState_ = PLAYBIN_STATE_IDLE;
    GstPipeline *playbin_ = nullptr;

    friend class PlayBinStateOperator;
    friend class PlayBinEventMsgSender;
    std::unordered_map<GstElement *, gulong> signalIds_;
};
}
} // namespace Media
} // namespace OHOS
#endif // PLAYBIN_CTRLER_BASE_H