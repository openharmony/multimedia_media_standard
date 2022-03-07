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

#ifndef AVMETA_BUFFER_BLOCKER_H
#define AVMETA_BUFFER_BLOCKER_H

#include <vector>
#include <mutex>
#include <functional>
#include <memory>
#include <gst/gst.h>
#include "nocopyable.h"

namespace OHOS {
namespace Media {
/**
 * Utility for easy to process the work that block buffer on one element's pads.
 * Only available for avmeta_meta_collector.
 */
class AVMetaBufferBlocker : public std::enable_shared_from_this<AVMetaBufferBlocker>, public NoCopyable {
public:
    using BufferRecievedNotifier = std::function<void(void)>;
    // direction == true means block srcpads's buffer
    AVMetaBufferBlocker(GstElement &elem, bool direction, BufferRecievedNotifier notifier);
    ~AVMetaBufferBlocker();

    void Init();
    bool IsRemoved();
    bool IsBufferDetected();
    void Remove();
    void Hide();

private:
    static GstPadProbeReturn BlockCallback(GstPad *pad, GstPadProbeInfo *info, gpointer usrdata);
    static void PadAdded(GstElement *elem, GstPad *pad, gpointer userdata);
    GstPadProbeReturn OnBlockCallback(GstPad &pad, GstPadProbeInfo &info);
    void OnPadAdded(GstElement &elem, GstPad &pad);
    bool CheckUpStreamBlocked(GstPad &pad);
    void AddPadProbe(GstPad &pad, GstPadProbeType type);

    struct PadInfo {
        GstPad *pad;
        gulong probeId;
        bool hasBuffer;
        bool blocked;
    };

    std::mutex mutex_;
    bool isHidden_ = false;
    bool isRemoved_ = false;
    GstPadProbeReturn probeRet_ = GST_PAD_PROBE_PASS;
    std::vector<PadInfo> padInfos_;
    GstElement &elem_;
    gulong signalId_ = 0;
    bool direction_;
    BufferRecievedNotifier notifier_;
};
} // namespace Media
} // namespace OHOS
#endif // AVMETA_BUFFER_BLOCKER_H
