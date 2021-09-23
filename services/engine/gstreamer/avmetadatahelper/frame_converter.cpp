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

#include "frame_converter.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "FrameConvert"};
}

namespace OHOS {
namespace Media {
FrameConverter::FrameConverter()
{
    MEDIA_LOGD("enter ctor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
}

FrameConverter::~FrameConverter()
{
    MEDIA_LOGD("enter dtor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
    (void)Reset();
}

int32_t FrameConverter::Init(const OutputConfiguration &config)
{
    (void)config;
    return MSERR_OK;
}

void FrameConverter::OnFrameAvaiable(GstBuffer &frame)
{
    (void)frame;
}

int32_t FrameConverter::StartConvert()
{
    return MSERR_OK;
}

std::shared_ptr<AVSharedMemory> FrameConverter::GetOneFrame()
{
    return nullptr;
}

int32_t FrameConverter::StopConvert()
{
    return MSERR_OK;
}

int32_t FrameConverter::Reset()
{
    if (pipeline_ != nullptr) {
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
    }

    if (appSink_ != nullptr) {
        gst_object_unref(appSink_);
        appSink_ = nullptr;
    }

    if (appSrc_ != nullptr) {
        gst_object_unref(appSrc_);
        appSrc_ = nullptr;
    }
    return MSERR_OK;
}
}
}
