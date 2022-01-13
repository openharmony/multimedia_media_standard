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

#ifndef I_CODEC_PARAMS_MGR_H
#define I_CODEC_PARAMS_MGR_H

#include <gst/gst.h>
#include "nocopyable.h"
#include "i_codec_common.h"

namespace OHOS {
namespace Media {
class ICodecParamsMgr {
public:
    virtual ~ICodecParamsMgr() = default;

    /**
     * @brief Set param with key, and value is in element.
     *
     * Every codec need to set param before start.
     *
     * @return Returns GST_CODEC_OK successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetParameter(GstCodecParamKey key, GstElement *element) = 0;

    /**
     * @brief update the value in element with key.
     *
     * Suggest to get parameter before set.
     *
     * @return Returns GST_CODEC_OK successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t GetParameter(GstCodecParamKey key, GstElement *element) = 0;
};
} // namespace Media
} // namespace OHOS
#endif // I_CODEC_PARAMS_MGR_H
