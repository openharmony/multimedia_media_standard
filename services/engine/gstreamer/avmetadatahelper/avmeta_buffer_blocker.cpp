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

        AVMetaBufferBlockerWrapper *wrapper = new(std::nothrow) AVMetaBufferBlockerWrapper(shared_from_this());
        CHECK_AND_RETURN_LOG(wrapper != nullptr, "can not create this wrapper");

        gulong probeId = gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, BlockCallback, wrapper,
            &AVMetaBufferBlockerWrapper::OnDestory);
        if (probeId == 0) {
            MEDIA_LOGW("add probe for %{public}s's pad %{public}s failed",
                       PAD_PARENT_NAME(&pad), PAD_NAME(&pad));
            delete wrapper;
            continue;
        }
        (void)padInfos_.emplace_back(PadInfo {pad, probeId, false});
    }

    AVMetaBufferBlockerWrapper *wrapper = new(std::nothrow) AVMetaBufferBlockerWrapper(shared_from_this());
    CHECK_AND_RETURN_LOG(wrapper != nullptr, "can not create this wrapper");

    signalId_ = g_signal_connect_data(&elem_, "pad-added", GCallback(&AVMetaBufferBlocker::PadAdded), wrapper,
        (GClosureNotify)&AVMetaBufferBlockerWrapper::OnDestory, static_cast<GConnectFlags>(0));
    if (signalId_ == 0) {
        delete wrapper;
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

void AVMetaBufferBlocker::CancelBlock(int32_t index)
{
    MEDIA_LOGD("cancel block at %{public}s's block id : %{public}d", ELEM_NAME(&elem_), index);

    std::unique_lock<std::mutex> lock(mutex_);
    if (index == -1) {
        for (auto &padInfo : padInfos_) {
            if (padInfo.probeId != 0) {
                gst_pad_remove_probe(padInfo.pad, padInfo.probeId);
                padInfo.probeId = 0;
            }
        }

        if (signalId_ != 0) {
            g_signal_handler_disconnect(&elem_, signalId_);
            signalId_ = 0;
        }
        return;
    }

    uint32_t uIndex = static_cast<size_t>(index);
    if (uIndex >= padInfos_.size()) {
        return;
    }
    if (padInfos_[uIndex].probeId != 0) {
        gst_pad_remove_probe(padInfos_[uIndex].pad, padInfos_[uIndex].probeId);
        padInfos_[uIndex].probeId = 0;
    }

    lock.unlock();
    lock.lock();
}

void AVMetaBufferBlocker::Clear()
{
    std::unique_lock<std::mutex> lock(mutex_);
    for (auto &padInfo : padInfos_) {
        if (padInfo.probeId != 0) {
            padInfo.probeId = 0;
        }
    }

    if (signalId_ != 0) {
        g_signal_handler_disconnect(&elem_, signalId_);
        signalId_ = 0;
    }
}

GstPadProbeReturn AVMetaBufferBlocker::OnBlockCallback(GstPad &pad, GstPadProbeInfo &info)
{
    auto type = static_cast<unsigned int>(info.type);
    if ((type & (GST_PAD_PROBE_TYPE_BUFFER | GST_PAD_PROBE_TYPE_BUFFER_LIST)) == 0) {
        return GST_PAD_PROBE_PASS;
    }

    std::unique_lock<std::mutex> lock(mutex_);
    for (auto &padInfo : padInfos_) {
        if (padInfo.pad != &pad) {
            continue;
        }

        if (padInfo.probeId == 0) {
            MEDIA_LOGI("block already be canceled, exit");
            return GST_PAD_PROBE_REMOVE;
        }

        padInfo.hasBuffer = true;
        MEDIA_LOGD("buffer arrived at %{public}s's pad %{public}s", PAD_PARENT_NAME(&pad), PAD_NAME(&pad));
        lock.unlock();
        if (notifier_ != nullptr) {
            notifier_();
        }
        MEDIA_LOGD("buffer arrived at %{public}s's pad %{public}s exit", PAD_PARENT_NAME(&pad), PAD_NAME(&pad));
        return GST_PAD_PROBE_OK;
    }

    return GST_PAD_PROBE_PASS;
}

void AVMetaBufferBlocker::OnPadAdded(GstElement &elem, GstPad &pad)
{
    MEDIA_LOGD("demuxer %{public}s sinkpad %{public}s added", ELEM_NAME(&elem), PAD_NAME(&pad));

    std::unique_lock<std::mutex> lock(mutex_);

    if (signalId_ == 0) {
        MEDIA_LOGI("block already be canceled, exit");
        return;
    }

    GstPadDirection gstDirection = (direction_) ? GST_PAD_SRC : GST_PAD_SINK;
    if (GST_PAD_DIRECTION(&pad) != gstDirection) {
        return;
    }

    std::string_view name = PAD_NAME(&pad);
    if (name.find("subtitle") != std::string_view::npos) { // the subtitle stream is ignore
        return;
    }

    AVMetaBufferBlockerWrapper *wrapper = new(std::nothrow) AVMetaBufferBlockerWrapper(shared_from_this());
    CHECK_AND_RETURN_LOG(wrapper != nullptr, "can not create this wrapper");

    gulong probeId = gst_pad_add_probe(&pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, BlockCallback, wrapper,
        &AVMetaBufferBlockerWrapper::OnDestory);
    if (probeId == 0) {
        MEDIA_LOGW("add probe for %{public}s's pad %{public}s failed", PAD_PARENT_NAME(&pad), PAD_NAME(&pad));
        delete wrapper;
        return;
    }

    (void)padInfos_.emplace_back(PadInfo {&pad, probeId, false});
}
}
}
