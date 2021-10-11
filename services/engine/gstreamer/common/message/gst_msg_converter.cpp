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

#include "gst_msg_converter.h"
#include <functional>
#include <unordered_map>
#include "media_errors.h"
#include "media_log.h"
#include "gst_utils.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "GstMsgConvDefault"};
}

namespace OHOS {
namespace Media {
static int32_t ConvertErrorMessage(GstMessage &gstMsg, InnerMessage &innerMsg)
{
    GError *error = nullptr;
    gchar *debug  = nullptr;
    gst_message_parse_error(const_cast<GstMessage *>(&gstMsg), &error, &debug);
    if (error == nullptr || debug == nullptr) {
        return MSERR_UNKNOWN;
    }
    MEDIA_LOGE("[ERROR] %{public}s, %{public}s", error->message, debug);

    innerMsg.type = INNER_MSG_ERROR;
    innerMsg.detail1 = MSERR_UNKNOWN;

    // need to add more detail error msg convert
    g_error_free(error);
    g_free(debug);
    return MSERR_OK;
}

static int32_t ConvertWarningMessage(GstMessage &gstMsg, InnerMessage &innerMsg)
{
    GError *error = nullptr;
    gchar *debug  = nullptr;
    gst_message_parse_warning(const_cast<GstMessage *>(&gstMsg), &error, &debug);
    if (error == nullptr || debug == nullptr) {
        return MSERR_UNKNOWN;
    }
    MEDIA_LOGW("[WARNING] %{public}s, %{public}s", error->message, debug);

    innerMsg.type = INNER_MSG_WARNING;
    innerMsg.detail1 = MSERR_UNKNOWN;

    g_error_free(error);
    g_free(debug);
    return MSERR_OK;
}

static int32_t ConvertInfoMessage(GstMessage &gstMsg, InnerMessage &innerMsg)
{
    GError *error = nullptr;
    gchar *debug  = nullptr;
    gst_message_parse_info(const_cast<GstMessage *>(&gstMsg), &error, &debug);
    if (error == nullptr || debug == nullptr) {
        return MSERR_UNKNOWN;
    }
    MEDIA_LOGI("[INFO] %{public}s, %{public}s", error->message, debug);

    innerMsg.type = INNER_MSG_INFO;
    innerMsg.detail1 = MSERR_UNKNOWN;

    g_error_free(error);
    g_free(debug);
    return MSERR_OK;
}

static int32_t ConvertStateChangedMessage(GstMessage &gstMsg, InnerMessage &innerMsg)
{
    GstState oldState = GST_STATE_VOID_PENDING;
    GstState newState = GST_STATE_VOID_PENDING;
    GstState pendingState = GST_STATE_VOID_PENDING;
    gst_message_parse_state_changed(const_cast<GstMessage *>(&gstMsg), &oldState, &newState, &pendingState);
    MEDIA_LOGI("%{public}s change state from %{public}s to %{public}s", ELEM_NAME(GST_MESSAGE_SRC(&gstMsg)),
        gst_element_state_get_name(oldState), gst_element_state_get_name(newState));

    innerMsg.type = INNER_MSG_STATE_CHANGED;
    innerMsg.detail1 = static_cast<int32_t>(oldState);
    innerMsg.detail2 = static_cast<int32_t>(newState);
    innerMsg.extend = gstMsg.src;

    return MSERR_OK;
}

static const std::unordered_map<GstMessageType, InnerMsgType> SIMPLE_MSG_TYPE_MAPPING = {
    { GST_MESSAGE_DURATION_CHANGED, INNER_MSG_DURATION_CHANGED },
    { GST_MESSAGE_EOS, INNER_MSG_EOS },
};

using MsgConvFunc = std::function<int32_t(GstMessage&, InnerMessage&)>;
static const std::unordered_map<GstMessageType, MsgConvFunc> MSG_CONV_FUNC_TABLE = {
    { GST_MESSAGE_ERROR, ConvertErrorMessage },
    { GST_MESSAGE_WARNING, ConvertWarningMessage },
    { GST_MESSAGE_INFO, ConvertInfoMessage },
    { GST_MESSAGE_STATE_CHANGED, ConvertStateChangedMessage },
};

int32_t GstMsgConverterDefault::ConvertToInnerMsg(GstMessage &gstMsg, InnerMessage &innerMsg) const
{
    innerMsg.type = INNER_MSG_UNKNOWN;

    if (SIMPLE_MSG_TYPE_MAPPING.count(gstMsg.type) != 0) {
        innerMsg.type = SIMPLE_MSG_TYPE_MAPPING.at(gstMsg.type);
        MEDIA_LOGI("convert gst msg type: %{public}s", GST_MESSAGE_TYPE_NAME(&gstMsg));
        return MSERR_OK;
    }

    if (MSG_CONV_FUNC_TABLE.count(gstMsg.type) == 0) {
        return MSERR_OK;
    }

    return MSG_CONV_FUNC_TABLE.at(gstMsg.type)(gstMsg, innerMsg);
}
}
}