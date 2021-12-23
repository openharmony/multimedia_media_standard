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

#ifndef COMMON_NAPI_H
#define COMMON_NAPI_H

#include <string>
#include <vector>
#include <unordered_map>
#include "format.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace Media {
const std::unordered_map<std::string, FormatDataType> FORMAT_DATA = {
    {"track_index", FORMAT_TYPE_INT32},
    {"track_type", FORMAT_TYPE_INT32},
    {"codec_mime", FORMAT_TYPE_STRING},
    {"duration", FORMAT_TYPE_INT32},
    {"bitrate", FORMAT_TYPE_INT32},
    {"max_input_size", FORMAT_TYPE_INT32},
    {"max_encoder_fps", FORMAT_TYPE_INT32},
    {"width", FORMAT_TYPE_INT32},
    {"height", FORMAT_TYPE_INT32},
    {"pixel_format", FORMAT_TYPE_INT32},
    {"audio_raw_format", FORMAT_TYPE_INT32},
    {"frame_rate", FORMAT_TYPE_INT32},
    {"capture_rate", FORMAT_TYPE_INT32},
    {"i_frame_interval", FORMAT_TYPE_INT32},
    {"req_i_frame", FORMAT_TYPE_INT32},
    {"repeat_frame_after", FORMAT_TYPE_INT32},
    {"suspend_input_surface", FORMAT_TYPE_INT32},
    {"video_encode_bitrate_mode", FORMAT_TYPE_INT32},
    {"codec_profile", FORMAT_TYPE_INT32},
    {"codec_quality", FORMAT_TYPE_INT32},
    {"rect_top", FORMAT_TYPE_INT32},
    {"rect_bottom", FORMAT_TYPE_INT32},
    {"rect_left", FORMAT_TYPE_INT32},
    {"rect_right", FORMAT_TYPE_INT32},
    {"color_standard", FORMAT_TYPE_INT32},
    {"channel_count", FORMAT_TYPE_INT32},
    {"sample_rate", FORMAT_TYPE_INT32},
    {"vendor.custom", FORMAT_TYPE_ADDR},
};

class CommonNapi {
public:
    CommonNapi() = delete;
    ~CommonNapi() = delete;
    static std::string GetStringArgument(napi_env env, napi_value value);
    static bool GetPropertyInt32(napi_env env, napi_value configObj, const std::string &type, int32_t &result);
    static bool GetPropertyDouble(napi_env env, napi_value configObj, const std::string &type, double &result);
    static std::string GetPropertyString(napi_env env, napi_value configObj, const std::string &type);
    static napi_status FillErrorArgs(napi_env env, int32_t errCode, const napi_value &args);
    static napi_status CreateError(napi_env env, int32_t errCode, const std::string &errMsg, napi_value &errVal);
    static napi_ref CreateReference(napi_env env, napi_value arg);
    static napi_deferred CreatePromise(napi_env env, napi_ref ref, napi_value &result);
    static bool SetPropertyInt32(napi_env env, napi_value &obj, const std::string &key, int32_t value);
    static bool SetPropertyString(napi_env env, napi_value &obj, const std::string &key, const std::string &value);
    static napi_value CreateFormatBuffer(napi_env env, Format &format);
    static bool AddRangeProperty(napi_env env, napi_value obj, const std::string &name, int32_t min, int32_t max);
    static bool AddArrayProperty(napi_env env, napi_value obj, const std::string &name,
        const std::vector<int32_t> &vec);
};

class MediaJsResult {
public:
    virtual ~MediaJsResult() = default;
    virtual napi_status GetJsResult(napi_env env, napi_value &result) = 0;
};

class MediaJsResultInt : public MediaJsResult {
public:
    explicit MediaJsResultInt(int32_t value)
        : value_(value)
    {
    }
    ~MediaJsResultInt() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override
    {
        return napi_create_int32(env, value_, &result);
    }
private:
    int32_t value_;
};

class MediaJsResultString : public MediaJsResult {
public:
    explicit MediaJsResultString(const std::string &value)
        : value_(value)
    {
    }
    ~MediaJsResultString() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override
    {
        return napi_create_string_utf8(env, value_.c_str(), NAPI_AUTO_LENGTH, &result);
    }

private:
    std::string value_;
};

class MediaJsResultArray : public MediaJsResult {
public:
    explicit MediaJsResultArray(const std::vector<Format> &value)
        : value_(value)
    {
    }
    ~MediaJsResultArray() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override;

private:
    std::vector<Format> value_;
};

class MediaJsResultInstance : public MediaJsResult {
public:
    explicit MediaJsResultInstance(const napi_ref &constructor)
        : constructor_(constructor)
    {
    }
    ~MediaJsResultInstance() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override
    {
        napi_value constructor = nullptr;
        napi_status ret = napi_get_reference_value(env, constructor_, &constructor);
        if (ret != napi_ok || constructor == nullptr) {
            return ret;
        }
        return napi_new_instance(env, constructor, 0, nullptr, &result);
    }

private:
    napi_ref constructor_;
};

class AVCodecJsResultCtor : public MediaJsResult {
public:
    explicit AVCodecJsResultCtor(const napi_ref &constructor, int32_t isMimeType, const std::string &name)
        : constructor_(constructor),
          isMimeType_(isMimeType),
          name_(name)
    {
    }
    ~AVCodecJsResultCtor() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override
    {
        napi_value constructor = nullptr;
        napi_status ret = napi_get_reference_value(env, constructor_, &constructor);
        if (ret != napi_ok || constructor == nullptr) {
            return ret;
        }

        napi_value args[2] = { nullptr };
        ret = napi_create_string_utf8(env, name_.c_str(), NAPI_AUTO_LENGTH, &args[0]);
        if (ret != napi_ok) {
            return ret;
        }

        ret = napi_create_int32(env, isMimeType_, &args[1]);
        if (ret != napi_ok) {
            return ret;
        }

        return napi_new_instance(env, constructor, 2, args, &result);
    }

private:
    napi_ref constructor_;
    int32_t isMimeType_ = 0;
    std::string name_ = "";
};

class AVCodecJsResultFormat : public MediaJsResult {
public:
    explicit AVCodecJsResultFormat(const Format &format)
        : format_(format)
    {
    }
    ~AVCodecJsResultFormat() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override
    {
        (void)format_;
        (void)env;
        (void)result;
        return napi_ok;
    }

private:
    Format format_;
};

class MediaCapsJsResultAudio : public MediaJsResult {
public:
    explicit MediaCapsJsResultAudio(bool isDecoder)
        : isDecoder_(isDecoder)
    {
    }
    ~MediaCapsJsResultAudio() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override;

private:
    bool isDecoder_;
};

struct MediaAsyncContext {
    explicit MediaAsyncContext(napi_env env) : env(env) {}
    virtual ~MediaAsyncContext() = default;
    static void CompleteCallback(napi_env env, napi_status status, void *data);
    void SignError(int32_t code, std::string message);
    napi_env env;
    napi_async_work work = nullptr;
    napi_deferred deferred = nullptr;
    napi_ref callbackRef = nullptr;
    std::unique_ptr<MediaJsResult> JsResult;
    bool errFlag = false;
    int32_t errCode = 0;
    std::string errMessage = "";
};

struct AutoRef {
    AutoRef(napi_env env, napi_ref cb)
        : env_(env), cb_(cb)
    {
    }
    ~AutoRef()
    {
        if (env_ != nullptr && cb_ != nullptr) {
            (void)napi_delete_reference(env_, cb_);
        }
    }
    napi_env env_;
    napi_ref cb_;
};
}
}
#endif
