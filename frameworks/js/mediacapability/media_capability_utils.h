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
#include "recorder_profiles.h"

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

class MediaJsVideoCapsDynamic : public MediaJsResult {
public:
    MediaJsVideoCapsDynamic(const std::string &name, const bool &isDecoder)
        : name_(name),
          isDecoder_(isDecoder)
    {
    }
    ~MediaJsVideoCapsDynamic() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override;

private:
    std::string name_;
    bool isDecoder_;
};

class MediaJsAudioCapsDynamic : public MediaJsResult {
public:
    MediaJsAudioCapsDynamic(const std::string &name, const bool &isDecoder)
        : name_(name),
          isDecoder_(isDecoder)
    {
    }
    ~MediaJsAudioCapsDynamic() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override;

private:
    std::string name_;
    bool isDecoder_;
};

class MediaJsAudioRecorderCapsArray : public MediaJsResult {
public:
    explicit MediaJsAudioRecorderCapsArray(std::vector<std::shared_ptr<OHOS::Media::AudioRecorderCaps>> value)
        : value_(value)
    {
    }
    ~MediaJsAudioRecorderCapsArray() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override;

private:
    std::vector<std::shared_ptr<OHOS::Media::AudioRecorderCaps>> value_;
};

class MediaJsVideoRecorderCapsArray : public MediaJsResult {
public:
    explicit MediaJsVideoRecorderCapsArray(std::vector<std::shared_ptr<OHOS::Media::VideoRecorderCaps>> value)
        : value_(value)
    {
    }
    ~MediaJsVideoRecorderCapsArray() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override;

private:
    std::vector<std::shared_ptr<OHOS::Media::VideoRecorderCaps>> value_;
};

class MediaJsVideoRecorderProfile : public MediaJsResult {
public:
    explicit MediaJsVideoRecorderProfile(std::shared_ptr<OHOS::Media::VideoRecorderProfile> value)
        : value_(value)
    {
    }
    ~MediaJsVideoRecorderProfile() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override;

private:
    std::shared_ptr<OHOS::Media::VideoRecorderProfile> value_;
};

class MediaCapabilityUtil {
public:
    MediaCapabilityUtil() = delete;
    ~MediaCapabilityUtil() = delete;
    static bool ExtractAudioRecorderProfile(napi_env env, napi_value profile, AudioRecorderProfile &result);
};
} // namespace Media
} // namespace OHOS
#endif
