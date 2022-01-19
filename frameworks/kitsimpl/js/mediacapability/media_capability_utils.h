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

#ifndef MEDIA_CAPABILITY_NAPI_UTILS_H
#define MEDIA_CAPABILITY_NAPI_UTILS_H

#include "common_napi.h"

namespace OHOS {
namespace Media {
class MediaJsAudioCapsStatic : public MediaJsResult {
public:
    explicit MediaJsAudioCapsStatic(bool isDecoder)
        : isDecoder_(isDecoder)
    {
    }
    ~MediaJsAudioCapsStatic() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override;

private:
    bool isDecoder_;
};

class MediaJsVideoCapsStatic : public MediaJsResult {
public:
    explicit MediaJsVideoCapsStatic(bool isDecoder)
        : isDecoder_(isDecoder)
    {
    }
    ~MediaJsVideoCapsStatic() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override;

private:
    bool isDecoder_;
};

class MediaJsResultAudioCapsDynamic : public MediaJsResult {
public:
    explicit MediaJsResultAudioCapsDynamic(std::string name, bool isDecoder)
        : name_(name),
          isDecoder_(isDecoder)
    {
    }
    ~MediaJsResultAudioCapsDynamic() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override;

private:
    std::string name_;
    bool isDecoder_;
};
}
}
#endif
