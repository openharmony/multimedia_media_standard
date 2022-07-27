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

#ifndef OMX_CODEC_UTIL_H
#define OMX_CODEC_UTIL_H

#include <memory>
#include <gst/gst.h>
#include <gst/video/gstvideodecoder.h>
#include "securec.h"
#include "i_gst_codec.h"
#include "OMX_Core.h"
#include "OMX_Component.h"
#include "display_type.h"
#include "codec_component_if.h"
#include "codec_omx_ext.h"

namespace OHOS {
namespace Media {
class HdiCodecUtil {
public:
    static OMX_VIDEO_CODINGTYPE CompressionGstToHdi(GstCompressionFormat format);
    static PixelFormat FormatGstToHdi(GstVideoFormat format);
    static GstVideoFormat FormatHdiToGst(PixelFormat format);
    static OMX_COLOR_FORMATTYPE FormatGstToOmx(GstVideoFormat format);
    static GstVideoFormat FormatOmxToGst(OMX_COLOR_FORMATTYPE format);
private:
    HdiCodecUtil() = delete;
    ~HdiCodecUtil() = delete;
};

template <typename T>
inline void InitParam(T &param, CompVerInfo &verInfo)
{
    memset_s(&param, sizeof(param), 0x0, sizeof(param));
    param.nSize = sizeof(param);
    param.nVersion = verInfo.compVersion;
}

template <typename T>
inline void InitHdiParam(T &param, CompVerInfo &verInfo)
{
    memset_s(&param, sizeof(param), 0x0, sizeof(param));
    param.size = sizeof(param);
    param.version = verInfo.compVersion;
}

template <typename T, typename U>
inline int32_t HdiSetParameter(T *handle, uint32_t paramIndex, U &param)
{
    return handle->SetParameter(handle, paramIndex, reinterpret_cast<int8_t *>(&param), sizeof(param));
}

template <typename T, typename U>
inline int32_t HdiGetParameter(T *handle, uint32_t paramIndex, U &param)
{
    return handle->GetParameter(handle, paramIndex, reinterpret_cast<int8_t *>(&param), sizeof(param));
}

template <typename T, typename U>
inline int32_t HdiGetConfig(T *handle, uint32_t paramIndex, U &param)
{
    return handle->GetConfig(handle, paramIndex, reinterpret_cast<int8_t *>(&param), sizeof(param));
}

template <typename T, typename U>
inline int32_t HdiSetConfig(T *handle, uint32_t paramIndex, U &param)
{
    return handle->SetConfig(handle, paramIndex, reinterpret_cast<int8_t *>(&param), sizeof(param));
}

template <typename T, typename U>
inline int32_t HdiSendCommand(T *handle, OMX_COMMANDTYPE cmd, uint32_t param, U &&cmdData)
{
    return handle->SendCommand(handle, cmd, param, reinterpret_cast<int8_t *>(&cmdData), sizeof(cmdData));
}

template <typename T, typename U>
inline int32_t HdiFillThisBuffer(T *handle, U *buffer)
{
    return handle->FillThisBuffer(handle, buffer);
}

template <typename T, typename U>
inline int32_t HdiEmptyThisBuffer(T *handle, U *buffer)
{
    return handle->EmptyThisBuffer(handle, buffer);
}

template <typename T>
inline void EmptyList(T &list)
{
    T temp;
    list.swap(temp);
}
} // namespace Media
} // namespace OHOS
#endif // OMX_CODEC_UTIL_H