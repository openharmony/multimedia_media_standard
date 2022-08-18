/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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
#include <unordered_map>
#include <map>
#include <vector>
#include <sstream>
#include "config.h"
#include <gst/video/video.h>
#include "securec.h"
#include "hdi_codec.h"
#include "gst_vdec_h264.h"
#include "gst_venc_h264.h"
#include "hdi_init.h"
#include "media_log.h"
#include "media_errors.h"
#include "scope_guard.h"
#include "hdi_vdec_in_buffer_mgr.h"
#include "hdi_vdec_out_buffer_mgr.h"
#include "hdi_vdec_params_mgr.h"
#include "hdi_venc_in_buffer_mgr.h"
#include "hdi_venc_out_buffer_mgr.h"
#include "hdi_venc_params_mgr.h"
#include "avcodec_ability_singleton.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "hdiPluginInit"};
    const std::string DEFAULT_H264_CAPS = "video/x-h264,"
        "alignment=(string) nal,"
        "stream-format=(string) byte-stream";
    using namespace OHOS::Media;
    const std::unordered_map<int32_t, std::string> FORMAT_MAPPING = {
        { NV21, "NV21" },
        { NV12, "NV12" },
        { YUVI420, "I420" },
    };
    const char *GST_CODEC_NAME = "codec_name";
    constexpr int32_t HDI_DEFAULT_SCORE = 2;
}

namespace OHOS {
namespace Media {
using CreateCodecFunc = std::shared_ptr<IGstCodec> (*)(GstElementClass *kclass);
using GetCapsStr = std::string (*)(CapabilityData &capData);

struct CapDataWarp {
    CapabilityData capData;
    std::string componentName;
};

class GstHdiFactory {
public:
    GstHdiFactory() = delete;
    ~GstHdiFactory() = delete;
    static std::shared_ptr<IGstCodec> CreateHdiVdecH264(GstElementClass *kclass);
    static std::shared_ptr<IGstCodec> CreateHdiVencH264(GstElementClass *kclass);
    static gboolean InputNeedCopy();
    static gboolean PluginInit(GstPlugin *plugin);
private:
    static void SetCreateFuncs(GstElementClass *elementClass, CapabilityData &capData);
    static GstCaps *GetSrcCaps(CapabilityData &capData);
    static GstCaps *GetSinkCaps(CapabilityData &capData);
    static inline std::string GetH264Caps(CapabilityData &capData)
    {
        (void)capData;
        return DEFAULT_H264_CAPS;
    }
    static std::string GetRawCaps(CapabilityData &capData);
    static void GetWidth(std::string &capStr, CapabilityData &capData);
    static void GetHeight(std::string &capStr, CapabilityData &capData);
    static void GetFrameRate(std::string &capStr, CapabilityData &capData);
    static void GetFormat(std::string &capStr, CapabilityData &capData);
    static void GstHdiCodecClassInit(gpointer kclass, gpointer data);
    static void UpdatePluginName(std::string &codecName);
    static gboolean HdiClassRegister(GstPlugin *plugin, CapabilityData &capData);
    const static std::map<std::pair<int32_t, std::string>, GType> COMPONENT_MAP;
    const static std::map<std::pair<int32_t, std::string>, GetCapsStr> SINK_CAPS_MAP;
    const static std::map<std::pair<int32_t, std::string>, GetCapsStr> SRC_CAPS_MAP;
    const static std::map<std::pair<int32_t, std::string>, CreateCodecFunc> FUNCTIONS_MAP;
};

const std::map<std::pair<int32_t, std::string>, GType> GstHdiFactory::COMPONENT_MAP = {
    {std::pair<int32_t, std::string>(AVCODEC_TYPE_VIDEO_DECODER, CodecMimeType::VIDEO_AVC), GST_TYPE_VDEC_H264},
    {std::pair<int32_t, std::string>(AVCODEC_TYPE_VIDEO_ENCODER, CodecMimeType::VIDEO_AVC), GST_TYPE_VENC_H264}
};

const std::map<std::pair<int32_t, std::string>, GetCapsStr> GstHdiFactory::SINK_CAPS_MAP = {
    {std::pair<int32_t, std::string>(AVCODEC_TYPE_VIDEO_DECODER, CodecMimeType::VIDEO_AVC), GetH264Caps},
    {std::pair<int32_t, std::string>(AVCODEC_TYPE_VIDEO_ENCODER, CodecMimeType::VIDEO_AVC), GetRawCaps}
};

const std::map<std::pair<int32_t, std::string>, GetCapsStr> GstHdiFactory::SRC_CAPS_MAP = {
    {std::pair<int32_t, std::string>(AVCODEC_TYPE_VIDEO_DECODER, CodecMimeType::VIDEO_AVC), GetRawCaps},
    {std::pair<int32_t, std::string>(AVCODEC_TYPE_VIDEO_ENCODER, CodecMimeType::VIDEO_AVC), GetH264Caps}
};

const std::map<std::pair<int32_t, std::string>, CreateCodecFunc> GstHdiFactory::FUNCTIONS_MAP = {
    {std::pair<int32_t, std::string>(AVCODEC_TYPE_VIDEO_DECODER, CodecMimeType::VIDEO_AVC),
        &GstHdiFactory::CreateHdiVdecH264},
    {std::pair<int32_t, std::string>(AVCODEC_TYPE_VIDEO_ENCODER, CodecMimeType::VIDEO_AVC),
        &GstHdiFactory::CreateHdiVencH264}
};

std::shared_ptr<IGstCodec> GstHdiFactory::CreateHdiVdecH264(GstElementClass *kclass)
{
    std::string component = gst_element_class_get_metadata(kclass, GST_CODEC_NAME);
    std::shared_ptr<HdiCodec> hdiCodec = std::make_shared<HdiCodec>(component);
    std::shared_ptr<HdiVdecInBufferMgr> inBufferMgr = std::make_shared<HdiVdecInBufferMgr>();
    std::shared_ptr<HdiVdecOutBufferMgr> outBufferMgr = std::make_shared<HdiVdecOutBufferMgr>();
    std::shared_ptr<HdiVdecParamsMgr> paramsMgr = std::make_shared<HdiVdecParamsMgr>();
    CHECK_AND_RETURN_RET_LOG(hdiCodec->Init() == GST_CODEC_OK, nullptr, "Init failed");
    hdiCodec->SetHdiInBufferMgr(inBufferMgr);
    hdiCodec->SetHdiOutBufferMgr(outBufferMgr);
    hdiCodec->SetHdiParamsMgr(paramsMgr);
    return hdiCodec;
}

std::shared_ptr<IGstCodec> GstHdiFactory::CreateHdiVencH264(GstElementClass *kclass)
{
    std::string component = gst_element_class_get_metadata(kclass, GST_CODEC_NAME);
    std::shared_ptr<HdiCodec> hdiCodec = std::make_shared<HdiCodec>(component);
    std::shared_ptr<HdiVencInBufferMgr> inBufferMgr = std::make_shared<HdiVencInBufferMgr>();
    std::shared_ptr<HdiVencOutBufferMgr> outBufferMgr = std::make_shared<HdiVencOutBufferMgr>();
    std::shared_ptr<HdiVencParamsMgr> paramsMgr = std::make_shared<HdiVencParamsMgr>();
    CHECK_AND_RETURN_RET_LOG(hdiCodec->Init() == GST_CODEC_OK, nullptr, "Init failed");
    hdiCodec->SetHdiInBufferMgr(inBufferMgr);
    hdiCodec->SetHdiOutBufferMgr(outBufferMgr);
    hdiCodec->SetHdiParamsMgr(paramsMgr);
    return hdiCodec;
}

gboolean GstHdiFactory::InputNeedCopy()
{
    return TRUE;
}

void GstHdiFactory::SetCreateFuncs(GstElementClass *elementClass, CapabilityData &capData)
{
    std::pair<int32_t, std::string> factoryPair = {capData.codecType, capData.mimeType};
    if (FUNCTIONS_MAP.find(factoryPair) == FUNCTIONS_MAP.end()) {
        return;
    }
    switch (capData.codecType) {
        case AVCODEC_TYPE_VIDEO_DECODER: {
            GstVdecBaseClass *vdecClass = reinterpret_cast<GstVdecBaseClass*>(elementClass);
            vdecClass->create_codec = FUNCTIONS_MAP.at(factoryPair);
            vdecClass->input_need_copy = InputNeedCopy;
            break;
        }
        case AVCODEC_TYPE_VIDEO_ENCODER: {
            GstVencBaseClass *vencClass = reinterpret_cast<GstVencBaseClass*>(elementClass);
            vencClass->create_codec = FUNCTIONS_MAP.at(factoryPair);
            break;
        }
        default:
            break;
    }
}

void GstHdiFactory::GetWidth(std::string &capStr, CapabilityData &capData)
{
    std::stringstream widthRange;
    widthRange << "(int) [ " << capData.width.minVal << ", " << capData.width.maxVal << " ]";
    capStr += "width = ";
    capStr += widthRange.str();
    capStr += ", ";
}

void GstHdiFactory::GetHeight(std::string &capStr, CapabilityData &capData)
{
    std::stringstream heightRange;
    heightRange << "(int) [ " << capData.height.minVal << ", " << capData.height.maxVal << " ]";
    capStr += "height = ";
    capStr += heightRange.str();
    capStr += ", ";
}

void GstHdiFactory::GetFrameRate(std::string &capStr, CapabilityData &capData)
{
    (void)capData;
    capStr += "framerate = (fraction) [ 0, max ]";
}

void GstHdiFactory::GetFormat(std::string &capStr, CapabilityData &capData)
{
    if (capData.format.empty()) {
        MEDIA_LOGW("No format");
        return;
    }
    for (auto format : capData.format) {
        if (FORMAT_MAPPING.find(format) == FORMAT_MAPPING.end()) {
            MEDIA_LOGW("Error format %{public}d", format);
            return;
        }
    }
    capStr += "format = (string) ";
    if (capData.format.size() == 1) {
        capStr += FORMAT_MAPPING.at(capData.format[0]);
    } else {
        capStr += "{ ";
        capStr += FORMAT_MAPPING.at(capData.format[0]);
        for (uint32_t i = 1; i < capData.format.size(); ++i) {
            capStr += ", ";
            capStr += FORMAT_MAPPING.at(capData.format[i]);
        }
        capStr += " }";
    }
    capStr += ", ";
}

std::string GstHdiFactory::GetRawCaps(CapabilityData &capData)
{
    std::string capStr = "video/x-raw, ";
    GetFormat(capStr, capData);
    GetWidth(capStr, capData);
    GetHeight(capStr, capData);
    GetFrameRate(capStr, capData);
    return capStr;
}

GstCaps *GstHdiFactory::GetSrcCaps(CapabilityData &capData)
{
    std::pair<int32_t, std::string> factoryPair = {capData.codecType, capData.mimeType};
    if (SRC_CAPS_MAP.find(factoryPair) != SRC_CAPS_MAP.end()) {
        return gst_caps_from_string(SRC_CAPS_MAP.at(factoryPair)(capData).c_str());
    }
    MEDIA_LOGD("Not find src caps");
    return nullptr;
}

GstCaps *GstHdiFactory::GetSinkCaps(CapabilityData &capData)
{
    std::pair<int32_t, std::string> factoryPair = {capData.codecType, capData.mimeType};
    if (SINK_CAPS_MAP.find(factoryPair) != SINK_CAPS_MAP.end()) {
        return gst_caps_from_string(SINK_CAPS_MAP.at(factoryPair)(capData).c_str());
    }
    MEDIA_LOGD("Not find sink caps");
    return nullptr;
}

void GstHdiFactory::UpdatePluginName(std::string &codecName)
{
    MEDIA_LOGD("Codec Name in %{public}s", codecName.c_str());
    for (auto &i : codecName) {
        if (i == '.') {
            i = '_';
        }
    }
    MEDIA_LOGD("Codec Name out %{public}s", codecName.c_str());
}

void GstHdiFactory::GstHdiCodecClassInit(gpointer kclass, gpointer data)
{
    MEDIA_LOGD("HdiClassInit");
    GstElementClass *elementClass = reinterpret_cast<GstElementClass*>(kclass);
    CapDataWarp *capDataWarp = reinterpret_cast<CapDataWarp *>(data);
    CHECK_AND_RETURN_LOG(elementClass != nullptr && capDataWarp != nullptr, "ClassInit cap is nullptr");
    CapabilityData &capData = capDataWarp->capData;
    GstCaps *sinkcaps = GetSinkCaps(capData);
    GstCaps *srccaps = GetSrcCaps(capData);
    // Caps must delete before return.
    ON_SCOPE_EXIT(0) {
        gst_caps_unref(sinkcaps);
        gst_caps_unref(srccaps);
    };
    CHECK_AND_RETURN_LOG(sinkcaps != nullptr && srccaps != nullptr, "Caps is nullptr");
    GstPadTemplate *sinktempl = gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS, sinkcaps);
    GstPadTemplate *srctempl = gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS, srccaps);
    ON_SCOPE_EXIT(1) {
        gst_object_unref(sinktempl);
        gst_object_unref(srctempl);
    };
    CHECK_AND_RETURN_LOG(sinktempl != nullptr && srctempl != nullptr, "Templ is nullptr");
    gst_element_class_add_pad_template(elementClass, srctempl);
    gst_element_class_add_pad_template(elementClass, sinktempl);
    gst_element_class_add_metadata(elementClass, GST_CODEC_NAME, capDataWarp->componentName.c_str());
    SetCreateFuncs(elementClass, capData);
    CANCEL_SCOPE_EXIT_GUARD(1);
}

gboolean GstHdiFactory::HdiClassRegister(GstPlugin *plugin, CapabilityData &capData)
{
    CapDataWarp *params = new CapDataWarp();
    CHECK_AND_RETURN_RET_LOG(params != nullptr, FALSE, "Params new failed");
    params->componentName = capData.codecName;
    UpdatePluginName(capData.codecName);
    std::string typeName = capData.codecName;
    params->capData = capData;
    // Params must delete before return.
    ON_SCOPE_EXIT(0) { delete params; };
    GType type = 0;
    GTypeQuery query;
    std::pair<int32_t, std::string> factoryPair = {capData.codecType, capData.mimeType};
    if (COMPONENT_MAP.find(factoryPair) != COMPONENT_MAP.end()) {
        type = COMPONENT_MAP.at(factoryPair);
    } else {
        MEDIA_LOGD("No type");
        return FALSE;
    }
    g_type_query(type, &query);
    GTypeInfo typeInfo = {};
    (void)memset_s(&typeInfo, sizeof(typeInfo), 0, sizeof(typeInfo));
    typeInfo.class_size = query.class_size;
    typeInfo.instance_size = query.instance_size;
    typeInfo.class_init = GstHdiCodecClassInit;
    typeInfo.class_data = params;
    MEDIA_LOGD("TypeName %{public}s", typeName.c_str());
    CHECK_AND_RETURN_RET_LOG(g_type_from_name(typeName.c_str()) == G_TYPE_INVALID, FALSE, "typeName exist");
    // In register, will GstHdiCodecClassInit.
    GType subtype = g_type_register_static(type, typeName.c_str(), &typeInfo, static_cast<GTypeFlags>(0));
    CHECK_AND_RETURN_RET_LOG(subtype != 0, FALSE, "Type register failed");
    return gst_element_register(plugin, typeName.c_str(), GST_RANK_NONE, subtype);
}

gboolean GstHdiFactory::PluginInit(GstPlugin *plugin)
{
    // hdi recommand
    MEDIA_LOGD("Plugin init");
    std::vector<CapabilityData> &&capDatas = HdiInit::GetInstance().GetCapabilitys();
    std::vector<CapabilityData> newCapDatas;
    for (auto &capData : capDatas) {
        if (HdiClassRegister(plugin, capData) != TRUE) {
            MEDIA_LOGD("Cant register %{public}s ", capData.codecName.c_str());
            continue;
        }
        newCapDatas.push_back(capData);
    }
    AVCodecAbilitySingleton::GetInstance().RegisterHdiCapability(newCapDatas);
    return TRUE;
}
}
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    _codec_plugin_hdi,
    "GStreamer Codec Source",
    OHOS::Media::GstHdiFactory::PluginInit,
    PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
