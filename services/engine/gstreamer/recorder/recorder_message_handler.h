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

#ifndef OHOS_MEDIA_RECORDER_MESSAGE_HANDLER
#define OHOS_MEDIA_RECORDER_MESSAGE_HANDLER

#include <gst/gstmessage.h>
#include "nocopyable.h"
#include "recorder_inner_defines.h"

namespace OHOS {
namespace Media {
/**
 * @brief Recorder engine internal message type.
 */
enum RecorderMessageType : int32_t {
    REC_MSG_ERROR,   // see IRecorderEngineObs::ErrorType in i_recorder_engine.h
    REC_MSG_INFO,    // see IRecorderEngineObs::InfoType in i_recorder_engine.h
    REC_MSG_FEATURE, // see RecorderMessageFeature
};

/**
 * @brief Recorder engine internal feature message type.
 */
enum RecorderMessageFeature : int32_t {
    REC_MSG_FEATURE_ASYNC_DONE,
    REC_MSG_FEATURE_EOS_DONE,
    REC_MSG_FEATURE_STATE_CHANGE_DONE,
};

/**
 * @brief Recorder engine internal message structure define.
 */
struct RecorderMessage {
    int32_t type;    // see RecorderMessageType
    int32_t code;
    int32_t detail;
    int32_t sourceId = INVALID_SOURCE_ID;
};

/**
 * @brief Recorder engine internal process raw message(GstMessage) result value.
 */
enum class RecorderMsgProcResult : uint8_t {
    REC_MSG_PROC_OK,
    REC_MSG_PROC_IGNORE,
    REC_MSG_PROC_FAILED,
};

/**
 * @brief Recorder message handler base type, the subclass inherites to it to implement the message proccess.
 *
 * If the gst message's format is ohos custom, the engine transparently transmits the encoding value to the user.
 * If the gst message's source is's not gstpipeline, the engine will deliver the message to RecorderMsgHandlers.
 * If there no RecorderMsgHandler to process the gst error messages, the engine will take over. For other kind
 * of messages, the engine will ignore it directly.
 *
 * The RecorderMsgHandlers should translate all the messages it receives as much as possible to give the
 * user a more friendly message prompt.
 */
class RecorderMsgHandler {
public:
    RecorderMsgHandler() = default;
    virtual ~RecorderMsgHandler() = default;

    virtual RecorderMsgProcResult OnMessageReceived(GstMessage &rawMsg, RecorderMessage &prettyMsg) = 0;

    static RecorderMsgProcResult ProcessInfoMsgDefault(GstMessage &rawMsg, RecorderMessage &prettyMsg);
    static RecorderMsgProcResult ProcessWarningMsgDefault(GstMessage &rawMsg, RecorderMessage &prettyMsg);
    static RecorderMsgProcResult ProcessErrorMsgDefault(GstMessage &rawMsg, RecorderMessage &prettyMsg);

    DISALLOW_COPY_AND_MOVE(RecorderMsgHandler);
};

#define GST_MSG_PARSER_DEFINE(type, Type)                               \
class Gst##Type##MsgParser {                                            \
public:                                                                 \
    explicit Gst##Type##MsgParser(GstMessage &msg) : msg_(msg) {}       \
    bool InitCheck()                                                    \
    {                                                                   \
        FreeErrAndDbg();                                                \
        gst_message_parse_##type(&msg_, &error_, &debug_);              \
        if (error_ == nullptr || debug_ == nullptr) {                   \
            return false;                                               \
        }                                                               \
        return true;                                                    \
    }                                                                   \
    ~Gst##Type##MsgParser()                                             \
    {                                                                   \
        FreeErrAndDbg();                                                \
    }                                                                   \
                                                                        \
    DISALLOW_COPY_AND_MOVE(Gst##Type##MsgParser);                       \
                                                                        \
    GError *error_ = nullptr;                                           \
    gchar *debug_ = nullptr;                                            \
    GstMessage &msg_;                                                   \
                                                                        \
private:                                                                \
    void FreeErrAndDbg()                                                \
    {                                                                   \
        if (error_ != nullptr) {                                        \
            g_error_free(error_);                                       \
        }                                                               \
        if (debug_ != nullptr) {                                        \
            g_free(debug_);                                             \
        }                                                               \
    }                                                                   \
}

GST_MSG_PARSER_DEFINE(info, Info);
GST_MSG_PARSER_DEFINE(warning, Warning);
GST_MSG_PARSER_DEFINE(error, Error);
}
}
#endif