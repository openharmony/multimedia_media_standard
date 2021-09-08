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

#include "avmeta_buffer_blocker.h"
#include "media_errors.h"
#include "media_log.h"
#include "gst_utils.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "BufferBlocker"};
}

namespace OHOS {
namespace Media {
AVMetaBufferBlocker::AVMetaBufferBlocker(GstElement &elem, bool direction, BufferRecievedNotifier notifier)
    : elem_(elem), direction_(direction), notifier_(notifier)
{
    MEDIA_LOGD("ctor, elem: %{public}s, direction: %{public}d", ELEM_NAME(&elem_), direction_);
}

AVMetaBufferBlocker::~AVMetaBufferBlocker()
{
    MEDIA_LOGD("dtor, elem: %{public}s", ELEM_NAME(&elem_));
    std::unique_lock<std::mutex> lock(mutex_);
    for (auto &padInfo : padInfos_) {
        gst_pad_remove_probe(padInfo.pad, padInfo.probeId);
    }
    if (signalId_ != 0) {
        g_signal_handler_disconnect(&elem_, signalId_);
    }
}

void AVMetaBufferBlocker::Init()
{
    MEDIA_LOGD("Buffer Blocker for elem: %{public}s", ELEM_NAME(&elem_));

    auto padList = direction_ ? elem_.srcpads : elem_.sinkpads;
    for (GList *padNode = g_list_first(padList); padNode != nullptr; padNode = padNode->next) {
        if (padNode->data == nullptr) {
            continue;
        }
        GstPad *pad = reinterpret_cast<GstPad *>(padNode->data);
        std::string_view name = PAD_NAME(pad);
        if (name.find("subtitle") != std::string_view::npos) { // the subtitle stream is ignore
            return;
        }
        gulong probeId = gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, BlockCallback, this, nullptr);
        if (probeId == 0) {
            MEDIA_LOGW("add probe for %{public}s's pad %{public}s failed",
                       PAD_PARENT_NAME(&pad), PAD_NAME(&pad));
            continue;
        }
        (void)padInfos_.emplace_back(PadInfo {pad, probeId, false});
    }

    signalId_ = g_signal_connect(&elem_, "pad-added", GCallback(&AVMetaBufferBlocker::PadAdded), this);
    if (signalId_ == 0) {
        MEDIA_LOGW("add signal failed for %{public}s", ELEM_NAME(&elem_));
    }
}

bool AVMetaBufferBlocker::CheckBufferRecieved()
{
    std::unique_lock<std::mutex> lock(mutex_);
    for (const auto &item : padInfos_) {
        if ((item.probeId != 0) && !item.hasBuffer) {
            MEDIA_LOGD("buffer not arrived for %{public}s's %{public}s",
                       ELEM_NAME(&elem_), PAD_NAME(item.pad));
            return false;
        }
    }

    return true;
}

uint32_t AVMetaBufferBlocker::GetStreamCount()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return static_cast<uint32_t>(padInfos_.size());
}

void AVMetaBufferBlocker::CancelBlock(uint32_t index)
{
    MEDIA_LOGD("cancel block at %{public}s's block id : %{public}u", ELEM_NAME(&elem_), index);

    std::unique_lock<std::mutex> lock(mutex_);
    if (index >= padInfos_.size()) {
        return;
    }
    if (padInfos_[index].probeId != 0) {
        gst_pad_remove_probe(padInfos_[index].pad, padInfos_[index].probeId);
        padInfos_[index].probeId = 0;
    }
}

void AVMetaBufferBlocker::Clear()
{
    std::unique_lock<std::mutex> lock(mutex_);
    {
        decltype(padInfos_) temp;
        temp.swap(padInfos_);
    }

    if (signalId_ != 0) {
        g_signal_handler_disconnect(&elem_, signalId_);
        signalId_ = 0;
    }
}

GstPadProbeReturn AVMetaBufferBlocker::BlockCallback(GstPad *pad, GstPadProbeInfo *info, gpointer usrdata)
{
    if (pad == nullptr || info ==  nullptr || usrdata == nullptr) {
        MEDIA_LOGE("param is invalid");
        return GST_PAD_PROBE_PASS;
    }

    auto type = static_cast<unsigned int>(info->type);
    if ((type & (GST_PAD_PROBE_TYPE_BUFFER | GST_PAD_PROBE_TYPE_BUFFER_LIST)) == 0) {
        return GST_PAD_PROBE_PASS;
    }

    auto blocker = reinterpret_cast<AVMetaBufferBlocker *>(usrdata);
    std::unique_lock<std::mutex> lock(blocker->mutex_);
    for (auto &padInfo : blocker->padInfos_) {
        if (padInfo.pad != pad) {
            continue;
        }
        padInfo.hasBuffer = true;
        MEDIA_LOGD("buffer arrived at %{public}s's pad %{public}s", PAD_PARENT_NAME(pad), PAD_NAME(pad));
        if (blocker->notifier_ != nullptr) {
            blocker->notifier_();
        }
        return GST_PAD_PROBE_OK;
    }

    return GST_PAD_PROBE_PASS;
}

void AVMetaBufferBlocker::PadAdded(GstElement *elem, GstPad *pad, gpointer userdata)
{
    if (elem == nullptr || pad == nullptr || userdata == nullptr) {
        MEDIA_LOGE("invalid param");
        return;
    }
    MEDIA_LOGD("demuxer %{public}s sinkpad %{public}s added", ELEM_NAME(elem), PAD_NAME(pad));

    auto blocker = reinterpret_cast<AVMetaBufferBlocker *>(userdata);
    std::unique_lock<std::mutex> lock(blocker->mutex_);

    GstPadDirection gstDirection = (blocker->direction_) ? GST_PAD_SRC : GST_PAD_SINK;
    if (GST_PAD_DIRECTION(pad) != gstDirection) {
        return;
    }

    std::string_view name = PAD_NAME(pad);
    if (name.find("subtitle") != std::string_view::npos) { // the subtitle stream is ignore
        return;
    }

    gulong probeId = gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, BlockCallback, userdata, nullptr);
    if (probeId == 0) {
        MEDIA_LOGW("add probe for %{public}s's pad %{public}s failed",
                   PAD_PARENT_NAME(pad), PAD_NAME(pad));
        return;
    }

    (void)blocker->padInfos_.emplace_back(PadInfo {pad, probeId, false});
}
}
}
