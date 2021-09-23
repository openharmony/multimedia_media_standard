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

#include "avmeta_sinkprovider.h"
#include "avmetadatahelper.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMetaSinkProvider"};
}

namespace OHOS {
namespace Media {
AVMetaSinkProvider::AVMetaSinkProvider(int32_t usage) : usage_(usage)
{
    MEDIA_LOGD("enter ctor, instance: 0x%{public}06" PRIXPTR ", usage: %{public}d", FAKE_POINTER(this), usage_);
}

AVMetaSinkProvider::~AVMetaSinkProvider()
{
    MEDIA_LOGD("enter dtor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
    if (audSink_ != nullptr) {
        gst_object_unref(audSink_);
        audSink_ = nullptr;
    }
    if (vidSink_ != nullptr) {
        gst_object_unref(vidSink_);
        vidSink_ = nullptr;
    }
}

PlayBinSinkProvider::SinkPtr AVMetaSinkProvider::CreateAudioSink()
{
    if (audSink_ == nullptr) {
        audSink_ = gst_element_factory_make("fakesink", "avmeta_aud_sink");
    }

    if (audSink_ == nullptr) {
        MEDIA_LOGE("audsink is nullptr");
    }

    return GST_ELEMENT_CAST(gst_object_ref(audSink_));
}

PlayBinSinkProvider::SinkPtr AVMetaSinkProvider::CreateVideoSink()
{
    if (vidSink_ == nullptr) {
        if (usage_ == AVMetadataUsage::AV_META_USAGE_META_ONLY) {
            vidSink_ = gst_element_factory_make("fakesink", "avmeta_vid_sink");
        } else {
            vidSink_ = gst_element_factory_make("appsink", "avmeta_vid_sink");
            // setup callbacks for appsink
        }
    }

    if (vidSink_ == nullptr) {
        MEDIA_LOGE("vidsink is nullptr");
    }

    return GST_ELEMENT_CAST(gst_object_ref(vidSink_));
}

void AVMetaSinkProvider::SetFrameCallback(const std::shared_ptr<FrameCallback> &callback)
{
    callback_ = callback;
}
}
}
