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
static int32_t StreamDecErrorParse(const gchar *name)
{
    if (strstr(name, "aac") != nullptr) {
        MEDIA_LOGE("tag: MSERR_AUD_DEC_FAILED");
        return MSERR_AUD_DEC_FAILED;
    } else if (strstr(name, "h264") != nullptr || strstr(name, "h265") != nullptr) {
        MEDIA_LOGE("tag: MSERR_VID_DEC_FAILED");
        return MSERR_VID_DEC_FAILED;
    } else {
        MEDIA_LOGE("tag: MSERR_UNKNOWN");
        return MSERR_UNKNOWN;
    }
}

using StreamToServiceErrFunc = std::function<int32_t(const gchar*)>;
static const std::unordered_map<int32_t, StreamToServiceErrFunc> STREAM_TO_SERVICE_ERR_FUNC_TABLE = {
    { GST_STREAM_ERROR_DECODE, StreamDecErrorParse },
};
static const std::unordered_map<int32_t, MediaServiceErrCode> STREAM_TO_SERVICE_ERR_TABLE = {
    { GST_STREAM_ERROR_FORMAT, MSERR_UNSUPPORT_CONTAINER_TYPE },
    { GST_STREAM_ERROR_TYPE_NOT_FOUND, MSERR_NOT_FIND_CONTAINER },
    /* Currently, audio decoding and video decoding cannot be distinguished which is not supported by msg.
    * By default, we set video decoding is not supported.
    * The identification method must be added when the new demux pulgin in is use.
    */
    { GST_STREAM_ERROR_CODEC_NOT_FOUND, MSERR_UNSUPPORT_VID_DEC_TYPE },
    { GST_STREAM_ERROR_DEMUX, MSERR_DEMUXER_FAILED },
    { GST_STREAM_ERROR_WRONG_TYPE, MSERR_UNSUPPORT_FILE },
    { GST_STREAM_ERROR_FAILED, MSERR_UNSUPPORT_STREAM },
};
static const std::unordered_map<int32_t, MediaServiceErrCode> RESOURCE_TO_SERVICE_ERR_TABLE = {
    { GST_RESOURCE_ERROR_NOT_FOUND, MSERR_OPEN_FILE_FAILED },
    { GST_RESOURCE_ERROR_OPEN_READ, MSERR_OPEN_FILE_FAILED },
    { GST_RESOURCE_ERROR_READ, MSERR_FILE_ACCESS_FAILED },
    { GST_RESOURCE_ERROR_NOT_AUTHORIZED, MSERR_FILE_ACCESS_FAILED },
    { GST_RESOURCE_ERROR_TIME_OUT, MSERR_NETWORK_TIMEOUT },
};

static int32_t StreamErrorParse(const gchar *name, const GError *error)
{
    CHECK_AND_RETURN_RET_LOG(name != nullptr, MSERR_UNSUPPORT_STREAM, "name is nullptr");
    MEDIA_LOGE("domain: GST_STREAM_ERROR(%{public}d)", error->code);
    auto streamIter = STREAM_TO_SERVICE_ERR_TABLE.find(error->code);
    if (streamIter != STREAM_TO_SERVICE_ERR_TABLE.end()) {
        return streamIter->second;
    }
    auto streamFuncIter = STREAM_TO_SERVICE_ERR_FUNC_TABLE.find(error->code);
    if (streamFuncIter != STREAM_TO_SERVICE_ERR_FUNC_TABLE.end()) {
        return streamFuncIter->second(name);
    }

    return MSERR_UNSUPPORT_STREAM;
}

static int32_t ResourceErrorParse(const GError *error)
{
    MEDIA_LOGE("domain: GST_RESOURCE_ERROR");
    auto resIter = RESOURCE_TO_SERVICE_ERR_TABLE.find(error->code);
    if (resIter == RESOURCE_TO_SERVICE_ERR_TABLE.end()) {
        return MSERR_UNSUPPORT_SOURCE;
    }
    return resIter->second;
}

static int32_t ConvertErrorMessage(GstMessage &gstMsg, InnerMessage &innerMsg)
{
    GError *error = nullptr;
    gchar *debug  = nullptr;
    gst_message_parse_error(&gstMsg, &error, &debug);
    if (error == nullptr || debug == nullptr) {
        return MSERR_UNKNOWN;
    }
    MEDIA_LOGE("[ERROR] %{public}s, %{public}s", error->message, debug);

    gchar *name = gst_object_get_path_string(gstMsg.src);
    innerMsg.type = INNER_MSG_ERROR;
    if (error->domain == GST_STREAM_ERROR) {
        innerMsg.detail1 = StreamErrorParse(name, error);
    } else if (error->domain == GST_RESOURCE_ERROR) {
        innerMsg.detail1 = ResourceErrorParse(error);
    } else {
        innerMsg.detail1 = MSERR_UNKNOWN;
    }

    g_error_free(error);
    g_free(debug);
    g_free(name);
    return MSERR_OK;
}

static int32_t ConvertWarningMessage(GstMessage &gstMsg, InnerMessage &innerMsg)
{
    GError *error = nullptr;
    gchar *debug  = nullptr;
    gst_message_parse_warning(&gstMsg, &error, &debug);
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
    gst_message_parse_info(&gstMsg, &error, &debug);
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
    gst_message_parse_state_changed(&gstMsg, &oldState, &newState, &pendingState);
    MEDIA_LOGI("%{public}s change state from %{public}s to %{public}s", ELEM_NAME(GST_MESSAGE_SRC(&gstMsg)),
        gst_element_state_get_name(oldState), gst_element_state_get_name(newState));

    innerMsg.type = INNER_MSG_STATE_CHANGED;
    innerMsg.detail1 = static_cast<int32_t>(oldState);
    innerMsg.detail2 = static_cast<int32_t>(newState);
    if (GST_IS_PIPELINE(gstMsg.src)) {
        GstPipeline *pipeline = GST_PIPELINE(GST_MESSAGE_SRC(&gstMsg));
        innerMsg.extend = pipeline;
    }

    return MSERR_OK;
}

static int32_t ConvertAsyncDoneMessage(GstMessage &gstMsg, InnerMessage &innerMsg)
{
    innerMsg.type = INNER_MSG_ASYNC_DONE;
    if (GST_IS_PIPELINE(gstMsg.src)) {
        GstPipeline *pipeline = GST_PIPELINE(GST_MESSAGE_SRC(&gstMsg));
        innerMsg.extend = pipeline;
    }

    return MSERR_OK;
}

static int32_t ConvertResolutionChangedMessage(GstMessage &gstMsg, InnerMessage &innerMsg)
{
    gint width = 0;
    gint height = 0;
    gst_message_parse_resulution_changed(&gstMsg, &width, &height);
    MEDIA_LOGI("resolution changed to width:%{public}d height:%{public}d", width, height);

    innerMsg.type = INNER_MSG_RESOLUTION_CHANGED;
    innerMsg.detail1 = width;
    innerMsg.detail2 = height;
    return MSERR_OK;
}

static int32_t ConvertBufferingTimeMessage(GstMessage &gstMsg, InnerMessage &innerMsg)
{
    gint64 bufferingTime;
    guint mqNumId;
    gst_message_parse_buffering_time(&gstMsg, &bufferingTime, &mqNumId);
    MEDIA_LOGI("mqNumId = %{public}u, bufferingTime = %{public}" PRIi64 "", mqNumId, bufferingTime);

    innerMsg.type = INNER_MSG_BUFFERING_TIME;
    innerMsg.detail1 = static_cast<int32_t>(mqNumId);
    innerMsg.extend = bufferingTime;
    return MSERR_OK;
}

static int32_t ConvertUsedMqNumMessage(GstMessage &gstMsg, InnerMessage &innerMsg)
{
    guint usedMqNum;
    gst_message_parse_mq_num_use_buffering(&gstMsg, &usedMqNum);
    MEDIA_LOGI("used multiqueue num for buffering is %{public}u", usedMqNum);

    innerMsg.type = INNER_MSG_BUFFERING_USED_MQ_NUM;
    innerMsg.detail1 = static_cast<int32_t>(usedMqNum);
    return MSERR_OK;
}

static int32_t ConvertElementMessage(GstMessage &gstMsg, InnerMessage &innerMsg)
{
    const GstStructure *s = gst_message_get_structure(&gstMsg);
    if (gst_structure_has_name(s, "resolution-changed")) {
        return ConvertResolutionChangedMessage(gstMsg, innerMsg);
    } else if (gst_structure_has_name(s, "message-buffering-time")) {
        return ConvertBufferingTimeMessage(gstMsg, innerMsg);
    } else if (gst_structure_has_name(s, "message-mq-num-use-buffering")) {
        return ConvertUsedMqNumMessage(gstMsg, innerMsg);
    }

    return MSERR_OK;
}

static int32_t ConvertBufferingMessage(GstMessage &gstMsg, InnerMessage &innerMsg)
{
    gchar *eleName = gst_element_get_name(GST_MESSAGE_SRC(&gstMsg));
    if ((GST_MESSAGE_SRC(&gstMsg) != nullptr) && (strncmp(eleName, "queue2", strlen("queue2")) == 0)) {
        MEDIA_LOGD("buffering msg comes from queue2, do not handle it");
        g_free(eleName);
        return MSERR_OK;
    }

    gint percent;
    gst_message_parse_buffering (&gstMsg, &percent);
    MEDIA_LOGI("multiqueue percent is %{public}d", percent);

    innerMsg.type = INNER_MSG_BUFFERING;
    innerMsg.detail1 = percent;
    g_free(eleName);
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
    { GST_MESSAGE_ASYNC_DONE, ConvertAsyncDoneMessage },
    { GST_MESSAGE_ELEMENT, ConvertElementMessage },
    { GST_MESSAGE_BUFFERING, ConvertBufferingMessage },
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
} // namespace Media
} // namespace OHOS