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

#include "avcodec_napi_utils.h"
#include <map>
#include "common_napi.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecNapiUtil"};
    const std::map<std::string, OHOS::Media::FormatDataType> FORMAT = {
        {"codec_mime", OHOS::Media::FORMAT_TYPE_STRING},
        {"audio_raw_format", OHOS::Media::FORMAT_TYPE_INT32},
        {"bitrate", OHOS::Media::FORMAT_TYPE_INT32},
        {"max_input_size", OHOS::Media::FORMAT_TYPE_INT32},
        {"max_encoder_fps", OHOS::Media::FORMAT_TYPE_INT32},
        {"width", OHOS::Media::FORMAT_TYPE_INT32},
        {"height", OHOS::Media::FORMAT_TYPE_INT32},
        {"pixel_format", OHOS::Media::FORMAT_TYPE_INT32},
        {"frame_rate", OHOS::Media::FORMAT_TYPE_INT32},
        {"capture_rate", OHOS::Media::FORMAT_TYPE_INT32},
        {"i_frame_interval", OHOS::Media::FORMAT_TYPE_INT32},
        {"req_i_frame", OHOS::Media::FORMAT_TYPE_INT32},
        {"repeate_frame_after", OHOS::Media::FORMAT_TYPE_INT32},
        {"suspend_input_surface", OHOS::Media::FORMAT_TYPE_INT32},
        {"video_encode_bitrate_mode", OHOS::Media::FORMAT_TYPE_INT32},
        {"codec_profile", OHOS::Media::FORMAT_TYPE_INT32},
        {"codec_quality", OHOS::Media::FORMAT_TYPE_INT32},
        {"rect_top", OHOS::Media::FORMAT_TYPE_INT32},
        {"rect_bottom", OHOS::Media::FORMAT_TYPE_INT32},
        {"rect_left", OHOS::Media::FORMAT_TYPE_INT32},
        {"rect_right", OHOS::Media::FORMAT_TYPE_INT32},
        {"color_standard", OHOS::Media::FORMAT_TYPE_INT32},
        {"channel_count", OHOS::Media::FORMAT_TYPE_INT32},
        {"sample_rate", OHOS::Media::FORMAT_TYPE_INT32},
        {"vendor.custom", OHOS::Media::FORMAT_TYPE_ADDR},
    };
}

namespace OHOS {
namespace Media {
struct AvMemNapiWarp {
    explicit AvMemNapiWarp(const std::shared_ptr<AVSharedMemory> &mem) : mem_(mem)
    {
        MEDIA_LOGD("0x%{public}06" PRIXPTR " AvMemNapiWarp Instances create", FAKE_POINTER(this));
    };
    ~AvMemNapiWarp()
    {
        MEDIA_LOGD("0x%{public}06" PRIXPTR " AvMemNapiWarp Instances destroy", FAKE_POINTER(this));
    };
    std::shared_ptr<AVSharedMemory> mem_;
};

napi_value AVCodecNapiUtil::CreateInputCodecBuffer(napi_env env, uint32_t index, std::shared_ptr<AVSharedMemory> mem)
{
    CHECK_AND_RETURN_RET(mem != nullptr, nullptr);

    napi_value buffer = nullptr;
    napi_status status = napi_create_object(env, &buffer);
    CHECK_AND_RETURN_RET(status == napi_ok, nullptr);

    CHECK_AND_RETURN_RET(AddNumberProp(env, buffer, "timeMs", 0) == true, nullptr);
    CHECK_AND_RETURN_RET(AddNumberProp(env, buffer, "index", static_cast<int32_t>(index)) == true, nullptr);
    CHECK_AND_RETURN_RET(AddNumberProp(env, buffer, "offset", 0) == true, nullptr);
    CHECK_AND_RETURN_RET(AddNumberProp(env, buffer, "length", mem->GetSize()) == true, nullptr);
    CHECK_AND_RETURN_RET(AddNumberProp(env, buffer, "flags", 0) == true, nullptr);

    napi_value dataStr = nullptr;
    status = napi_create_string_utf8(env, "data", NAPI_AUTO_LENGTH, &dataStr);
    CHECK_AND_RETURN_RET(status == napi_ok, nullptr);

    AvMemNapiWarp *memWarp = new(std::nothrow) AvMemNapiWarp(mem);
    CHECK_AND_RETURN_RET(memWarp != nullptr, nullptr);
    napi_value dataVal = nullptr;
    status = napi_create_external_arraybuffer(env, mem->GetBase(), static_cast<size_t>(mem->GetSize()),
        [](napi_env env, void *data, void *hint) {
            (void)env;
            (void)data;
            AvMemNapiWarp *memWarp = reinterpret_cast<AvMemNapiWarp *>(hint);
            delete memWarp;
        }, reinterpret_cast<void *>(memWarp), &dataVal);
    CHECK_AND_RETURN_RET(status == napi_ok, nullptr);

    status = napi_set_property(env, buffer, dataStr, dataVal);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set property");

    return buffer;
}

napi_value AVCodecNapiUtil::CreateOutputCodecBuffer(napi_env env, uint32_t index,
    std::shared_ptr<AVSharedMemory> memory, const AVCodecBufferInfo &info, AVCodecBufferFlag flag)
{
    if (flag & AVCODEC_BUFFER_FLAG_EOS) {
        MEDIA_LOGI("Return empty buffer with eos flag");
        return CreateEmptyEOSBuffer(env);
    }

    napi_value buffer = nullptr;
    napi_status status = napi_create_object(env, &buffer);
    CHECK_AND_RETURN_RET(status == napi_ok, nullptr);

    const int32_t msToUs = 1000;
    CHECK_AND_RETURN_RET(AddNumberProp(env, buffer, "timeMs", info.presentationTimeUs / msToUs) == true, nullptr);
    CHECK_AND_RETURN_RET(AddNumberProp(env, buffer, "index", static_cast<int32_t>(index)) == true, nullptr);
    CHECK_AND_RETURN_RET(AddNumberProp(env, buffer, "offset", 0) == true, nullptr);
    CHECK_AND_RETURN_RET(AddNumberProp(env, buffer, "length", info.size) == true, nullptr);
    CHECK_AND_RETURN_RET(AddNumberProp(env, buffer, "flags", static_cast<int32_t>(flag)) == true, nullptr);

    if (memory != nullptr) {
        napi_value dataStr = nullptr;
        status = napi_create_string_utf8(env, "data", NAPI_AUTO_LENGTH, &dataStr);
        CHECK_AND_RETURN_RET(status == napi_ok, nullptr);

        CHECK_AND_RETURN_RET(memory->GetSize() > (info.offset + info.size), nullptr);
        napi_value dataVal = nullptr;
        AvMemNapiWarp *memWarp = new(std::nothrow) AvMemNapiWarp(memory);
        CHECK_AND_RETURN_RET(memWarp != nullptr, nullptr);
        status = napi_create_external_arraybuffer(env, memory->GetBase() + info.offset, info.size,
            [](napi_env env, void *data, void *hint) {
                (void)env;
                (void)data;
                AvMemNapiWarp *memWarp = reinterpret_cast<AvMemNapiWarp *>(hint);
                delete memWarp;
            }, reinterpret_cast<void *>(memWarp), &dataVal);
        CHECK_AND_RETURN_RET(status == napi_ok, nullptr);

        status = napi_set_property(env, buffer, dataStr, dataVal);
        CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set property");
    }

    return buffer;
}

napi_value AVCodecNapiUtil::CreateEmptyEOSBuffer(napi_env env)
{
    napi_value buffer = nullptr;
    napi_status status = napi_create_object(env, &buffer);
    CHECK_AND_RETURN_RET(status == napi_ok, nullptr);

    (void)AddNumberProp(env, buffer, "timeMs", -1);
    (void)AddNumberProp(env, buffer, "index", -1);
    (void)AddNumberProp(env, buffer, "offset", 0);
    (void)AddNumberProp(env, buffer, "length", 0);
    if (AddNumberProp(env, buffer, "flags", static_cast<int32_t>(AVCODEC_BUFFER_FLAG_EOS)) != true) {
        MEDIA_LOGE("Failed to add eos flag for empty buffer");
        return nullptr;
    }

    return buffer;
}

bool AVCodecNapiUtil::AddNumberProp(napi_env env, napi_value obj, const std::string &key, int32_t value)
{
    CHECK_AND_RETURN_RET(obj != nullptr, false);

    napi_value keyNapi = nullptr;
    napi_status status = napi_create_string_utf8(env, key.c_str(), NAPI_AUTO_LENGTH, &keyNapi);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    napi_value valueNapi = nullptr;
    status = napi_create_int32(env, value, &valueNapi);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    status = napi_set_property(env, obj, keyNapi, valueNapi);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, false, "Failed to set property");

    return true;
}

bool AVCodecNapiUtil::ExtractCodecBuffer(napi_env env, napi_value buffer, int32_t &index,
    AVCodecBufferInfo &info, AVCodecBufferFlag &flag)
{
    CHECK_AND_RETURN_RET(buffer != nullptr, false);

    CHECK_AND_RETURN_RET(CommonNapi::GetPropertyInt32(env, buffer, "index", index) == true, false);
    CHECK_AND_RETURN_RET(CommonNapi::GetPropertyInt32(env, buffer, "offset", info.offset) == true, false);
    CHECK_AND_RETURN_RET(CommonNapi::GetPropertyInt32(env, buffer, "length", info.size) == true, false);

    int32_t tmpFlag = 0;
    CHECK_AND_RETURN_RET(CommonNapi::GetPropertyInt32(env, buffer, "flags", tmpFlag) == true, false);
    flag = static_cast<AVCodecBufferFlag>(tmpFlag);

    int32_t timeMs = 0;
    CHECK_AND_RETURN_RET(CommonNapi::GetPropertyInt32(env, buffer, "timeMs", timeMs) == true, false);
    const int32_t msToUs = 1000;
    info.presentationTimeUs = msToUs * timeMs;

    return true;
}

bool AVCodecNapiUtil::ExtractMediaFormat(napi_env env, napi_value mediaFormat, Format &format)
{
    CHECK_AND_RETURN_RET(mediaFormat != nullptr, false);

    for (auto it = FORMAT.begin(); it != FORMAT.end(); it++) {
        if (it->second == FORMAT_TYPE_STRING) {
            bool exist = false;
            if (napi_has_named_property(env, mediaFormat, it->first.c_str(), &exist) == napi_ok && exist) {
                napi_value item = nullptr;
                CHECK_AND_CONTINUE(napi_get_named_property(env, mediaFormat, it->first.c_str(), &item) == napi_ok);
                format.PutStringValue(it->first, CommonNapi::GetStringArgument(env, item));
            }
        } else if (it->second == FORMAT_TYPE_INT32) {
            bool exist = false;
            if (napi_has_named_property(env, mediaFormat, it->first.c_str(), &exist) == napi_ok && exist) {
                napi_value item = nullptr;
                CHECK_AND_CONTINUE(napi_get_named_property(env, mediaFormat, it->first.c_str(), &item) == napi_ok);
                int32_t result = 0;
                (void)napi_get_value_int32(env, item, &result);
                format.PutIntValue(it->first, result);
            }
        }
    }

    return true;
}
}
}
