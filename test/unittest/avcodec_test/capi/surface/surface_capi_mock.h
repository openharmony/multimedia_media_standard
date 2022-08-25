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

#ifndef SURFACE_CAPI_MOCK_H
#define SURFACE_CAPI_MOCK_H

#include "avcodec_mock.h"
#include "window.h"
#include "native_avcodec_base.h"

namespace OHOS {
namespace Media {
class SurfaceCapiMock : public SurfaceMock {
public:
    explicit SurfaceCapiMock(OHNativeWindow* nativeWindow) : nativeWindow_(nativeWindow) {}
    SurfaceCapiMock() = default;
    ~SurfaceCapiMock() = default;
    OHNativeWindow* GetSurface();

private:
    OHNativeWindow* nativeWindow_ = nullptr;
};
} // Media
} // OHOS
#endif // SURFACE_CAPI_MOCK_H