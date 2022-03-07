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

#include "napi_demo.h"
#include <random>
#include <sync_fence.h>
#include "scope_guard.h"
#include "media_errors.h"
#include "media_log.h"
#include "display_type.h"
#include "securec.h"
#include "surface_utils.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "NapiDemo"};
    constexpr uint32_t STRIDE_ALIGN = 8;
}

namespace OHOS {
namespace Media {
napi_ref NapiDemo::constructor_ = nullptr;
const std::string CLASS_NAME = "MediaTest";

NapiDemo::NapiDemo()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

NapiDemo::~NapiDemo()
{
    if (wrap_ != nullptr) {
        napi_delete_reference(env_, wrap_);
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

napi_value NapiDemo::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("startStream", StartStream),
        DECLARE_NAPI_FUNCTION("closeStream", CloseStream),
        DECLARE_NAPI_FUNCTION("setResolution", SetResolution),
        DECLARE_NAPI_FUNCTION("setFrameCount", SetFrameCount),
        DECLARE_NAPI_FUNCTION("setFrameRate", SetFrameRate),
    };
    napi_property_descriptor staticProperty[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createMediaTest", CreateMediaTest),
    };

    napi_value constructor = nullptr;
    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define AudioDecodeProcessor class");

    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to create reference of constructor");

    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set constructor");

    status = napi_define_properties(env, exports, sizeof(staticProperty) / sizeof(staticProperty[0]), staticProperty);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define static function");

    MEDIA_LOGD("Init success");
    return exports;
}

napi_value NapiDemo::Constructor(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    napi_value jsThis = nullptr;
    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET(status == napi_ok, result);

    NapiDemo *napiDemo = new(std::nothrow) NapiDemo();
    CHECK_AND_RETURN_RET(napiDemo != nullptr, result);

    napiDemo->env_ = env;

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(napiDemo),
        NapiDemo::Destructor, nullptr, &(napiDemo->wrap_));
    if (status != napi_ok) {
        delete napiDemo;
        MEDIA_LOGE("Failed to wrap native instance");
        return result;
    }

    MEDIA_LOGD("Constructor success");
    return jsThis;
}

void NapiDemo::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    (void)env;
    (void)finalize;
    if (nativeObject != nullptr) {
        delete reinterpret_cast<NapiDemo *>(nativeObject);
    }
    MEDIA_LOGD("Destructor success");
}

napi_value NapiDemo::CreateMediaTest(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter CreateMediaTest");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    napi_value constructor = nullptr;
    napi_status status = napi_get_reference_value(env, constructor_, &constructor);
    CHECK_AND_RETURN_RET(status == napi_ok, result);

    status = napi_new_instance(env, constructor, 0, nullptr, &result);
    CHECK_AND_RETURN_RET(status == napi_ok, result);

    MEDIA_LOGD("CreateMediaTest success");
    return result;
}

napi_value NapiDemo::StartStream(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter StartStream");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET(status == napi_ok && jsThis != nullptr, result);

    NapiDemo *napiDemo = nullptr;
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&napiDemo));
    CHECK_AND_RETURN_RET(napiDemo != nullptr, result);

    std::string surfaceId = "";
    napi_valuetype type = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &type) == napi_ok && type == napi_string) {
        surfaceId = GetStringArgument(env, args[0]);
    }

    uint64_t surfaceUniqueId = 0;
    StrToUint64(surfaceId, surfaceUniqueId);

    napiDemo->producerSurface_ = SurfaceUtils::GetInstance()->GetSurface(surfaceUniqueId);
    CHECK_AND_RETURN_RET(napiDemo->producerSurface_ != nullptr, result);

    if (napiDemo->bufferThread_ != nullptr) {
        napiDemo->isStart_.store(false);
        napiDemo->bufferThread_->join();
        napiDemo->bufferThread_.reset();
        napiDemo->bufferThread_ = nullptr;
    }

    napiDemo->isStart_.store(true);
    napiDemo->requestConfig_ = {
        .width = napiDemo->width_,
        .height = napiDemo->height_,
        .strideAlignment = STRIDE_ALIGN,
        .format = PIXEL_FMT_YCRCB_420_SP,
        .usage = HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA,
        .timeout = 0
    };

    napiDemo->flushConfig_ = {
        .damage = {
            .x = 0,
            .y = 0,
            .w = napiDemo->width_,
            .h = napiDemo->height_
        },
        .timestamp = 0
    };
    napiDemo->bufferThread_ = std::make_unique<std::thread>(&NapiDemo::BufferLoop, napiDemo);

    MEDIA_LOGD("StartStream success");
    return result;
}

napi_value NapiDemo::CloseStream(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter CloseStream");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    napi_value jsThis = nullptr;
    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET(status == napi_ok && jsThis != nullptr, result);

    NapiDemo *napiDemo = nullptr;
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&napiDemo));
    CHECK_AND_RETURN_RET(napiDemo != nullptr, result);

    napiDemo->isStart_.store(false);
    if (napiDemo->bufferThread_ != nullptr) {
        napiDemo->bufferThread_->join();
        napiDemo->bufferThread_.reset();
        napiDemo->bufferThread_ = nullptr;
    }

    MEDIA_LOGD("CloseStream success");
    return result;
}

napi_value NapiDemo::SetResolution(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter SetResolution");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    napi_value jsThis = nullptr;
    napi_value args[2] = {nullptr};
    size_t argCount = 2;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET(status == napi_ok && jsThis != nullptr, result);

    NapiDemo *napiDemo = nullptr;
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&napiDemo));
    CHECK_AND_RETURN_RET(napiDemo != nullptr, result);

    napi_valuetype type = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &type) == napi_ok && type == napi_number) {
        (void)napi_get_value_int32(env, args[0], &napiDemo->width_);
    }

    if (args[1] != nullptr && napi_typeof(env, args[1], &type) == napi_ok && type == napi_number) {
        (void)napi_get_value_int32(env, args[1], &napiDemo->height_);
    }

    CHECK_AND_RETURN_RET(napiDemo->width_ > 0 && napiDemo->height_ > 0, result);
    MEDIA_LOGD("SetResolution success, width:%{public}d, height:%{public}d", napiDemo->width_, napiDemo->height_);

    return result;
}

napi_value NapiDemo::SetFrameCount(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter SetFrameCount");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET(status == napi_ok && jsThis != nullptr, result);

    NapiDemo *napiDemo = nullptr;
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&napiDemo));
    CHECK_AND_RETURN_RET(napiDemo != nullptr, result);

    napi_valuetype type = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &type) == napi_ok && type == napi_number) {
        (void)napi_get_value_int32(env, args[0], &napiDemo->totalFrameCount_);
    }

    CHECK_AND_RETURN_RET(napiDemo->totalFrameCount_ > 0, result);
    MEDIA_LOGD("SetFrameCount success:%{public}d", napiDemo->totalFrameCount_);
    return result;
}

napi_value NapiDemo::SetFrameRate(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter SetFrameRate");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET(status == napi_ok && jsThis != nullptr, result);

    NapiDemo *napiDemo = nullptr;
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&napiDemo));
    CHECK_AND_RETURN_RET(napiDemo != nullptr, result);

    napi_valuetype type = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &type) == napi_ok && type == napi_number) {
        (void)napi_get_value_int32(env, args[0], &napiDemo->frameRate_);
    }

    CHECK_AND_RETURN_RET(napiDemo->frameRate_ > 0, result);
    MEDIA_LOGD("SetFrameRate success:%{public}d", napiDemo->frameRate_);
    return result;
}

bool NapiDemo::StrToUint64(const std::string &str, uint64_t &value)
{
    if (str.empty() || !isdigit(str.front())) {
        return false;
    }

    char *end = nullptr;
    auto addr = str.data();
    auto result = strtoull(addr, &end, 10); // decimal
    if (end == addr || end[0] != '\0') {
        return false;
    }

    value = result;
    return true;
}

std::string NapiDemo::GetStringArgument(napi_env env, napi_value value)
{
    std::string strValue = "";
    size_t bufLength = 0;
    napi_status status = napi_get_value_string_utf8(env, value, nullptr, 0, &bufLength);
    if (status == napi_ok && bufLength > 0 && bufLength < PATH_MAX) {
        char *buffer = (char *)malloc((bufLength + 1) * sizeof(char));
        CHECK_AND_RETURN_RET(buffer != nullptr, strValue);
        status = napi_get_value_string_utf8(env, value, buffer, bufLength + 1, &bufLength);
        if (status == napi_ok) {
            MEDIA_LOGD("argument = %{public}s", buffer);
            strValue = buffer;
        }
        free(buffer);
        buffer = nullptr;
    }
    return strValue;
}

void NapiDemo::BufferLoop()
{
    pts_ = 0;
    color_ = 0xFF;
    count_ = 0;
    CHECK_AND_RETURN(frameRate_ > 0);
    const uint32_t sleepTime = 1000000 / frameRate_; // 1s = 1000000us
    const int64_t interval = 1000000000 / frameRate_; // 1s = 1000000000ns
    const uint32_t bufferSize = width_ * height_ * 3 / 2;
    while (count_ <= totalFrameCount_) {
        if (!isStart_.load()) {
            break;
        }
        usleep(sleepTime);

        OHOS::sptr<OHOS::SurfaceBuffer> buffer = nullptr;
        int32_t releaseFence = 0;

        CHECK_AND_BREAK(producerSurface_ != nullptr);
        if (producerSurface_->RequestBuffer(buffer, releaseFence, requestConfig_) != SURFACE_ERROR_OK) {
            continue;
        }
        CHECK_AND_BREAK(buffer != nullptr);

        ON_SCOPE_EXIT(0) { (void)producerSurface_->CancelBuffer(buffer); };

        sptr<SyncFence> tempFence = new SyncFence(releaseFence);
        tempFence->Wait(100); // 100ms

        auto addr = static_cast<uint8_t *>(buffer->GetVirAddr());
        CHECK_AND_BREAK(addr != nullptr);
        CHECK_AND_BREAK(bufferSize <= buffer->GetSize());
        CHECK_AND_BREAK(memset_s(addr, buffer->GetSize(), color_, bufferSize) == EOK);

        srand(static_cast<int32_t>(time(0)));
        constexpr uint32_t len = 100;
        for (uint32_t i = 0; i < bufferSize; i += len) {
            if (i >= bufferSize) {
                break;
            }
            addr[i] = (unsigned char)(rand() % 255); // 255 is the scope of RGB
        }

        if (count_ == totalFrameCount_) {
            (void)buffer->ExtraSet("endOfStream", true);
        } else {
            (void)buffer->ExtraSet("endOfStream", false);
        }

        (void)buffer->ExtraSet("timeStamp", pts_);

        count_++;
        color_ = color_ <= 0 ? 0xFF : (color_ - 1);
        pts_ += interval;

        CANCEL_SCOPE_EXIT_GUARD(0);

        MEDIA_LOGD("FlushBuffer %{public}d", count_);
        (void)producerSurface_->FlushBuffer(buffer, -1, flushConfig_);
    }
}
} // namespace Media
} // namespace OHOS
