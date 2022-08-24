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

#ifndef PLAYER_CODEC_CTRL_H
#define PLAYER_CODEC_CTRL_H

#include <string>
#include <list>
#include <gst/gst.h>
#include <gst/player/player.h>

namespace OHOS {
namespace Media {
class PlayerCodecCtrl {
public:
    PlayerCodecCtrl();
    ~PlayerCodecCtrl();
    void DetectCodecSetup(const std::string &metaStr, GstElement *src, GstElement *videoSink);
    void DetectCodecUnSetup(GstElement *src, GstElement *videoSink);
    void EnhanceSeekPerformance(bool enable);

private:
    void SetupCodecCb(const std::string &metaStr, GstElement *src, GstElement *videoSink);
    void HlsSwichSoftAndHardCodec(GstElement *videoSink);
    void SetupCodecBufferNum(const std::string &metaStr, GstElement *src);

    bool isHardwareDec_ = false;
    GstElement *decoder_ = nullptr;
    std::list<bool> codecTypeList_;
};
} // Media
} // OHOS
#endif // PLAYER_CODEC_CTRL_H