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

#include "native_avformat.h"

#include "securec.h"
#include "native_avmagic.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "OH_AVFormat"};
}

using namespace OHOS::Media;

OH_AVFormat::OH_AVFormat()
    : AVObjectMagic(AVMagic::MEDIA_MAGIC_FORMAT)
{
}

OH_AVFormat::OH_AVFormat(const Format &fmt)
    : AVObjectMagic(AVMagic::MEDIA_MAGIC_FORMAT), format_(fmt)
{
}

OH_AVFormat::~OH_AVFormat()
{
    if (outString_ != nullptr) {
        free(outString_);
        outString_ = nullptr;
    }
}

struct OH_AVFormat *OH_AVFormat_Create(void)
{
    return new(std::nothrow) OH_AVFormat();
}

void OH_AVFormat_Destroy(struct OH_AVFormat *format)
{
    delete format;
}

bool OH_AVFormat_Copy(struct OH_AVFormat *to, struct OH_AVFormat *from)
{
    CHECK_AND_RETURN_RET_LOG(to != nullptr, false, "to format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(to->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, false, "magic error!");
    CHECK_AND_RETURN_RET_LOG(from != nullptr, false, "from format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(from->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, false, "magic error!");

    to->format_ = from->format_;
    return true;
}

bool OH_AVFormat_SetIntValue(struct OH_AVFormat *format, const char *key, int32_t value)
{
    CHECK_AND_RETURN_RET_LOG(format != nullptr, false, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, false, "magic error!");
    CHECK_AND_RETURN_RET_LOG(key != nullptr, false, "key is nullptr!");

    return format->format_.PutIntValue(key, value);
}

bool OH_AVFormat_SetLongValue(struct OH_AVFormat *format, const char *key, int64_t value)
{
    CHECK_AND_RETURN_RET_LOG(format != nullptr, false, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, false, "magic error!");
    CHECK_AND_RETURN_RET_LOG(key != nullptr, false, "key is nullptr!");

    return format->format_.PutLongValue(key, value);
}

bool OH_AVFormat_SetFloatValue(struct OH_AVFormat *format, const char *key, float value)
{
    CHECK_AND_RETURN_RET_LOG(format != nullptr, false, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, false, "magic error!");
    CHECK_AND_RETURN_RET_LOG(key != nullptr, false, "key is nullptr!");

    return format->format_.PutFloatValue(key, value);
}

bool OH_AVFormat_SetDoubleValue(struct OH_AVFormat *format, const char *key, double value)
{
    CHECK_AND_RETURN_RET_LOG(format != nullptr, false, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, false, "magic error!");
    CHECK_AND_RETURN_RET_LOG(key != nullptr, false, "key is nullptr!");

    return format->format_.PutDoubleValue(key, value);
}

bool OH_AVFormat_SetStringValue(struct OH_AVFormat *format, const char *key, const char *value)
{
    CHECK_AND_RETURN_RET_LOG(format != nullptr, false, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, false, "magic error!");
    CHECK_AND_RETURN_RET_LOG(key != nullptr, false, "key is nullptr!");
    CHECK_AND_RETURN_RET_LOG(value != nullptr, false, "value is nullptr!");

    return format->format_.PutStringValue(key, value);
}

bool OH_AVFormat_SetBuffer(struct OH_AVFormat *format, const char *key, const uint8_t *addr, size_t size)
{
    CHECK_AND_RETURN_RET_LOG(format != nullptr, false, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, false, "magic error!");
    CHECK_AND_RETURN_RET_LOG(key != nullptr, false, "key is nullptr!");
    CHECK_AND_RETURN_RET_LOG(addr != nullptr, false, "addr is nullptr!");
    CHECK_AND_RETURN_RET_LOG(size != 0, false, "size is zero!");

    return format->format_.PutBuffer(key, addr, size);
}

bool OH_AVFormat_GetIntValue(struct OH_AVFormat *format, const char *key, int32_t *out)
{
    CHECK_AND_RETURN_RET_LOG(format != nullptr, false, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, false, "magic error!");
    CHECK_AND_RETURN_RET_LOG(key != nullptr, false, "key is nullptr!");
    CHECK_AND_RETURN_RET_LOG(out != nullptr, false, "out is nullptr!");

    return format->format_.GetIntValue(key, *out);
}

bool OH_AVFormat_GetLongValue(struct OH_AVFormat *format, const char *key, int64_t *out)
{
    CHECK_AND_RETURN_RET_LOG(format != nullptr, false, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, false, "magic error!");
    CHECK_AND_RETURN_RET_LOG(key != nullptr, false, "key is nullptr!");
    CHECK_AND_RETURN_RET_LOG(out != nullptr, false, "out is nullptr!");

    return format->format_.GetLongValue(key, *out);
}

bool OH_AVFormat_GetFloatValue(struct OH_AVFormat *format, const char *key, float *out)
{
    CHECK_AND_RETURN_RET_LOG(format != nullptr, false, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, false, "magic error!");
    CHECK_AND_RETURN_RET_LOG(key != nullptr, false, "key is nullptr!");
    CHECK_AND_RETURN_RET_LOG(out != nullptr, false, "out is nullptr!");

    return format->format_.GetFloatValue(key, *out);
}

bool OH_AVFormat_GetDoubleValue(struct OH_AVFormat *format, const char *key, double *out)
{
    CHECK_AND_RETURN_RET_LOG(format != nullptr, false, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, false, "magic error!");
    CHECK_AND_RETURN_RET_LOG(key != nullptr, false, "key is nullptr!");
    CHECK_AND_RETURN_RET_LOG(out != nullptr, false, "out is nullptr!");

    return format->format_.GetDoubleValue(key, *out);
}

bool OH_AVFormat_GetStringValue(struct OH_AVFormat *format, const char *key, const char **out)
{
    CHECK_AND_RETURN_RET_LOG(format != nullptr, false, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, false, "magic error!");
    CHECK_AND_RETURN_RET_LOG(key != nullptr, false, "key is nullptr!");
    CHECK_AND_RETURN_RET_LOG(out != nullptr, false, "out is nullptr!");

    if (format->outString_ != nullptr) {
        free(format->outString_);
        format->outString_ = nullptr;
    }

    std::string str;
    constexpr uint32_t bufLength = 256;
    bool ret = format->format_.GetStringValue(key, str);
    if (!ret || str.size() > bufLength) {
        return false;
    }

    format->outString_ = (char *)malloc((bufLength + 1) * sizeof(char));
    CHECK_AND_RETURN_RET_LOG(format->outString_ != nullptr, false, "malloc out string nullptr!");

    if (strcpy_s(format->outString_, str.size(), str.c_str()) != EOK) {
        MEDIA_LOGE("Failed to strcpy_s");
        free(format->outString_);
        format->outString_ = nullptr;
        return false;
    }

    *out = format->outString_;
    return true;
}

bool OH_AVFormat_GetBuffer(struct OH_AVFormat *format, const char *key, uint8_t **addr, size_t *size)
{
    CHECK_AND_RETURN_RET_LOG(format != nullptr, false, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, false, "magic error!");
    CHECK_AND_RETURN_RET_LOG(key != nullptr, false, "key is nullptr!");
    CHECK_AND_RETURN_RET_LOG(addr != nullptr, false, "addr is nullptr!");
    CHECK_AND_RETURN_RET_LOG(size != nullptr, false, "size is nullptr!");

    return format->format_.GetBuffer(key, addr, *size);
}

const char *OH_AVFormat_DumpInfo(struct OH_AVFormat *format)
{
    return format->format_.Stringify().c_str();
}
