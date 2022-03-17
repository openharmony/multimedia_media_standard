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
using AVMetaBufferBlockerWrapper = ThizWrapper<AVMetaBufferBlocker>;

GstPadProbeReturn AVMetaBufferBlocker::BlockCallback(GstPad *pad, GstPadProbeInfo *info, gpointer usrdata)
{
    if (pad == nullptr || info == nullptr || usrdata == nullptr) {
        return GST_PAD_PROBE_PASS;
    }

    auto thizStrong = AVMetaBufferBlockerWrapper::TakeStrongThiz(usrdata);
    if (thizStrong != nullptr) {
        return thizStrong->OnBlockCallback(*pad, *info);
    }
    return GST_PAD_PROBE_PASS;
}

void AVMetaBufferBlocker::PadAdded(GstElement *elem, GstPad *pad, gpointer userdata)
{
    if (elem == nullptr || pad == nullptr || userdata == nullptr) {
        return;
    }

    auto thizStrong = AVMetaBufferBlockerWrapper::TakeStrongThiz(userdata);
    if (thizStrong != nullptr) {
        return thizStrong->OnPadAdded(*elem, *pad);
    }
}

AVMetaBufferBlocker::AVMetaBufferBlocker(GstElement &elem, bool direction, BufferRecievedNotifier notifier)
    : elem_(elem), direction_(direction), notifier_(notifier)
{
    MEDIA_LOGD("ctor, elem: %{public}s, direction: %{public}d, 0x%{public}06" PRIXPTR,
        ELEM_NAME(&elem_), direction_, FAKE_POINTER(this));
}

AVMetaBufferBlocker::~AVMetaBufferBlocker()
{
    MEDIA_LOGD("dtor, 0x%{public}06" PRIXPTR, FAKE_POINTER(this));
}

void AVMetaBufferBlocker::Init()
{
    MEDIA_LOGD("elem: %{public}s", ELEM_NAME(&elem_));
    auto padList = direction_ ? elem_.srcpads : elem_.sinkpads;

    /**
     * Defaultly, the probe type is blocked. If the pads count is greater than 1,
     * we can only setup detecting probe, because the element maybe not start individual
     * thread for each pad, all pad maybe drived by just one thread. If one pad is
     * blocked, then all other pads can not detect any buffer.
     */
    GstPadProbeType type = GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM;
    if (g_list_length(padList) > 1) {
        MEDIA_LOGI("%{public}s's pads count is not 1, direction: %{public}d, only detect buffer, no blocked",
                   ELEM_NAME(&elem_), direction_);
        type = static_cast<GstPadProbeType>(type & ~GST_PAD_PROBE_TYPE_BLOCK);
        probeRet_ = GST_PAD_PROBE_OK;
    }

    // Add pad probe for all pads. If the pads count is greater than 1, we just detect buffer.
    for (GList *node = g_list_first(padList); node != nullptr; node = node->next) {
        if (node->data == nullptr) {
            continue;
        }
        GstPad *pad = reinterpret_cast<GstPad *>(node->data);

        // the subtitle stream must be ignored, currently we dont support it.
        std::string_view name = PAD_NAME(pad);
        if (name.find("subtitle") != std::string_view::npos) {
            continue;
        }

        AddPadProbe(*pad, type);
    }

    // listen to the "pad-added" signal to figure out whether the pads count is greater than 1.
    AVMetaBufferBlockerWrapper *wrapper = new (std::nothrow) AVMetaBufferBlockerWrapper(shared_from_this());
    CHECK_AND_RETURN_LOG(wrapper != nullptr, "can not create this wrapper");

    signalId_ = g_signal_connect_data(&elem_, "pad-added", GCallback(&AVMetaBufferBlocker::PadAdded), wrapper,
        (GClosureNotify)&AVMetaBufferBlockerWrapper::OnDestory, static_cast<GConnectFlags>(0));
    if (signalId_ == 0) {
        delete wrapper;
        MEDIA_LOGW("add signal failed for %{public}s", ELEM_NAME(&elem_));
    }
}

bool AVMetaBufferBlocker::CheckUpStreamBlocked(GstPad &pad)
{
    GstPad *upstreamPad = nullptr;

    if (GST_PAD_DIRECTION(&pad) == GST_PAD_SINK) {
        upstreamPad = gst_pad_get_peer(&pad);
    } else {
        GstElement *elem = gst_pad_get_parent_element(&pad);
        if (elem == nullptr) {
            if (GST_IS_PROXY_PAD(&pad)) { // find the proxy sinkpad for decodebin
                return false;
            }
            MEDIA_LOGE("unexpected, avoid lock, return true");
            return true;
        }

        /**
         * This is a multi-srcpads element, means that we meet the demuxer or multiqueue.
         * There is no need to figure out whether the demuxer or multiqueue's sinkpads are
         * blocked, because we guarante it will never happen.
         */
        if (g_list_length(elem->srcpads) > 1 || g_list_length(elem->sinkpads) == 0) {
            gst_object_unref(elem);
            return false;
        }

        GList *node = g_list_first(elem->sinkpads);
        if (node != nullptr && node->data != nullptr) {
            upstreamPad = GST_PAD_CAST(gst_object_ref(GST_PAD_CAST(node->data)));
        }

        gst_object_unref(elem);
    }

    if (upstreamPad == nullptr) {
        return false;
    }

    if (gst_pad_is_blocked(upstreamPad)) {
        gst_object_unref(upstreamPad);
        return true;
    }

    bool ret = CheckUpStreamBlocked(*upstreamPad);
    gst_object_unref(upstreamPad);

    return ret;
}

/**
 * Call it after IsRemoved, this function maybe return false if there
 * are no any probe setuped.
 */
bool AVMetaBufferBlocker::IsBufferDetected()
{
    std::unique_lock<std::mutex> lock(mutex_);

    for (auto &item : padInfos_) {
        if (item.hasBuffer) {
            continue;
        }

        MEDIA_LOGD("buffer not arrived for %{public}s's %{public}s",
                   ELEM_NAME(&elem_), PAD_NAME(item.pad));

        /**
         * if the upstream is blocked, we can not wait buffer arriving at this pad.
         * Thus, we just remove the probe at this pad and set the hasBuffer to true.
         * This operation maybe remove this block probe too early, but the upstream's
         * blocker will guarantee at least receiving one frame of buffer to finish the meta
         * extracting process.
         */
        if (item.probeId != 0 && CheckUpStreamBlocked(*item.pad)) {
            MEDIA_LOGD("%{public}s's %{public}s upstream is blocked, dont need this blocker",
                       ELEM_NAME(&elem_), PAD_NAME(item.pad));
            gst_pad_remove_probe(item.pad, item.probeId);
            item.probeId = 0;
            item.hasBuffer = true;
            continue;
        }

        // not detect buffer and the upstream of this pad is not blocked.
        return false;
    }

    return true;
}

bool AVMetaBufferBlocker::IsRemoved()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return isRemoved_;
}

void AVMetaBufferBlocker::Remove()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (isHidden_) {
        return;
    }

    for (auto &padInfo : padInfos_) {
        if (padInfo.probeId != 0) {
            MEDIA_LOGD("cancel block at %{public}s's %{public}s", ELEM_NAME(&elem_), PAD_NAME(padInfo.pad));
            gst_pad_remove_probe(padInfo.pad, padInfo.probeId);
            padInfo.probeId = 0;
        }
    }

    if (signalId_ != 0) {
        g_signal_handler_disconnect(&elem_, signalId_);
        signalId_ = 0;
    }

    isRemoved_ = true;
}

void AVMetaBufferBlocker::Hide()
{
    std::unique_lock<std::mutex> lock(mutex_);
    isHidden_ = true;
}

GstPadProbeReturn AVMetaBufferBlocker::OnBlockCallback(GstPad &pad, GstPadProbeInfo &info)
{
    auto type = static_cast<unsigned int>(info.type);
    if ((type & (GST_PAD_PROBE_TYPE_BUFFER | GST_PAD_PROBE_TYPE_BUFFER_LIST)) == 0) {
        return probeRet_;
    }

    std::unique_lock<std::mutex> lock(mutex_);
    for (auto &padInfo : padInfos_) {
        if (&pad != padInfo.pad) {
            continue;
        }

        if (padInfo.probeId == 0) {
            MEDIA_LOGI("block already be canceled, exit");
            return GST_PAD_PROBE_REMOVE;
        }

        padInfo.hasBuffer = true;
        MEDIA_LOGD("buffer arrived at %{public}s's pad %{public}s", PAD_PARENT_NAME(&pad), PAD_NAME(&pad));
        lock.unlock(); // ???

        if (notifier_ != nullptr) {
            notifier_();
        }
        return GST_PAD_PROBE_OK;
    }

    return probeRet_;
}

void AVMetaBufferBlocker::OnPadAdded(GstElement &elem, GstPad &pad)
{
    MEDIA_LOGD("demux %{public}s sinkpad %{public}s added", ELEM_NAME(&elem), PAD_NAME(&pad));

    std::unique_lock<std::mutex> lock(mutex_);

    /**
     * if it've already been required to remove all probes, we need to
     * reject to add probe to new pad.
     */
    if (isRemoved_) {
        MEDIA_LOGI("block already be removed, exit");
        return;
    }

    GstPadDirection currDirection = direction_ ? GST_PAD_SRC : GST_PAD_SINK;
    if (GST_PAD_DIRECTION(&pad) != currDirection) {
        return;
    }

    GstPadProbeType type = GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM;

    /**
     * If it has one pad before, we must change the blocked probe to
     * detecting probe, or no buffer can arrive at this new pad if the
     * previous pad has been blocked, because the element maybe not
     * start individual thread for each pad, all pad maybe drived by
     * just one thread.
     */
    if (padInfos_.size() != 0) {
        MEDIA_LOGI("%{public}s's pads count is not 1, direction: %{public}d, only detect buffer, no blocked",
                   ELEM_NAME(&elem_), direction_);
        type = static_cast<GstPadProbeType>(type & ~GST_PAD_PROBE_TYPE_BLOCK);
        probeRet_ = GST_PAD_PROBE_OK;

        std::vector<PadInfo> temp;
        temp.swap(padInfos_);

        for (auto &padInfo : temp) {
            /**
             * if the blocked probe setuped at this pad, we add detecting probe
             * at it firstly, then remove the blocing probe, for avoiding to miss
             * the first buffer passing through this pad.
             */
            if (padInfo.blocked && padInfo.probeId != 0) {
                AddPadProbe(*padInfo.pad, type);
                gst_pad_remove_probe(padInfo.pad, padInfo.probeId);
            } else {
                // not blocked probe, just move to new container.
                padInfos_.push_back(padInfo);
            }
        }
    }

    // the subtitle stream must be ignored, currently we dont support it.
    std::string_view name = PAD_NAME(&pad);
    if (name.find("subtitle") != std::string_view::npos) {
        return;
    }

    // now, we can add the probe for new pad.
    AddPadProbe(pad, type);
}

void AVMetaBufferBlocker::AddPadProbe(GstPad &pad, GstPadProbeType type)
{
    AVMetaBufferBlockerWrapper *wrapper = new(std::nothrow) AVMetaBufferBlockerWrapper(shared_from_this());
    CHECK_AND_RETURN_LOG(wrapper != nullptr, "can not create this wrapper");

    bool blocked = type & GST_PAD_PROBE_TYPE_BLOCK;
    gulong probeId = gst_pad_add_probe(&pad, type, BlockCallback,
        wrapper, &AVMetaBufferBlockerWrapper::OnDestory);
    if (probeId == 0) {
        MEDIA_LOGW("add probe for %{public}s's pad %{public}s failed",
                    PAD_PARENT_NAME(&pad), PAD_NAME(&pad));
        delete wrapper;
    } else {
        (void)padInfos_.emplace_back(PadInfo { &pad, probeId, false, blocked });
    }
}
} // namespace Media
} // namespace OHOS
