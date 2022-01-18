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
#include "wm_common.h"
#include "foundation/windowmanager/interfaces/innerkits/wm/window_option.h"

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
    auto it = surfaceMap_.begin();
    while (it != surfaceMap_.end()) {
        if (it->second.promote() == surface) {
            return it->first;
        }
        if (it->second.promote() == nullptr) {
            it = surfaceMap_.erase(it);
        } else {
            ++it;
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

void MediaSurfaceImpl::Release()
{
    MEDIA_LOGD("Release");
    producerSurface_ = nullptr;
    mwindow_ =nullptr;
    if (previewWindow_ != nullptr) {
        previewWindow_->Destroy();
    }
    previewWindow_ = nullptr;
    surfaceMap_.clear();
}

sptr<Surface> MediaSurfaceImpl::GetSurface()
{
    if (previewWindow_ != nullptr || producerSurface_ != nullptr) {
        Release();
    }

    std::lock_guard<std::mutex> lock(mutex_);
    sptr<Rosen::WindowOption> option = new Rosen::WindowOption();
    option->SetWindowRect({ 0, 0, 1920, 1080 }); // 1920 is width, 1080 is height
    option->SetWindowType(Rosen::WindowType::WINDOW_TYPE_APP_LAUNCHING);
    option->SetWindowMode(Rosen::WindowMode::WINDOW_MODE_FLOATING);
    std::string winName = "media_player_window" + std::to_string(id_);
    previewWindow_ = Rosen::Window::Create(winName, option);
    id_++;
    if (previewWindow_ == nullptr || previewWindow_->GetSurfaceNode() == nullptr) {
        MEDIA_LOGE("previewWindow_ is nullptr");
        return nullptr;
    }

    producerSurface_ = previewWindow_->GetSurfaceNode()->GetSurface();
    previewWindow_->Show();
    MEDIA_LOGD("Create MediaSurfaceImpl Surface");
    return producerSurface_;
}
}
}