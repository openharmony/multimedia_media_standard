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

#include "media_surface_impl.h"
#include "media_log.h"
#include "media_errors.h"
#include "display_type.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaSurface"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<MediaSurface> MediaSurfaceFactory::CreateMediaSurface()
{
    static std::shared_ptr<MediaSurfaceImpl> instance = std::make_shared<MediaSurfaceImpl>();
    return instance;
}

MediaSurfaceImpl::MediaSurfaceImpl()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaSurfaceImpl::~MediaSurfaceImpl()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

std::string MediaSurfaceImpl::GetSurfaceId(const sptr<Surface> &surface)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = surfaceMap_.begin(); it != surfaceMap_.end(); ++it) {
        if (it->second.promote() == surface) {
            return it->first;
        }
        if (it->second.promote() == nullptr) {
            surfaceMap_.erase(it);
            --it;
        }
    }
    wptr<Surface> wpSurface = surface;
    ++idCount;
    std::string idKey = std::to_string(idCount);
    (void)surfaceMap_.insert(std::make_pair(idKey, wpSurface));
    MEDIA_LOGD("create surface id, surfaceId:%{public}s", idKey.c_str());
    return idKey;
}

sptr<Surface> MediaSurfaceImpl::GetSurface(const std::string &id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = surfaceMap_.find(id);
    if (it != surfaceMap_.end()) {
        return it->second.promote();
    }
    MEDIA_LOGE("failed to get surface, surfaceId:%{public}s", id.c_str());
    return nullptr;
}

sptr<Surface> MediaSurfaceImpl::GetSurface()
{
    sptr<WindowManager> wmi = WindowManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(wmi != nullptr, nullptr, "WindowManager is nullptr!");

    wmi->Init();
    sptr<WindowOption> option = WindowOption::Get();
    CHECK_AND_RETURN_RET_LOG(option != nullptr, nullptr, "WindowOption is nullptr!");
    const int32_t height = 360;
    const int32_t width = 640;
    (void)option->SetWidth(width);
    (void)option->SetHeight(height);
    (void)option->SetX(0);
    (void)option->SetY(0);
    (void)option->SetWindowType(WINDOW_TYPE_NORMAL);
    (void)wmi->CreateWindow(mwindow_, option);
    CHECK_AND_RETURN_RET_LOG(mwindow_ != nullptr, nullptr, "mwindow_ is nullptr!");

    sptr<Surface> producerSurface = mwindow_->GetSurface();
    CHECK_AND_RETURN_RET_LOG(producerSurface != nullptr, nullptr, "producerSurface is nullptr!");

    const std::string format = "SURFACE_FORMAT";
    (void)producerSurface->SetUserData(format, std::to_string(static_cast<int>(PIXEL_FMT_RGBA_8888)));
    return producerSurface;
}
}
}