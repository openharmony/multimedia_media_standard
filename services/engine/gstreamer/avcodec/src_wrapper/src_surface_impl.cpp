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

#include "src_surface_impl.h"
#include "common_utils.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SrcSurfaceImpl"};
}

namespace OHOS {
namespace Media {
SrcSurfaceImpl::SrcSurfaceImpl()
{
}

SrcSurfaceImpl::~SrcSurfaceImpl()
{
    if (src_ != nullptr) {
        gst_object_unref(src_);
        src_ = nullptr;
    }
}

int32_t SrcSurfaceImpl::Init()
{
    src_ = GST_ELEMENT_CAST(gst_object_ref(gst_element_factory_make("surfacesrc", "src")));
    CHECK_AND_RETURN_RET_LOG(src_ != nullptr, MSERR_UNKNOWN, "Failed to make gstsurfacevideosrc");
    return MSERR_OK;
}

int32_t SrcSurfaceImpl::Configure(const std::shared_ptr<ProcessorConfig> &config)
{
    CHECK_AND_RETURN_RET(config != nullptr && config->caps_ != nullptr, MSERR_UNKNOWN);
    g_object_set(G_OBJECT(src_), "caps", config->caps_, nullptr);
    return MSERR_OK;
}

sptr<Surface> SrcSurfaceImpl::CreateInputSurface(const std::shared_ptr<ProcessorConfig> &inputConfig)
{
    CHECK_AND_RETURN_RET(Configure(inputConfig) == MSERR_OK, nullptr);

    GstStateChangeReturn ret = gst_element_set_state(src_, GST_STATE_READY);
    CHECK_AND_RETURN_RET(ret != GST_STATE_CHANGE_FAILURE, nullptr);

    GValue val = G_VALUE_INIT;
    g_object_get_property(G_OBJECT(src_), "surface", &val);
    gpointer surfaceObj = g_value_get_pointer(&val);
    CHECK_AND_RETURN_RET_LOG(surfaceObj != nullptr, nullptr, "Failed to get surface");
    sptr<Surface> surface = reinterpret_cast<Surface *>(surfaceObj);
    return surface;
}

int32_t SrcSurfaceImpl::SetParameter(const Format &format)
{
    int32_t value = 0;
    if (format.GetValueType(std::string_view("suspend_input_surface")) == FORMAT_TYPE_INT32) {
        if (format.GetIntValue("suspend_input_surface", value) && (value == 0 || value == 1)) {
            g_object_set(src_, "suspend", static_cast<gboolean>(value), nullptr);
        }
    }

    if (format.GetValueType(std::string_view("max_encoder_fps")) == FORMAT_TYPE_INT32) {
        if (format.GetIntValue("max_encoder_fps", value) && value >= 0) {
            g_object_set(src_, "max-framerate", static_cast<uint32_t>(value), nullptr);
        }
    }

    if (format.GetValueType(std::string_view("repeat_frame_after")) == FORMAT_TYPE_INT32) {
        if (format.GetIntValue("repeat_frame_after", value) && value >= 0) {
            g_object_set(src_, "repeat", static_cast<uint32_t>(value), nullptr);
        }
    }

    return MSERR_OK;
}

int32_t SrcSurfaceImpl::NotifyEos()
{
    gboolean value = TRUE;
    g_object_set(src_, "notify-eos", value, nullptr);
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
