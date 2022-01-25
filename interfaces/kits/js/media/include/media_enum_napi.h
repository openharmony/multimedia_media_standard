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

#ifndef MEDIA_ENUM_NAPI_H_
#define MEDIA_ENUM_NAPI_H_

#include "media_data_source.h"
#include "callback_works.h"

namespace OHOS {
namespace Media {
class MediaEnumNapi {
public:
    MediaEnumNapi() = default;
    ~MediaEnumNapi() = default;
    DISALLOW_COPY_AND_MOVE(MediaEnumNapi);
    static napi_value Init(napi_env env, napi_value exports);

private:
    static napi_value JsEnumIntInit(napi_env env, napi_value exports);
    static napi_value JsEnumStringInit(napi_env env, napi_value exports);
};
} // namespace Media
} // namespace OHOS
#endif /* MEDIA_ENUM_NAPI_H_ */