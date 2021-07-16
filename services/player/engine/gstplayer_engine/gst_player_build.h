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

#ifndef PLAYER_PIPELINE_BUILD_H
#define PLAYER_PIPELINE_BUILD_H

#include <memory>
#include <string>
#include <gst/gst.h>
#include <gst/app/app.h>
#include <gst/player/player.h>
#include "gst_player_ctrl.h"
#include "gst_player_video_renderer_ctrl.h"

namespace OHOS {
namespace Media {
class GstPlayerBuild {
public:
    GstPlayerBuild();
    ~GstPlayerBuild();
    std::shared_ptr<GstPlayerCtrl> Build(sptr<Surface> surface = nullptr);
    void CreateLoop();

private:
    void Release();
    void DestroyLoop();

private:
    GMainContext *context_ = nullptr;
    GMainLoop *loop_ = nullptr;
    GstPlayerVideoRenderer *videoRenderer_ = nullptr;
    GstPlayerSignalDispatcher *signalDispatcher_ = nullptr;
    GstPlayer *gstPlayer_ = nullptr;
    std::shared_ptr<GstPlayerCtrl> playerCtrl_ = nullptr;
    std::shared_ptr<GstPlayerVideoRendererCtrl> rendererCtrl_ = nullptr;
};
} // Media
} // OHOS
#endif // PLAYER_PIPELINE_BUILD_H