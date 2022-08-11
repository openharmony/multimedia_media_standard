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

#include "surface_native_mock.h"
#include "ui/rs_surface_node.h"
#include "window_option.h"

namespace OHOS {
namespace Media {
namespace {
    constexpr uint32_t DEFAULT_WIDTH = 480;
    constexpr uint32_t DEFAULT_HEIGHT = 360;
}
SurfaceNativeMock::~SurfaceNativeMock()
{
    if (window_ != nullptr) {
        window_->Destroy();
        window_ = nullptr;
    }
}
sptr<Surface> SurfaceNativeMock::GetSurface()
{
    if (surface_ == nullptr) {
        sptr<Rosen::WindowOption> option = new Rosen::WindowOption();
        option->SetWindowRect({ 0, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT });
        option->SetWindowType(Rosen::WindowType::WINDOW_TYPE_APP_LAUNCHING);
        option->SetWindowMode(Rosen::WindowMode::WINDOW_MODE_FLOATING);
        window_ = Rosen::Window::Create("avcodec_unittest", option);
        if (window_ == nullptr || window_->GetSurfaceNode() == nullptr) {
            return nullptr;
        }
        window_->Show();
        surface_ = window_->GetSurfaceNode()->GetSurface();
    }
    return surface_;
}
} // Media
} // OHOS